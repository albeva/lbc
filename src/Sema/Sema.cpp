//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

SemanticAnalyser::SemanticAnalyser(Context& context)
: m_context(context) {
}

SemanticAnalyser::~SemanticAnalyser() = default;

auto SemanticAnalyser::analyse(AstModule& ast) -> Result {
    return accept(ast);
}

auto SemanticAnalyser::accept(AstModule& ast) -> Result {
    return accept(*ast.getStmtList());
}
