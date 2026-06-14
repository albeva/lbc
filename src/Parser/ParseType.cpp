//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Parser.hpp"
using namespace lbc;

// type = [ "CONST" ] builtin { "CONST" | "PTR" } [ "REF" ] .
//
// CONST and PTR qualifiers may repeat in any order; a REF, if present, must be
// the last (outermost) qualifier. Redundant CONST is collapsed, and the
// remaining combinations validated, during semantic analysis.
auto Parser::type() -> Result<AstType*> {
    const auto start = startLoc();
    bool isConst = false; // NOLINT(*-const-correctness)
    TRY_IF (accept(TokenKind::Const)) {
        isConst = true;
    }

    AstType* ty {}; // NOLINT(*-const-correctness)
    TRY_ASSIGN(ty, builtinType())
    if (isConst) {
        ty = make<AstConstType>(range(start), ty);
    }

    // CONST and PTR qualifiers may repeat in any order.
    while (true) {
        TRY_IF (accept(TokenKind::Const)) {
            ty = make<AstConstType>(range(ty), ty);
            continue;
        }
        TRY_IF (accept(TokenKind::Ptr)) {
            ty = make<AstPointerType>(range(ty), ty);
            continue;
        }
        break;
    }

    // A REF, if present, must be the last (outermost) qualifier — nothing may
    // qualify a reference further.
    TRY_IF (accept(TokenKind::Ref)) {
        ty = make<AstReferenceType>(range(ty), ty);
        const auto next = m_token.kind();
        if (next == TokenKind::Ref || next == TokenKind::Ptr || next == TokenKind::Const) {
            return diag(diagnostics::referenceNotLast(), m_token.getRange());
        }
    }

    return ty;
}

// -------------------------------------------------------------------------
// builtin = "BOOL"    | "ZSTRING"
//         | "BYTE"    | "UBYTE"
//         | "SHORT"   | "USHORT"
//         | "INTEGER" | "UINTEGER"
//         | "LONG"    | "ULONG"
//         | "SINGLE"  | "DOUBLE"
//         .
// -------------------------------------------------------------------------
auto Parser::builtinType() -> Result<AstBuiltInType*> {
    const auto start = startLoc();
    if (not m_token.kind().isType()) {
        return expected("type");
    }
    const auto kind = m_token.kind();
    TRY(advance());
    return make<AstBuiltInType>(range(start), kind);
}
