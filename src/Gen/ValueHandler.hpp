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

namespace Gen {
    using ValuePtr = llvm::PointerIntPair<llvm::Value*, 1, bool>;

    class ValueHandler final : llvm::PointerUnion<ValuePtr, AstExpr*> {
    public:
        /// Create temporary allocated variable - it is not inserted into symbol table
        static ValueHandler createTemp(CodeGen& gen, AstExpr& expr, StringRef name = "") noexcept;

        /// Create temporary variable if expression is not a constant
        static ValueHandler createTempOrConstant(CodeGen& gen, AstExpr& expr, StringRef name = "") noexcept;

        constexpr ValueHandler() noexcept = default;
        ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value) noexcept;
        ValueHandler(CodeGen* gen, AstIdentExpr& ast) noexcept;
        ValueHandler(CodeGen* gen, AstMemberAccess& ast) noexcept;
        ValueHandler(CodeGen* gen, AstDereference& ast) noexcept;
        ValueHandler(CodeGen* gen, AstAddressOf& ast) noexcept;

        [[nodiscard]] llvm::Value* load() const noexcept;
        [[nodiscard]] llvm::Value* getAddress() const noexcept;
        [[nodiscard]] llvm::Type * getLlvmType() const noexcept;
        void store(llvm::Value* val) const noexcept;
        void store(ValueHandler& val) const noexcept {
            store(val.load());
        }

        [[nodiscard]] constexpr inline bool isValid() const noexcept {
            return m_gen != nullptr;
        }

    private:
        ValueHandler(CodeGen* gen, const TypeRoot* type, ValuePtr ptr) noexcept;

        using IndexArray = llvm::SmallVectorImpl<llvm::Value*>;
        [[nodiscard]] llvm::Value* getAggregateAddress(llvm::Value* base, IndexArray& idxs) const noexcept;

        CodeGen* m_gen = nullptr;
        const TypeRoot* m_type = nullptr;
    };

} // namespace Gen
} // namespace lbc
