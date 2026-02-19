//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Parser.hpp"
using namespace lbc;

// type = builtin { "PTR" | "REF" } .
auto Parser::type() -> Result<AstType*> {
    AstType* ty {}; // NOLINT(*-const-correctness)
    TRY_ASSIGN(ty, builtinType())
    while (true) {
        TRY_IF (accept(TokenKind::Ptr)) {
            ty = make<AstPointerType>(range(ty), ty);
            continue;
        }
        TRY_IF (accept(TokenKind::Ref)) {
            ty = make<AstReferenceType>(range(ty), ty);
            continue;
        }
        break;
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
