//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Generator.hpp"
#include "Type/Aggregate.hpp"
#include "Type/Numeric.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace lbc::gen;

auto Generator::lowerType(const Type* type) -> llvm::Type* {
    if (auto* cached = type->getLlvmType()) {
        return cached;
    }

    llvm::Type* result = nullptr;
    if (type->isVoid()) {
        result = m_builder.getVoidTy();
    } else if (type->isBool()) {
        result = m_builder.getInt1Ty();
    } else if (const auto* integral = llvm::dyn_cast<TypeIntegral>(type)) {
        // Width comes from the type factory's target sizing, not a fixed map.
        result = m_builder.getIntNTy(static_cast<unsigned>(integral->getBits()));
    } else if (const auto* fp = llvm::dyn_cast<TypeFloatingPoint>(type)) {
        result = fp->getBits() == 32 ? m_builder.getFloatTy() : m_builder.getDoubleTy();
    } else if (type->isConst()) {
        result = lowerType(type->getBaseType()); // const is erased at the LLVM level
    } else if (type->isPointer() || type->isReference() || type->isZString() || type->isAny() || type->isNull()) {
        result = llvm::PointerType::get(m_llvm, 0); // opaque pointer
    } else if (type->isFunction()) {
        result = lowerFunctionType(type);
    } else {
        std::unreachable();
    }

    type->setLlvmType(result);
    return result;
}

auto Generator::lowerFunctionType(const Type* type) -> llvm::FunctionType* {
    const auto* fn = llvm::cast<TypeFunction>(type);
    auto* ret = lowerType(fn->getReturnType());
    llvm::SmallVector<llvm::Type*> params;
    params.reserve(fn->getParams().size());
    for (const auto* param : fn->getParams()) {
        params.push_back(lowerType(param));
    }
    return llvm::FunctionType::get(ret, params, false);
}

auto Generator::isSigned(const Type* type) -> bool {
    const auto* unqualified = type->isConst() ? type->getBaseType() : type;
    const auto* integral = llvm::dyn_cast<TypeIntegral>(unqualified);
    return integral != nullptr && integral->isSigned();
}

auto Generator::lowerCast(llvm::Value* val, const Type* from, const Type* to) -> llvm::Value* {
    auto* src = lowerType(from);
    auto* dst = lowerType(to);
    if (src == dst) {
        return val;
    }

    const bool fromInt = src->isIntegerTy();
    const bool toInt = dst->isIntegerTy();
    const bool fromFp = src->isFloatingPointTy();
    const bool toFp = dst->isFloatingPointTy();

    if (fromInt && toInt) {
        if (dst->getIntegerBitWidth() > src->getIntegerBitWidth()) {
            return isSigned(from) ? m_builder.CreateSExt(val, dst) : m_builder.CreateZExt(val, dst);
        }
        return m_builder.CreateTrunc(val, dst);
    }
    if (fromInt && toFp) {
        return isSigned(from) ? m_builder.CreateSIToFP(val, dst) : m_builder.CreateUIToFP(val, dst);
    }
    if (fromFp && toInt) {
        return isSigned(to) ? m_builder.CreateFPToSI(val, dst) : m_builder.CreateFPToUI(val, dst);
    }
    if (fromFp && toFp) {
        return src->isFloatTy() ? m_builder.CreateFPExt(val, dst) : m_builder.CreateFPTrunc(val, dst);
    }
    // Pointer / reference conversions are no-ops under opaque pointers.
    return val;
}
