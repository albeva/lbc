//
// Created by Albert on 11/12/2022.
//
#pragma once
#include "Ast/Ast.hpp"
#include "Type/Type.hpp"

namespace lbc::Gen {

class CallingConenvention {
public:
    virtual void emitFunctionSignature(AstFuncDecl&) = 0;
    virtual void emitReturnValue(const TypeFunction*, AstReturnStmt&) = 0;
    virtual void emitCallFunction(const TypeFunction*, AstCallExpr&) = 0;
    virtual void lowerArgument(const TypeFunction*, AstFuncParamDecl&) = 0;
    virtual bool supportsCVarArg() = 0;
};

} // namespace lbc::Gen
