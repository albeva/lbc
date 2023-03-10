//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "pch.hpp"
#include "AstVisitor.hpp"

namespace lbc {

class CodePrinter final : public AstVisitor<CodePrinter> {
public:
    explicit CodePrinter(llvm::raw_ostream& os) : m_os{ os } {}
    AST_VISITOR_DECLARE_CONTENT_FUNCS()

private:
    [[nodiscard]] std::string indent() const;
    size_t m_indent = 0;
    llvm::raw_ostream& m_os;
    static constexpr auto SPACES = 4;
    bool m_emitDimKeyword = true;
};

} // namespace lbc