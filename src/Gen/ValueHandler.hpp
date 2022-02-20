//
// Created by Albert Varaksin on 28/05/2021.
//
#pragma once
#include <llvm/ADT/PointerUnion.h>

namespace lbc {
class CodeGen;
struct AstExpr;
struct AstIdentExpr;
struct AstMemberAccess;
struct AstDereference;
struct AstAddressOf;
struct TypeRoot;
class Symbol;

namespace Gen {
    class ValueHandler final : llvm::PointerUnion<llvm::Value*, AstExpr*> {
    public:
        /// Create temporary allocated variable - it is not inserted into symbol table
        static ValueHandler createTemp(CodeGen& gen, AstExpr& expr, StringRef name = "") noexcept;

        /// Create temporary variable if expression is not a constant
        static ValueHandler createTempOrConstant(CodeGen& gen, AstExpr& expr, StringRef name = "") noexcept;

        /// Create temporary from given llvm value
        static ValueHandler createOpaqueIdent(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::SMRange range, StringRef name) noexcept;
        static ValueHandler createOpaqueIdent(CodeGen& gen, Symbol* symbol, llvm::SMRange range, StringRef name) noexcept;

        constexpr ValueHandler() noexcept = default;
        ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value) noexcept;
        ValueHandler(CodeGen* gen, AstExpr& ast) noexcept;

        [[nodiscard]] llvm::Value* load() const noexcept;
        [[nodiscard]] llvm::Value* getAddress() const noexcept;
        [[nodiscard]] llvm::Type* getLlvmType() const noexcept;
        void store(llvm::Value* val) const noexcept;
        void store(ValueHandler& val) const noexcept;

        [[nodiscard]] constexpr inline bool isValid() const noexcept {
            return m_gen != nullptr;
        }

    private:
        using IndexArray = llvm::SmallVectorImpl<llvm::Value*>;
        [[nodiscard]] llvm::Value* getAggregateAddress(llvm::Value* base, IndexArray& idxs) const noexcept;

        CodeGen* m_gen = nullptr;
        const TypeRoot* m_type = nullptr;
    };

} // namespace Gen
} // namespace lbc
