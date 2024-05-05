//
// Created by Albert Varaksin on 28/05/2021.
//
#pragma once
#include "pch.hpp"
#include <llvm/ADT/PointerUnion.h>

namespace lbc {
class CodeGen;
struct AstExpr;
struct AstIdentExpr;
struct AstBinaryExpr;
struct AstDereference;
struct AstAddressOf;
class TypeRoot;
class Symbol;

namespace Gen {
    class ValueHandler final : llvm::PointerUnion<llvm::Value*, Symbol*, AstExpr*> {
    public:
        /// Create temporary allocated variable - it is not inserted into symbol table
        static ValueHandler createTemp(CodeGen& gen, AstExpr& expr, llvm::StringRef name = "");

        /// Create temporary variable if expression is not a constant
        static ValueHandler createTempOrConstant(CodeGen& gen, AstExpr& expr, llvm::StringRef name = "");

        /// Create temporary from given llvm value
        static ValueHandler createOpaqueValue(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::StringRef name);

        constexpr ValueHandler() = default;
        ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value);
        ValueHandler(CodeGen* gen, Symbol* symbol);
        ValueHandler(CodeGen* gen, AstIdentExpr& ast);
        ValueHandler(CodeGen* gen, AstBinaryExpr& ast);
        ValueHandler(CodeGen* gen, AstDereference& ast);
        ValueHandler(CodeGen* gen, AstAddressOf& ast);

        [[nodiscard]] llvm::Value* getAddress() const;
        [[nodiscard]] llvm::Value* load() const;
        [[nodiscard]] llvm::Type* getLlvmType() const;
        void store(llvm::Value* val) const;
        void store(ValueHandler& val) const;

        [[nodiscard]] constexpr inline bool isValid() const {
            return m_gen != nullptr;
        }

    private:
        [[nodiscard]] llvm::Value* getAggregageAddress(AstBinaryExpr& ast) const;

        CodeGen* m_gen = nullptr;
        const TypeRoot* m_type = nullptr;
    };

} // namespace Gen
} // namespace lbc
