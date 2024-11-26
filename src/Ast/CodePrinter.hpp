//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "pch.hpp"
#include "AstVisitor.hpp"

namespace lbc {

class CodePrinter final : public AstVisitor<CodePrinter> {
    NO_COPY_AND_MOVE(CodePrinter)
public:
    explicit CodePrinter(llvm::raw_ostream& os) : m_os{ os } {}
    ~CodePrinter() = default;

    AST_VISITOR_DECLARE_CONTENT_FUNCS()

private:
    [[nodiscard]] auto indent() const -> std::string;
    size_t m_indent = 0;
    llvm::raw_ostream& m_os;
    static constexpr size_t SPACES = 4;
    bool m_emitDimKeyword = true;
    ControlFlowStack<> m_controlStack{};
};

} // namespace lbc