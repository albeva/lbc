//
// Created by Albert Varaksin on 07/07/2020.
//
#include "Type.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
#include <llvm/Support/Alignment.h>
using namespace lbc;

namespace {

// Commonly used types
constexpr TypeVoid voidTy {}; // VOID
constexpr TypeAny anyTy {}; // Any typeExpr
constexpr TypePointer anyPtrTy { &anyTy }; // void*

// clang-format off

// primitives
#define DEFINE_TYPE(ID, STR, KIND, CPP) \
    constexpr Type## KIND ID## Ty{};
    PRIMITIVE_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

// integers
#define DEFINE_TYPE(ID, STR, KIND, CPP, BITS, ISSIGNED, ...) \
    constexpr Type## KIND ID## Ty{ TypeKind::ID, BITS, ISSIGNED };
    INTEGRAL_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

// Floating Points
#define DEFINE_TYPE(ID, STR, KIND, CPP, BITS, ...) \
    constexpr Type## KIND ID## Ty{ TypeKind::ID, BITS };
    FLOATINGPOINT_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

} // namespace

auto TypeRoot::fromTokenKind(const TokenKind kind) -> const TypeRoot* {
    #define CASE_TYPE(ID, ...) case TokenKind::ID: return &ID## Ty;
    switch (kind) {
    PRIMITIVE_TYPES(CASE_TYPE)
    INTEGRAL_TYPES(CASE_TYPE)
    FLOATINGPOINT_TYPES(CASE_TYPE)
    case TokenKind::Null:
        return &anyPtrTy;
    case TokenKind::Any:
        return &anyTy;
    case TokenKind::SizeOf:
        return &IntegerTy;
    default:
        fatalError("Unknown typeExpr "_t + Token::description(kind), false);
    }
    #undef CASE_TYPE
}

// clang-format on

auto TypeRoot::isAnyPointer() const -> bool {
    return this == &anyPtrTy;
}

auto TypeRoot::isSignedIntegral() const -> bool {
    if (!isIntegral()) {
        return false;
    }
    return llvm::cast<TypeIntegral>(this)->isSigned();
}

auto TypeRoot::isUnsignedIntegral() const -> bool {
    if (!isIntegral()) {
        return false;
    }
    return !llvm::cast<TypeIntegral>(this)->isSigned();
}

// clang-format off
#define CHECK_TYPE_IMPL(ID, ...)             \
    auto TypeRoot::is## ID() const -> bool {  \
        return this == &ID## Ty;              \
    }
    INTEGRAL_TYPES(CHECK_TYPE_IMPL)
    FLOATINGPOINT_TYPES(CHECK_TYPE_IMPL)
#undef CHECK_TYPE_IMPL
// clang-format on

auto TypeRoot::compare(const TypeRoot* other) const -> TypeComparison {
    if (this == other) {
        return TypeComparison::Equal;
    }

    if (const auto* ref = llvm::dyn_cast<TypeReference>(this)) {
        return ref->getBase()->compare(other);
    }

    if (const auto* ref = llvm::dyn_cast<TypeReference>(other)) {
        return compare(ref->getBase());
    }

    if (const auto* left = llvm::dyn_cast<TypePointer>(this)) {
        if (const auto* right = llvm::dyn_cast<TypePointer>(other)) {
            if (left->getBase()->isAny()) {
                return TypeComparison::Upcast;
            }
            if (right->getBase()->isAny()) {
                return TypeComparison::Downcast;
            }
        }
        return TypeComparison::Incompatible;
    }

    if (const auto* left = llvm::dyn_cast<TypeIntegral>(this)) {
        if (const auto* right = llvm::dyn_cast<TypeIntegral>(other)) {
            if (left->getBits() > right->getBits()) {
                return TypeComparison::Downcast;
            }
            if (left->getBits() < right->getBits()) {
                return TypeComparison::Upcast;
            }
            if (left->isSigned()) {
                return TypeComparison::Downcast;
            }
            if (right->isSigned()) {
                return TypeComparison::Upcast;
            }
        } else if (other->isFloatingPoint()) {
            return TypeComparison::Upcast;
        }
        return TypeComparison::Incompatible;
    }

    if (const auto& left = llvm::dyn_cast<TypeFloatingPoint>(this)) {
        if (other->isIntegral()) {
            return TypeComparison::Downcast;
        }
        if (const auto* right = llvm::dyn_cast<TypeFloatingPoint>(other)) {
            if (left->getBits() > right->getBits()) {
                return TypeComparison::Downcast;
            }
            return TypeComparison::Upcast;
        }
    }

    return TypeComparison::Incompatible;
}

auto TypeRoot::getSize(Context& context) const -> std::size_t {
    auto* llvmType = getLlvmType(context);
    constexpr auto bitsInByte = 8;
    const auto size = context.getDataLayout().getTypeSizeInBits(llvmType) / bitsInByte;
    if (size == 0 && isBoolean()) {
        return 1;
    }
    return size;
}

auto TypeRoot::getAlignment(Context& context) const -> std::size_t {
    auto* llvmType = getLlvmType(context);
    return context.getDataLayout().getPrefTypeAlign(llvmType).value();
}

auto TypeRoot::getPointer(Context& context) const -> const TypePointer* {
    return TypePointer::get(context, this);
}

auto TypeRoot::getReference(Context& context) const -> const TypeReference* {
    assert(!this->isReference() && "Type is already a reference");
    return TypeReference::get(context, this);
}

auto TypeRoot::getUnderlyingFunctionType() const -> const TypeFunction* {
    if (isFunction()) {
        return llvm::cast<TypeFunction>(this);
    }
    if (isPointer()) {
        return llvm::cast<TypePointer>(this)->getUnderlyingFunctionType();
    }
    return nullptr;
}

// Void

auto TypeVoid::get() -> const TypeVoid* {
    return &voidTy;
}

auto TypeVoid::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::Type::getVoidTy(context.getLlvmContext());
}

auto TypeVoid::asString() const -> std::string {
    return "VOID";
}

// Any
auto TypeAny::get() -> const TypeAny* {
    return &anyTy;
}

auto TypeAny::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::Type::getInt8Ty(context.getLlvmContext());
}

auto TypeAny::asString() const -> std::string {
    return "ANY";
}

// Pointer

auto TypePointer::get(Context& context, const TypeRoot* base) -> const TypePointer* {
    if (base == &anyTy) {
        return &anyPtrTy;
    }

    if (const auto* ref = llvm::dyn_cast<TypeReference>(base)) {
        return ref->convertToPointer(context);
    }

    for (const auto& ptr : context.getTypes(TypeFamily::Pointer)) {
        const auto* ptrType = llvm::cast<TypePointer>(ptr);
        if (ptrType->m_base == base) {
            return ptrType;
        }
    }

    const auto* ty = context.create<TypePointer>(base);
    context.getTypes(TypeFamily::Pointer).push_back(ty);
    return ty;
}

auto TypePointer::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::PointerType::get(context.getLlvmContext(), 0);
}

auto TypePointer::asString() const -> std::string {
    return m_base->asString() + " PTR";
}

// Reference

auto TypeReference::get(Context& context, const TypeRoot* base) -> const TypeReference* {
    assert(!base->isReference() && "Type is already a reference");

    for (const auto& ptrRef : context.getTypes(TypeFamily::Reference)) {
        if (const auto* ptr = llvm::cast<TypeReference>(ptrRef); ptr->m_base == base) {
            return ptr;
        }
    }

    const auto* ty = context.create<TypeReference>(base);
    context.getTypes(TypeFamily::Reference).push_back(ty);
    return ty;
}

auto TypeReference::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::PointerType::get(context.getLlvmContext(), 0);
}

auto TypeReference::convertToPointer(Context& context) const -> const TypePointer* {
    return TypePointer::get(context, m_base);
}

auto TypeReference::asString() const -> std::string {
    return m_base->asString() + " REF";
}

// Bool

auto TypeBoolean::get() -> const TypeBoolean* {
    return &BoolTy;
}

auto TypeBoolean::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::Type::getInt1Ty(context.getLlvmContext());
}

auto TypeBoolean::asString() const -> std::string {
    return "BOOL";
}

// Integer

auto TypeIntegral::get(unsigned bits, bool isSigned) -> const TypeIntegral* {
    // clang-format off
    #define USE_TYPE(ID, STR, KIND, CPP, BITS, IS_SIGNED, ...) \
        if (bits == BITS && isSigned == IS_SIGNED)        \
            return &ID## Ty;
    INTEGRAL_TYPES(USE_TYPE)
    #undef USE_TYPE
    // clang-format on

    fatalError("Invalid integer type size: "_t + llvm::Twine(bits), false);
}

auto TypeIntegral::getSigned() const -> const TypeIntegral* {
    return TypeIntegral::get(getBits(), true);
}

auto TypeIntegral::getUnsigned() const -> const TypeIntegral* {
    return TypeIntegral::get(getBits(), false);
}

auto TypeIntegral::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::IntegerType::get(context.getLlvmContext(), getBits());
}

auto TypeIntegral::asString() const -> std::string {
    // clang-format off
    #define GET_TYPE(ID, STR, KIND, CPP, BITS, SIGNED) \
        if (getBits() == (BITS) && isSigned() == (SIGNED)) \
            return STR;
    INTEGRAL_TYPES(GET_TYPE)
    #undef GET_TYPE
    // clang-format on
    llvm_unreachable("unknown integer type");
}

// Floating Point

auto TypeFloatingPoint::get(unsigned bits) -> const TypeFloatingPoint* {
    // clang-format off
    switch (bits) {
    #define USE_TYPE(ID, STR, KIND, CPP, BITS) \
        case BITS:                             \
            return &ID## Ty;
        FLOATINGPOINT_TYPES(USE_TYPE)
    #undef USE_TYPE
    default:
        fatalError("Invalid floating point type size: "_t + llvm::Twine(bits), false);
    }
    // clang-format on
}

auto TypeFloatingPoint::genLlvmType(Context& context) const -> llvm::Type* {
    switch (getBits()) {
    case 32: // NOLINT
        return llvm::Type::getFloatTy(context.getLlvmContext());
    case 64: // NOLINT
        return llvm::Type::getDoubleTy(context.getLlvmContext());
    default:
        fatalError("Invalid floating point type size: "_t + llvm::Twine(getBits()), false);
    }
}

auto TypeFloatingPoint::asString() const -> std::string {
    // clang-format off
    #define GET_TYPE(ID, STR, kind, CPP, BITS) \
        if (getBits() == (BITS))               \
            return STR;
    FLOATINGPOINT_TYPES(GET_TYPE)
    #undef GET_TYPE
    // clang-format on
    llvm_unreachable("unknown floating point type");
}

// Function

auto TypeFunction::get(
    Context& context,
    const TypeRoot* retType,
    llvm::SmallVector<const TypeRoot*> paramTypes,
    bool variadic
) -> const TypeFunction* {
    for (const auto& funcPtr : context.getTypes(TypeFamily::Function)) {
        const auto* ptr = llvm::cast<TypeFunction>(funcPtr);
        if (ptr->getReturn() == retType && ptr->getParams() == paramTypes && ptr->isVariadic() == variadic) {
            return ptr;
        }
    }

    auto* ty = context.create<TypeFunction>(retType, std::move(paramTypes), variadic);
    context.getTypes(TypeFamily::Function).push_back(ty);
    return ty;
}

auto TypeFunction::genLlvmType(Context& context) const -> llvm::Type* {
    llvm::Type* retTy = m_retType->getLlvmType(context);

    llvm::SmallVector<llvm::Type*> params;
    params.reserve(m_paramTypes.size());
    for (const auto& param : m_paramTypes) {
        params.emplace_back(param->getLlvmType(context));
    }

    return llvm::FunctionType::get(retTy, params, m_variadic);
}

auto TypeFunction::asString() const -> std::string {
    std::string out = m_retType != nullptr ? "FUNCTION(" : "SUB(";
    for (size_t i = 0; i < m_paramTypes.size(); i++) {
        if (i > 0) {
            out += ", ";
        }
        out += m_paramTypes[i]->asString();
    }
    out += ")";
    if (m_retType != nullptr) {
        out += " AS " + m_retType->asString();
    }
    return out;
}

// ZString
auto TypeZString::get() -> const TypeZString* {
    return &ZStringTy;
}

auto TypeZString::genLlvmType(Context& context) const -> llvm::Type* {
    return llvm::PointerType::get(context.getLlvmContext(), 0);
}

auto TypeZString::asString() const -> std::string {
    return "ZSTRING";
}
