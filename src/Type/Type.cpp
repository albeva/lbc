//
// Created by Albert Varaksin on 07/07/2020.
//
#include "Type.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
using namespace lbc;

namespace {
// Commonly used types
constexpr TypeVoid voidTy{};              // VOID
constexpr TypeAny anyTy{};                // Any typeExpr
constexpr TypePointer anyPtrTy{ &anyTy }; // void*

// clang-format off

// primitives
#define DEFINE_TYPE(ID, STR, KIND) \
    constexpr Type##KIND ID##Ty;
    PRIMITIVE_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

// integers
#define DEFINE_TYPE(ID, STR, KIND, BITS, ISSIGNED, ...) \
    constexpr Type##KIND ID##Ty{ BITS, ISSIGNED };
    INTEGRAL_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

// Floating Points
#define DEFINE_TYPE(ID, STR, KIND, BITS, ...) \
    constexpr Type##KIND ID##Ty{ BITS };
    FLOATINGPOINT_TYPES(DEFINE_TYPE)
#undef DEFINE_TYPE

} // namespace

const TypeRoot* TypeRoot::fromTokenKind(TokenKind kind) noexcept {
    #define CASE_TYPE(ID, ...) case TokenKind::ID: return &ID##Ty;
    switch (kind) {
    PRIMITIVE_TYPES(CASE_TYPE)
    INTEGRAL_TYPES(CASE_TYPE)
    FLOATINGPOINT_TYPES(CASE_TYPE)
    case TokenKind::Null:
        return &anyPtrTy;
    case TokenKind::Any:
        return &anyTy;
    default:
        fatalError("Unknown typeExpr "_t + Token::description(kind), false);
    }

    #undef TO_PRIMITIVE_TYPE
    #undef CASE_INTEGER
    #undef CASE_INTEGER
}

// clang-format on

bool TypeRoot::isAnyPointer() const noexcept {
    return this == &anyPtrTy;
}

bool TypeRoot::isSignedIntegral() const noexcept {
    if (!isIntegral()) {
        return false;
    }
    return static_cast<const TypeIntegral*>(this)->isSigned();
}

bool TypeRoot::isUnsignedIntegral() const noexcept {
    if (!isIntegral()) {
        return false;
    }
    return !static_cast<const TypeIntegral*>(this)->isSigned();
}

// clang-format off
#define CHECK_TYPE_IMPL(ID, ...)             \
    bool TypeRoot::is##ID() const noexcept { \
        return this == &ID##Ty;              \
    }
    INTEGRAL_TYPES(CHECK_TYPE_IMPL)
    FLOATINGPOINT_TYPES(CHECK_TYPE_IMPL)
#undef CHECK_TYPE_IMPL
// clang-format on

TypeComparison TypeRoot::compare(const TypeRoot* other) const noexcept {
    if (this == other) {
        return TypeComparison::Equal;
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

const TypePointer* TypeRoot::getPointer(Context& context) const noexcept {
    return TypePointer::get(context, this);
}

const TypeFunction* TypeRoot::getUnderlyingFunctionType() const noexcept {
    if (isFunction()) {
        return static_cast<const TypeFunction*>(this);
    }
    if (isPointer()) {
        return static_cast<const TypePointer*>(this)->getUnderlyingFunctionType();
    }
    return nullptr;
}

// Void

const TypeVoid* TypeVoid::get() noexcept {
    return &voidTy;
}

llvm::Type* TypeVoid::genLlvmType(Context& context) const {
    return llvm::Type::getVoidTy(context.getLlvmContext());
}

std::string TypeVoid::asString() const {
    return "VOID";
}

// Any
const TypeAny* TypeAny::get() noexcept {
    return &anyTy;
}

llvm::Type* TypeAny::genLlvmType(Context& context) const {
    return llvm::Type::getInt8Ty(context.getLlvmContext());
}

std::string TypeAny::asString() const {
    return "ANY";
}

// Pointer

const TypePointer* TypePointer::get(Context& context, const TypeRoot* base) noexcept {
    if (base == &anyTy) {
        return &anyPtrTy;
    }

    for (const auto& ptr : context.ptrTypes) {
        if (ptr->m_base == base) {
            return ptr;
        }
    }

    auto* ty = context.create<TypePointer>(base);
    context.ptrTypes.push_back(ty);
    return ty;
}

llvm::Type* TypePointer::genLlvmType(Context& context) const {
    return llvm::PointerType::get(m_base->getLlvmType(context), 0);
}

std::string TypePointer::asString() const {
    return m_base->asString() + " PTR";
}

// Bool

const TypeBoolean* TypeBoolean::get() noexcept {
    return &BoolTy;
}

llvm::Type* TypeBoolean::genLlvmType(Context& context) const {
    return llvm::Type::getInt1Ty(context.getLlvmContext());
}

std::string TypeBoolean::asString() const {
    return "BOOL";
}

// Integer

const TypeIntegral* TypeIntegral::get(unsigned bits, bool isSigned) noexcept {
#define USE_TYPE(ID, STR, KIND, BITS, IS_SIGNED, ...) \
    if (bits == BITS && isSigned == IS_SIGNED)        \
        return &ID##Ty;
    INTEGRAL_TYPES(USE_TYPE)
#undef USE_TYPE

    fatalError("Invalid integer type size: "_t + llvm::Twine(bits), false);
}

const TypeIntegral* TypeIntegral::getSigned() const noexcept {
    return TypeIntegral::get(getBits(), true);
}

const TypeIntegral* TypeIntegral::getUnsigned() const noexcept {
    return TypeIntegral::get(getBits(), false);
}

llvm::Type* TypeIntegral::genLlvmType(Context& context) const {
    return llvm::IntegerType::get(context.getLlvmContext(), getBits());
}

std::string TypeIntegral::asString() const {
#define GET_TYPE(ID, STR, KIND, BITS, SIGNED, ...) \
    if (getBits() == BITS && isSigned() == SIGNED) \
        return STR;
    INTEGRAL_TYPES(GET_TYPE)
#undef GET_TYPE

    llvm_unreachable("unknown integer type");
}

// Floating Point

const TypeFloatingPoint* TypeFloatingPoint::get(unsigned bits) noexcept {
    switch (bits) {
#define USE_TYPE(ID, STR, KIND, BITS, ...) \
    case BITS:                             \
        return &ID##Ty;
        FLOATINGPOINT_TYPES(USE_TYPE)
#undef USE_TYPE
    default:
        fatalError("Invalid floating point type size: "_t + llvm::Twine(bits), false);
    }
}

llvm::Type* TypeFloatingPoint::genLlvmType(Context& context) const {
    switch (getBits()) {
    case 32: // NOLINT
        return llvm::Type::getFloatTy(context.getLlvmContext());
    case 64: // NOLINT
        return llvm::Type::getDoubleTy(context.getLlvmContext());
    default:
        fatalError("Invalid floating point type size: "_t + llvm::Twine(getBits()), false);
    }
}

std::string TypeFloatingPoint::asString() const {
#define GET_TYPE(ID, STR, kind, BITS, ...) \
    if (getBits() == BITS)                 \
        return STR;
    FLOATINGPOINT_TYPES(GET_TYPE)
#undef GET_TYPE

    llvm_unreachable("unknown floating point type");
}

// Function

const TypeFunction* TypeFunction::get(
    Context& context,
    const TypeRoot* retType,
    std::vector<const TypeRoot*> paramTypes,
    bool variadic) noexcept {
    for (const auto& ptr : context.funcTypes) {
        if (ptr->getReturn() == retType && ptr->getParams() == paramTypes && ptr->isVariadic() == variadic) {
            return ptr;
        }
    }

    auto* ty = context.create<TypeFunction>(retType, std::move(paramTypes), variadic);
    context.funcTypes.push_back(ty);
    return ty;
}

llvm::Type* TypeFunction::genLlvmType(Context& context) const {
    auto* retTy = m_retType->getLlvmType(context);

    std::vector<llvm::Type*> params;
    params.reserve(m_paramTypes.size());
    for (const auto& param : m_paramTypes) {
        params.emplace_back(param->getLlvmType(context));
    }

    return llvm::FunctionType::get(retTy, params, m_variadic);
}

std::string TypeFunction::asString() const {
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
const TypeZString* TypeZString::get() noexcept {
    return &ZStringTy;
}

llvm::Type* TypeZString::genLlvmType(Context& context) const {
    return llvm::Type::getInt8PtrTy(context.getLlvmContext());
}

std::string TypeZString::asString() const {
    return "ZSTRING";
}
