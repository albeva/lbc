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
struct AstMemberExpr;
struct AstDereference;
struct AstAddressOf;
struct AstAlignOfExpr;
struct AstSizeOfExpr;
class TypeRoot;
class Symbol;

namespace Gen {
    class ValueHandler final : public llvm::PointerUnion<llvm::Value*, Symbol*, AstExpr*> {
    public:
        /// Create temporary allocated variable - it is not inserted into symbol table
        static auto createTemp(CodeGen& gen, AstExpr& expr, llvm::StringRef name = "") -> ValueHandler;

        /// Create temporary variable if expression is not a constant
        static auto createTempOrConstant(CodeGen& gen, AstExpr& expr, llvm::StringRef name = "") -> ValueHandler;

        /// Create temporary from given llvm value
        static auto createOpaqueValue(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::StringRef name) -> ValueHandler;

        constexpr ValueHandler() = default;
        ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value);
        ValueHandler(CodeGen* gen, Symbol* symbol);
        ValueHandler(CodeGen* gen, AstIdentExpr& ast);
        ValueHandler(CodeGen* gen, AstMemberExpr& ast);
        ValueHandler(CodeGen* gen, AstDereference& ast);
        ValueHandler(CodeGen* gen, AstAddressOf& ast);
        ValueHandler(CodeGen* gen, AstAlignOfExpr& ast);
        ValueHandler(CodeGen* gen, AstSizeOfExpr& ast);

        [[nodiscard]] auto getAddress() const -> llvm::Value*;
        [[nodiscard]] auto load(bool addressOnly = false) const -> llvm::Value*;
        [[nodiscard]] auto getLlvmType() const -> llvm::Type*;
        void store(llvm::Value* val) const;
        void store(const ValueHandler& val) const;

        [[nodiscard]] constexpr auto isValid() const -> bool {
            return m_gen != nullptr;
        }

    private:
        CodeGen* m_gen = nullptr;
        const TypeRoot* m_type = nullptr;
    };

} // namespace Gen
} // namespace lbc
