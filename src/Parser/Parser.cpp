//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Parser.hpp"
#include "Ast/Ast.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Token.hpp"
#include "Symbol/SymbolTable.hpp"
#include <llvm/ADT/TypeSwitch.h>
using namespace lbc;
using namespace flags::operators;

namespace {
auto getStart(const AstAttributeList* attribs, const Token& token) -> llvm::SMLoc {
    if (attribs == nullptr) {
        return token.getRange().Start;
    }
    return attribs->range.Start;
}
} // namespace

Parser::Parser(Context& context, Lexer& lexer, const bool isMain, SymbolTable* symbolTable)
: ErrorLogger(context.getDiag())
, m_context { context }
, m_lexer { lexer }
, m_isMain { isMain }
, m_symbolTable { symbolTable }
, m_language { CallingConv::Default }
, m_scope { Scope::Root } {
    advance();
}

void Parser::reset() {
    m_scope = Scope::Root;
    m_exprFlags = {};
    m_endLoc = {};
    m_token = {};
    m_controlStack.clear();
    advance();
    m_endLoc = m_token.getRange().Start;
}

/**
 * Parse given input and return AST module.
 *
 * Grammar:
 * Module = stmtList
 *        .
 */
auto Parser::parse() -> Result<AstModule*> {
    TRY_DECL(stmts, stmtList())

    return m_context.create<AstModule>(
        m_lexer.getFileId(),
        stmts->range,
        m_isMain,
        std::move(m_imports),
        stmts
    );
}

//----------------------------------------
// Statements
//----------------------------------------

/**
 * Parse list of statements.
 *
 * Each statement represents a "sentence" of code, be it a function call, declaration, control flow, etc.
 *
 * Statements are separated by a newline or a colon.
 *
 * Grammar:
 * stmtList = { statement <EndOfStmt> } .
 */
auto Parser::stmtList() -> Result<AstStmtList*> {
    constexpr auto isNonTerminator = [](const Token& token) {
        switch (token.getKind()) {
        case TokenKind::End:
        case TokenKind::Else:
        case TokenKind::Next:
        case TokenKind::Loop:
        case TokenKind::EndOfFile:
            return false;
        default:
            return true;
        }
    };

    const auto start = m_token.getRange().Start;
    std::vector<AstDecl*> decls;
    std::vector<AstFuncStmt*> funcs;
    std::vector<AstStmt*> stms;

    while (isNonTerminator(m_token)) {
        TRY_DECL(stmt, statement())

        llvm::TypeSwitch<AstStmt*>(stmt)
            .Case([&](AstFuncStmt* node) {
                funcs.emplace_back(node);
                decls.emplace_back(node->decl);
            })
            .Case([&](AstDecl* node) {
                decls.emplace_back(node);
                stms.emplace_back(node);
            })
            .Case([&](AstImport* import) {
                m_imports.emplace_back(import);
            })
            .Case([&](const AstExtern* ext) {
                for (auto* eStmt : ext->stmts) {
                    if (auto* decl = llvm::dyn_cast<AstDecl>(eStmt)) {
                        decl->local = false;
                        decls.emplace_back(decl);
                        stms.emplace_back(decl);
                    } else if (auto* func = llvm::dyn_cast<AstFuncStmt>(eStmt)) {
                        decls.emplace_back(func->decl);
                        funcs.emplace_back(func);
                    } else {
                        fatalError("Unknown declaration");
                    }
                }
            })
            .Default([&](auto* node) {
                stms.emplace_back(node);
            });

        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstStmtList>(
        llvm::SMRange { start, m_endLoc },
        std::move(decls),
        std::move(funcs),
        std::move(stms)
    );
}

/**
 * Parse single statement.
 *
 * Grammar:
 * statement = declaration
 *           | "IMPORT"   kwImport
 *           | "EXTERN"   kwExtern
 *           | "RETURN"   kwReturn
 *           | "ID"       kwIf
 *           | "FOR"      kwFor
 *           | "DO"       kwDo
 *           | "CONTINUE" kwContinue
 *           | "EXIT"     kwExit
 *           | expression
 *           .
 */
auto Parser::statement() -> Result<AstStmt*> {
    TRY_DECL(decl, declaration(true))
    if (decl != nullptr) {
        return decl;
    }

    if (m_scope == Scope::Root) {
        if (m_token.is(TokenKind::Import)) {
            return kwImport();
        }
        if (m_token.is(TokenKind::Extern)) {
            return kwExtern();
        }
        if (!m_isMain) {
            return makeError(Diag::notAllowedTopLevelStatement, m_token);
        }
    }

    switch (m_token.getKind()) {
    case TokenKind::Return:
        return kwReturn();
    case TokenKind::If:
        return kwIf();
    case TokenKind::For:
        return kwFor();
    case TokenKind::Do:
        return kwDo();
    case TokenKind::Continue:
        return kwContinue();
    case TokenKind::Exit:
        return kwExit();
    default:
        break;
    }

    TRY_DECL(expr, expression(ExprFlags::useAssign | ExprFlags::callWithoutParens))
    // TODO: detect a dangling expressions with no side effects and raise warning
    return m_context.create<AstExprStmt>(expr->range, expr);
}

/**
 * Parse import keyword.
 *
 * Import keyword is used to include another module into the current one.
 *
 * Grammar:
 * kwImport = "IMPORT" <id> .
 */
auto Parser::kwImport() -> Result<AstImport*> {
    // assume m_token == Import
    advance();

    TRY(expect(TokenKind::Identifier))

    auto import = m_token.lexeme();
    const auto range = m_token.getRange();
    advance();

    // Imported file
    const auto source = m_context.getOptions().getCompilerDir() / "lib" / (import + ".bas").str();
    if (!m_context.import(source.string())) {
        return m_context.create<AstImport>(llvm::SMRange { range.Start, m_endLoc }, import);
    }
    if (!fs::exists(source)) {
        getDiag().log(Diag::moduleNotFound, range.Start, range, import);
        return ResultError {};
    }

    // Load import into Source Mgr
    std::string included;
    const auto ID = m_context.getSourceMrg().AddIncludeFile(
        source.string(),
        range.Start,
        included
    );
    if (ID == ~0U) {
        getDiag().log(Diag::failedToLoadModule, range.Start, range, source.string());
        return ResultError {};
    }

    // parse the module
    Lexer lexer { m_context, ID };
    TRY_DECL(module, Parser(m_context, lexer, false).parse())

    return m_context.create<AstImport>(
        llvm::SMRange { range.Start, m_endLoc },
        import,
        module
    );
}

/**
 * Parse EXTERN keyword.
 *
 * Extern keyword is used to declare external functions. For example from a C library
 *
 * Grammar:
 * kwExtern        = [ ExternLanguage ] ( ExternStatement | ExternBlock ) .
 * ExternLanguage  = "C" | "DEFAULT" .
 * ExternStatement = declaration .
 * ExternBlock     = <EndOfStmt> { declaration <EndOfStmt> } "END" "EXTERN" .
 */
auto Parser::kwExtern() -> Result<AstExtern*> {
    // assume m_token == Extern
    const auto start = m_token.getRange().Start;
    advance();
    RESTORE_ON_EXIT(m_language);

    if (m_token.is(TokenKind::StringLiteral)) {
        if (auto str = m_token.getStringValue().upper(); str == "C") {
            m_language = CallingConv::C;
        } else if (str == "DEFAULT") {
            m_language = CallingConv::Default;
        } else {
            return makeError(Diag::unsupportedExternLanguage, m_token, str);
        }
        advance();
    }

    std::vector<AstStmt*> stmts {};

    if (accept(TokenKind::EndOfStmt)) {
        while (m_token.isNot(TokenKind::End)) {
            TRY_DECL(decl, declaration(false))
            if (decl == nullptr) {
                return makeError(Diag::onlyDeclarationsInExtern, m_token);
            }
            stmts.emplace_back(decl);
            TRY(consume(TokenKind::EndOfStmt))
        }
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::Extern))
    } else {
        TRY_DECL(decl, declaration(false))
        stmts.emplace_back(decl);
    }

    return m_context.create<AstExtern>(
        llvm::SMRange { start, m_endLoc },
        m_language,
        std::move(stmts)
    );
}

/**
 * Parse declaration statement.
 *
 * Declaration is anything that defines a new symbol. This includes variables, constants, functions, etc.
 * Declarations can include optional attributes.
 *
 * Grammar:
 * Declaration = [ attributeList ]
 *             ( "DIM"      kwDim
 *             | "CONST"    kwConst
 *             | "DECLARE"  kwDeclare
 *             | "FUNCTION" kwFunction
 *             | "SUB"      kwFunction
 *             | "TYPE"     kwType
 *             ) .
 * @param optional if true then declaration is optional, otherwise raise an error if no declaration found.
 */
auto Parser::declaration(const bool optional) -> Result<AstStmt*> {
    TRY_DECL(attribs, attributeList())

    switch (m_token.getKind()) {
    case TokenKind::Dim:
        return kwDim(attribs);
    case TokenKind::Const:
        return kwConst(attribs);
    case TokenKind::Declare:
        return kwDeclare(attribs);
    case TokenKind::Function:
    case TokenKind::Sub:
        return kwFunction(attribs);
    case TokenKind::Type:
        return kwType(attribs);
    default:
        if (!optional || attribs != nullptr) {
            return makeError(Diag::expectedDeclration, m_token, m_token.description());
        }
        return nullptr;
    }
}

//----------------------------------------
// Attributes
//----------------------------------------

/**
 * Parse attributes which are used to annotate declarations.
 *
 * Grammar:
 * attributeList = [ "[" attribute { "," attribute } "]" ] .
 */
auto Parser::attributeList() -> Result<AstAttributeList*> {
    if (m_token.isNot(TokenKind::BracketOpen)) {
        return nullptr;
    }

    const auto start = m_token.getRange().Start;
    advance();

    std::vector<AstAttribute*> attribs;

    while (true) {
        TRY_DECL(attrib, attribute())
        attribs.emplace_back(attrib);
        if (!accept(TokenKind::Comma)) {
            break;
        }
    }

    TRY(consume(TokenKind::BracketClose))

    return m_context.create<AstAttributeList>(
        llvm::SMRange { start, m_endLoc },
        std::move(attribs)
    );
}

/**
 * Parse individual attribute
 *
 * Grammar:
 * attribute  = identifier [ attributeArgList ] .
 */
auto Parser::attribute() -> Result<AstAttribute*> {
    const auto start = m_token.getRange().Start;

    TRY_DECL(id, identifier())

    AstExprList* args = nullptr;
    TRY_ASSIGN(args, attributeArgList())

    return m_context.create<AstAttribute>(
        llvm::SMRange { start, m_endLoc },
        id,
        args
    );
}

/**
 * Parse attribute argument list.
 *
 * Grammar:
 * attributeArgList = [ AttrValue | AttrArgs ] .
 * AttrValue        = "=" literal .
 * AttrArgs         = "(" [ literal { "," literal } ] ")" .
 */
auto Parser::attributeArgList() -> Result<AstExprList*> {
    const auto start = m_token.getRange().Start;
    std::vector<AstExpr*> args;

    if (accept(TokenKind::Assign)) {
        TRY_DECL(lit, literal())
        args.emplace_back(lit);
    } else if (accept(TokenKind::ParenOpen)) {
        while (m_token.isLiteral()) {
            TRY_DECL(lit, literal())
            args.emplace_back(lit);
            if (!accept(TokenKind::Comma)) {
                break;
            }
        }
        TRY(consume(TokenKind::ParenClose))
    }

    if (args.empty()) {
        return nullptr;
    }

    return m_context.create<AstExprList>(
        llvm::SMRange { start, m_endLoc },
        std::move(args)
    );
}

//----------------------------------------
// DIM
//----------------------------------------

/**
 * Declares a variable.
 *
 * Variable type can either be explicitly defined or inferred from the expression.
 * If variable is not initialized then its value is undefined.
 *
 * Grammar:
 * kwDim = "DIM" <id>
 *       ( "=" expression
 *       | "AS" typeExpr [ "=" expression ]
 *       )
 *       .
 *
 * @param attribs optional attributes
 */
auto Parser::kwDim(AstAttributeList* attribs) -> Result<AstVarDecl*> {
    // assume m_token == VAR
    const auto start = getStart(attribs, m_token);
    advance();

    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    AstTypeExpr* type = nullptr;
    AstExpr* expr = nullptr;

    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())

        if (accept(TokenKind::Assign)) {
            TRY_ASSIGN(expr, expression())
        }
    } else {
        TRY(consume(TokenKind::Assign))
        TRY_ASSIGN(expr, expression())
    }

    return m_context.create<AstVarDecl>(
        llvm::SMRange { start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        expr,
        false
    );
}

//----------------------------------------
// CONST
//----------------------------------------

/**
 * Declares a constant. Difference from variables declared with DIM are:
 * - must always be initialized
 * - must be known at compile time
 * - cannot be changed later
 *
 * Grammar:
 * kwConst = "CONST" <id> [ "AS" typeExpr ] "=" expression .
 *
 * @param attribs optional attributes
 */
auto Parser::kwConst(AstAttributeList* attribs) -> Result<AstVarDecl*> {
    // assume m_token == CONST
    const auto start = getStart(attribs, m_token);
    advance();

    // identifier
    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    // [ "AS" TypeExpr ]
    AstTypeExpr* type = nullptr;
    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())
    }

    // "=" Expression
    TRY(consume(TokenKind::Assign))
    TRY_DECL(expr, expression())

    return m_context.create<AstVarDecl>(
        llvm::SMRange { start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        expr,
        true
    );
}

//----------------------------------------
// DECLARE
//----------------------------------------

/**
 * Declares a sub or a function.
 *
 * This is used to declare external functions that are implemented in another module
 *
 * Grammar:
 * kwDeclare = "DECLARE" procSignature .
 *
 * @param attribs optional attributes
 */
auto Parser::kwDeclare(AstAttributeList* attribs) -> Result<AstFuncDecl*> {
    // assume m_token == DECLARE
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token, m_token.description());
    }
    const auto start = getStart(attribs, m_token);
    advance();

    return procSignature(start, attribs, FuncFlags::isDeclaration);
}

/**
 * Parse sub or function signature.
 *
 * Difference between SUB and FUNCTION is that FUNCTION must have a return type.
 *
 * Grammar:
 * procSignature = SubSignature | FuncSignature .
 * SubSignature  = "SUB" <id> [ "(" funcParamList ")" ] .
 * FuncSignature = "FUNCTION" <id> [ "(" funcParamList ")" ] "AS" typeExpr
 *
 * @param start start location of the declaration
 * @param attribs optional attributes
 * @param funcFlags flags to control the parsing behavior
 */
auto Parser::procSignature(const llvm::SMLoc start, AstAttributeList* attribs, const FuncFlags funcFlags) -> Result<AstFuncDecl*> {
    bool const isFunc = accept(TokenKind::Function);
    if (!isFunc) {
        TRY(consume(TokenKind::Sub))
    }

    llvm::StringRef id;
    Token token;
    if (flags::has(funcFlags, FuncFlags::isAnonymous)) {
        id = "";
    } else {
        TRY(expect(TokenKind::Identifier))
        id = m_token.getStringValue();
        token = m_token;
        advance();
    }

    bool isVariadic = false;
    AstFuncParamList* params = nullptr;
    if (accept(TokenKind::ParenOpen)) {
        TRY_ASSIGN(params, funcParamList(isVariadic, flags::has(funcFlags, FuncFlags::isAnonymous)))
        TRY(consume(TokenKind::ParenClose))
    }

    AstTypeExpr* ret = nullptr;
    if (isFunc) {
        TRY(consume(TokenKind::As))
        TRY_ASSIGN(ret, typeExpr())
    }

    return m_context.create<AstFuncDecl>(
        llvm::SMRange { start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        params,
        isVariadic,
        ret,
        !flags::has(funcFlags, FuncFlags::isDeclaration)
    );
}

/**
 * Parse function parameter list.
 *
 * When anonymous function is declared then parameters are not named.
 * Parameters can be variadic, in which case they must be the last parameter.
 *
 * Grammar:
 * funcParamList = funcParam { "," funcParam } [ "," "..." ] | "..." .
 *
 * @param isVariadic out parameter to indicate if the function is variadic
 * @param isAnonymous true if the function is anonymous
 */
auto Parser::funcParamList(bool& isVariadic, const bool isAnonymous) -> Result<AstFuncParamList*> {
    const auto start = m_token.getRange().Start;
    std::vector<AstFuncParamDecl*> params;
    while (!m_token.is(TokenKind::ParenClose)) {
        if (accept(TokenKind::Ellipsis)) {
            isVariadic = true;
            if (m_token.is(TokenKind::Comma)) {
                return makeError(Diag::variadicArgumentNotLast, m_token);
            }
            break;
        }

        TRY_DECL(param, funcParam(isAnonymous))
        params.push_back(param);

        if (!accept(TokenKind::Comma)) {
            break;
        }
    }

    return m_context.create<AstFuncParamList>(
        llvm::SMRange { start, m_endLoc },
        std::move(params)
    );
}

/**
 * Parse individual function parameter.
 *
 * If the function is anonymous then the parameter is not named.
 *
 * Grammar:
 * funcParam = <id> "AS" typeExpr | typeExpr .
 *
 * @param isAnonymous true if the function is anonymous
 */
auto Parser::funcParam(const bool isAnonymous) -> Result<AstFuncParamDecl*> {
    const auto start = m_token.getRange().Start;

    llvm::StringRef id;
    Token token;
    if (isAnonymous) {
        if (m_token.is(TokenKind::Identifier)) {
            token = m_token;
            id = token.getStringValue();
            TRY(consume(TokenKind::As))
        } else {
            id = "";
        }
    } else {
        TRY(expect(TokenKind::Identifier))
        token = m_token;
        id = token.getStringValue();
        advance();
        TRY(consume(TokenKind::As))
    }

    TRY_DECL(type, typeExpr())

    return m_context.create<AstFuncParamDecl>(
        llvm::SMRange { start, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type
    );
}

//----------------------------------------
// TYPE
//----------------------------------------

/**
 * Parse a type declaration
 *
 * This can either be a user defined type or a type alias.
 *
 * Grammar:
 * kwType = "TYPE" <id> ( <EndOfStmt> udt | "AS" alias ) .
 *
 * @param attribs optional attributes
 */
auto Parser::kwType(AstAttributeList* attribs) -> Result<AstDecl*> {
    // assume m_token == TYPE
    const auto start = m_token.getRange().Start;
    advance();

    TRY(expect(TokenKind::Identifier))
    const auto token = m_token;
    advance();

    if (accept(TokenKind::EndOfStmt)) {
        return udt(token, start, attribs);
    }

    if (accept(TokenKind::As)) {
        return alias(token, start, attribs);
    }

    return makeError(Diag::unexpectedToken, m_token, "'=' or end of statement", m_token.description());
}

/**
 * Parse a type alias.
 *
 * Grammar:
 * alias = typeExpr .
 *
 * @param token token of the alias, which also includes the name and position
 * @param start start location of the declaration
 * @param attribs optional attributes
 */
auto Parser::alias(Token token, const llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstTypeAlias*> {
    TRY_DECL(type, typeExpr())

    return m_context.create<AstTypeAlias>(
        llvm::SMRange { start, m_endLoc },
        token.getStringValue(),
        token,
        m_language,
        attribs,
        type
    );
}

/**
 * Parse a user defined type.
 *
 * Grammar:
 * udt = udtDeclList "END" "TYPE" .
 *
 * @param token token of the UDT, which also includes the name and position
 * @param start start location of the declaration
 * @param attribs optional attributes
 */
auto Parser::udt(Token token, const llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstUdtDecl*> {
    // assume m_token == declaration || "end"
    TRY_DECL(decls, udtDeclList())

    TRY(consume(TokenKind::End))
    TRY(consume(TokenKind::Type))

    return m_context.create<AstUdtDecl>(
        llvm::SMRange { start, m_endLoc },
        token.getStringValue(),
        token,
        m_language,
        attribs,
        decls
    );
}

/**
 * udtDeclList
 *   = { [ AttributeList ] udtMember EoS }
 *   .
 */
auto Parser::udtDeclList() -> Result<AstDeclList*> {
    const auto start = m_token.getRange().Start;
    std::vector<AstDecl*> decls;

    while (true) {
        TRY_DECL(attribs, attributeList())

        if (attribs != nullptr) {
            TRY(expect(TokenKind::Identifier))
        } else if (m_token.isNot(TokenKind::Identifier)) {
            break;
        }

        TRY_DECL(member, udtMember(attribs))

        decls.emplace_back(member);
        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstDeclList>(
        llvm::SMRange { start, m_endLoc },
        std::move(decls)
    );
}

/**
 * udtMember
 *   = id "AS" TypeExpr
 *   .
 */
auto Parser::udtMember(AstAttributeList* attribs) -> Result<AstDecl*> {
    // assume m_token == Identifier
    const auto start = m_token.getRange().Start;
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    TRY(consume(TokenKind::As))
    TRY_DECL(type, typeExpr())

    return m_context.create<AstVarDecl>(
        llvm::SMRange { start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        nullptr,
        false
    );
}

//----------------------------------------
// Call
//----------------------------------------

/**
 *  FUNCTION = funcSignature <EoS>
 *             stmtList
 *             "END" ("FUNCTION" | "SUB")
 */
auto Parser::kwFunction(AstAttributeList* attribs) -> Result<AstFuncStmt*> {
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token, m_token.description());
    }

    bool const isFunction = m_token.is(TokenKind::Function);
    const auto start = getStart(attribs, m_token);
    TRY_DECL(decl, procSignature(start, attribs, {}))

    RESTORE_ON_EXIT(m_scope);
    m_scope = Scope::Function;
    AstStmtList* stmts = nullptr;

    if (accept(TokenKind::LambdaBody)) {
        AstStmt* stmt = nullptr;
        if (isFunction) {
            TRY_DECL(expr, expression())
            stmt = m_context.create<AstReturnStmt>(
                llvm::SMRange { start, m_endLoc },
                expr
            );
        } else {
            TRY_ASSIGN(stmt, statement())
        }
        stmts = m_context.create<AstStmtList>(
            llvm::SMRange { start, m_endLoc },
            std::vector<AstDecl*> {}, // TODO: Fix. stmt could be a declaration!
            std::vector<AstFuncStmt*> {},
            std::vector { stmt }
        );
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        TRY_ASSIGN(stmts, stmtList())

        TRY(consume(TokenKind::End))
        if (isFunction) {
            TRY(consume(TokenKind::Function))
        } else {
            TRY(consume(TokenKind::Sub))
        }
    }

    return m_context.create<AstFuncStmt>(
        llvm::SMRange { start, m_endLoc },
        decl,
        stmts
    );
}

/**
 * RETURN = "RETURN" [ expression ] .
 */
auto Parser::kwReturn() -> Result<AstStmt*> {
    // assume m_token == RETURN
    if (m_scope == Scope::Root && !m_isMain) {
        return makeError(Diag::unexpectedReturn, m_token);
    }
    const auto start = m_token.getRange().Start;
    advance();

    AstExpr* expr = nullptr;
    if (m_token.isNot(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(expr, expression())
    }

    return m_context.create<AstReturnStmt>(
        llvm::SMRange { start, m_endLoc },
        expr
    );
}

//----------------------------------------
// If statement
//----------------------------------------

/**
 * IF
 *   = IfBlock
 *   { ELSE IF IfBlock }
 *   [ ELSE ThenBlock ]
 *   "END" "IF"
 *   .
 */
auto Parser::kwIf() -> Result<AstIfStmt*> {
    // assume m_token == IF
    const auto start = m_token.getRange().Start;
    advance();

    std::vector<AstIfStmtBlock*> blocks;
    TRY_DECL(block, ifBlock())

    blocks.emplace_back(block);

    if (m_token.is(TokenKind::EndOfStmt)) {
        acceptNext(TokenKind::Else);
    }

    while (accept(TokenKind::Else)) {
        if (accept(TokenKind::If)) {
            TRY_DECL(if_, ifBlock())
            blocks.emplace_back(if_);
        } else {
            TRY_DECL(else_, thenBlock({}, nullptr))
            blocks.emplace_back(else_);
        }

        if (m_token.is(TokenKind::EndOfStmt)) {
            acceptNext(TokenKind::Else);
        }
    }

    if (blocks.back()->stmt->kind == AstKind::StmtList) {
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::If))
    }
    return m_context.create<AstIfStmt>(
        llvm::SMRange { start, m_endLoc },
        std::move(blocks)
    );
}

/**
 * IfBlock
 *   = [ VAR { "," VAR } "," ] Expression "THEN" ThenBlock
 *   .
 */
auto Parser::ifBlock() -> Result<AstIfStmtBlock*> {
    std::vector<AstVarDecl*> decls;
    while (m_token.is(TokenKind::Dim)) {
        TRY_DECL(var, kwDim(nullptr))

        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }
    TRY_DECL(expr, expression(ExprFlags::defaultSequence))
    TRY(consume(TokenKind::Then))
    return thenBlock(std::move(decls), expr);
}

/**
 * ThenBlock
 *   =
 *   ( EoS StmtList
 *   | Statement
 *   )
 *   .
 */
auto Parser::thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr) -> Result<AstIfStmtBlock*> {
    AstStmt* stmt = nullptr;
    if (accept(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(stmt, stmtList())
    } else {
        TRY_ASSIGN(stmt, statement())
    }

    return m_context.create<AstIfStmtBlock>(
        std::move(decls),
        nullptr,
        expr,
        stmt
    );
}

//----------------------------------------
// FOR statement
//----------------------------------------

/**
 * FOR
 *   = "FOR" [ VAR { "," VAR } "," ]
 *     id [ "AS" TypeExpr ] "=" Expression "TO" Expression [ "STEP" expression ]
 *   ( "=>" Statement
 *   | <EoS> StatementList
 *     "NEXT" [ id ]
 *   )
 *   .
 */
auto Parser::kwFor() -> Result<AstForStmt*> {
    // assume m_token == FOR
    const auto start = m_token.getRange().Start;
    advance();

    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } "," ]
    while (m_token.is(TokenKind::Dim)) {
        TRY_DECL(var, kwDim(nullptr))

        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }

    // id [ "AS" TypeExpr ] "=" Expression
    const auto idStart = m_token.getRange().Start;
    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    AstTypeExpr* type = nullptr;
    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())
    }

    TRY(consume(TokenKind::Assign))
    TRY_DECL(expr, expression())

    auto* iterator = m_context.create<AstVarDecl>(
        llvm::SMRange { idStart, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type,
        expr,
        false
    );

    // "TO" Expression [ "STEP" expression ]
    TRY(consume(TokenKind::To))
    TRY_DECL(limit, expression())

    AstExpr* step = nullptr;
    if (accept(TokenKind::Step)) {
        TRY_ASSIGN(step, expression())
    }

    // "=>" statement ?
    AstStmt* stmt = nullptr;
    llvm::StringRef next;
    if (accept(TokenKind::LambdaBody)) {
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::For, [&] {
            return statement();
        }))
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::For, [&] {
            return stmtList();
        }))

        TRY(consume(TokenKind::Next))

        if (m_token.is(TokenKind::Identifier)) {
            next = m_token.getStringValue();
            advance();
        }
    }

    return m_context.create<AstForStmt>(
        llvm::SMRange { start, m_endLoc },
        std::move(decls),
        iterator,
        limit,
        step,
        stmt,
        next
    );
}

//----------------------------------------
// DO ... LOOP statement
//----------------------------------------

/**
 * DO = "DO" [ VAR { "," VAR } ]
 *    ( EndOfStmt StmtList "LOOP" [ LoopCondition ]
 *    | [ LoopCondition ] ( EoS StmtList "LOOP" | "=>" Statement )
 *    )
 *    .
 * LoopCondition
 *   = ("UNTIL" | "WHILE") expression
 *   .
 */
auto Parser::kwDo() -> Result<AstDoLoopStmt*> {
    // assume m_token == DO
    const auto start = m_token.getRange().Start;
    advance();

    auto condition = AstDoLoopStmt::Condition::None;
    AstStmt* stmt = nullptr;
    AstExpr* expr = nullptr;
    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } ]
    auto canHaveComma = false;
    while (m_token.is(TokenKind::Dim) || (canHaveComma && accept(TokenKind::Comma))) {
        canHaveComma = true;
        TRY_DECL(var, kwDim(nullptr))
        decls.emplace_back(var);
    }

    // ( EoS StmtList "LOOP" [ Condition ]
    if (accept(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
            return stmtList();
        }))

        TRY(consume(TokenKind::Loop))

        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PostUntil;
            TRY_ASSIGN(expr, expression(ExprFlags::defaultSequence))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PostWhile;
            TRY_ASSIGN(expr, expression(ExprFlags::defaultSequence))
        }
    } else {
        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PreUntil;
            TRY_ASSIGN(expr, expression(ExprFlags::defaultSequence))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PreWhile;
            TRY_ASSIGN(expr, expression(ExprFlags::defaultSequence))
        }

        // EoS StmtList "LOOP"
        if (accept(TokenKind::EndOfStmt)) {
            TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
                return stmtList();
            }))
            TRY(consume(TokenKind::Loop))
        }
        // "=>" Statement
        else {
            TRY(consume(TokenKind::LambdaBody))
            TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
                return statement();
            }))
        }
    }

    return m_context.create<AstDoLoopStmt>(
        llvm::SMRange { start, m_endLoc },
        std::move(decls),
        condition,
        expr,
        stmt
    );
}

//----------------------------------------
// Branching
//----------------------------------------

/**
 * CONTINUE
 *   = "CONTINUE" { "FOR" | "DO" }
 *   .
 */
auto Parser::kwContinue() -> Result<AstContinuationStmt*> {
    return continuation(AstContinuationAction::Continue);
}

/**
 * EXIT
 *   = "EXIT" { "FOR" }
 *   .
 */
auto Parser::kwExit() -> Result<AstContinuationStmt*> {
    return continuation(AstContinuationAction::Exit);
}

auto Parser::continuation(AstContinuationAction action) -> Result<AstContinuationStmt*> {
    // assume m_token == CONTINUE | EXIT
    const auto start = m_token.getRange().Start;
    auto control = m_token.description();

    if (m_controlStack.empty()) {
        return makeError(Diag::unexpectedContinuation, m_token, control);
    }

    advance();

    auto iter = m_controlStack.begin();
    auto index = m_controlStack.indexOf(iter);
    const auto target = [&](const ControlFlowStatement look) -> Result<void> {
        iter = m_controlStack.find(iter, look);
        if (iter == m_controlStack.end()) {
            return makeError(Diag::unexpectedContinuationTarget, m_token, control, m_token.description());
        }
        index = m_controlStack.indexOf(iter);
        ++iter;
        advance();
        return {};
    };

    while (true) {
        switch (m_token.getKind()) {
        case TokenKind::For:
            TRY(target(ControlFlowStatement::For))
            continue;
        case TokenKind::Do:
            TRY(target(ControlFlowStatement::Do))
            continue;
        default:
            break;
        }
        break;
    }

    return m_context.create<AstContinuationStmt>(
        llvm::SMRange { start, m_endLoc },
        action,
        index
    );
}

//----------------------------------------
// Types
//----------------------------------------

/**
 * TypeExpr = ( BuiltInType | "ANY" | identExpr ) { "PTR" }
 *          | SubSignature "PTR" { "PTR" }
 *          | "(" ProcSignature ")" "PTR" { "PTR" }
 *          | TypeOf
 *          .
 */
auto Parser::typeExpr() -> Result<AstTypeExpr*> {
    TypeParsingContext context {};
    return basicTypeExpr(context);
}

/**
 * Parse type expression.
 *
 * TypeExpr = ( BuiltInType | "ANY" | identExpr ) { "PTR" }
 *          | SubSignature "PTR" { "PTR" }
 *          | "(" ProcSignature ")" "PTR" { "PTR" }
 *          | TypeOf
 *          .
 * @param context provide context for parsing behaviour
 * @return a nullptr, an error or a type expression depending on context
 */
[[nodiscard]] auto Parser::basicTypeExpr(TypeParsingContext& context) -> Result<AstTypeExpr*> {
    const auto start = m_token.getRange().Start;

    // Resulting type expression
    AstTypeExpr::TypeExpr expr;

    if (m_token.isTypeKeyword() || m_token.is(TokenKind::Any)) {
        expr = m_token.getKind();
        advance();
    } else if (m_token.is(TokenKind::Sub)) {
        TRY_ASSIGN(expr, parseTypeProcedure(false, context))
    } else if (m_token.is(TokenKind::ParenOpen)) {
        TRY_ASSIGN(expr, parseTypeProcedure(true, context))
    } else if (m_token.is(TokenKind::TypeOf)) {
        TRY_ASSIGN(expr, kwTypeOf())
    } else if (m_token.is(TokenKind::Identifier)) {
        TRY_DECL(ident, parseTypeIdentifier(context));
        if (ident == nullptr) {
            return nullptr;
        }
        expr = ident;
    } else {
        return handleIncompleteTypeExpr<AstTypeExpr>(context);
    }

    // handle trailing ptr keywords
    context.deref += parseDereferences();

    // done
    return m_context.create<AstTypeExpr>(
        llvm::SMRange { start, m_endLoc },
        expr,
        context.deref
    );
}

/**
 * Parse identifier inside type expression
 *
 * @param context provide context for parsing behaviour
 * @return an identifier or an error state
 */
[[nodiscard]] auto Parser::parseTypeIdentifier(TypeParsingContext& context) -> Result<AstIdentExpr*> {
    auto* ident = identifier(m_token);
    advance();

    // If we have a symbol table, we can query for the ID.
    if (m_symbolTable != nullptr) {
        auto* symbol = m_symbolTable->find(ident->name);
        if (symbol == nullptr || symbol->valueFlags().kind != ValueFlags::Kind::type) {
            return ResultError {}; // NOTE: Semantic analyser handles the error logging.
        }
        ident->symbol = symbol;
    }

    // If next token is something that can either further define type or
    // terminate type expression, then consider it valid
    switch (m_token.getKind()) {
    case TokenKind::Ptr:
        context.deref += 1;
        advance();
        break;
    case TokenKind::ParenClose:
    case TokenKind::Comma:
    case TokenKind::EndOfStmt:
    case TokenKind::EndOfFile:
    case TokenKind::BracketClose:
    case TokenKind::LambdaBody:
    case TokenKind::Assign:
        // If we are dealing with a single identifier token, and we have an out ptr,
        // set that and return null
        if (context.allowIncompleteType) {
            context.identifier = ident;
            return nullptr;
        }
        break;
    default:
        return handleIncompleteTypeExpr<AstIdentExpr>(context);
    }
    return ident;
}

/**
 * Parse SUB or FUNCTION inside type expression
 *
 * @param enclosed indicate if the type is enclosed in parentheses
 * @param context provide context for parsing behaviour
 * @return error state or a func declaration
 */
[[nodiscard]] auto Parser::parseTypeProcedure(const bool enclosed, TypeParsingContext& context) -> Result<AstFuncDecl*> {
    const auto start = m_token.getRange().Start;

    if (enclosed) {
        // assume there is '('
        advance();
    }

    TRY_DECL(func, procSignature(start, nullptr, FuncFlags::isAnonymous))

    if (enclosed) {
        // ')'
        TRY(consume(TokenKind::ParenClose));
    }

    // Expect a PTR keyword
    if (m_token.getKind() != TokenKind::Ptr) {
        return makeError(
            Diag::procTypesMustHaveAPtr,
            m_token.getRange().Start,
            { start, m_endLoc },
            func->isFunction() ? "FUNCTION" : "SUB"
        );
    }

    context.deref += 1;
    advance();

    return func;
}

/**
 * Parse consecutive pointer dereferences in type expressions.
 *
 * dim x as integer ptr ptr ptr ptr = null
 *
 * @return the number of dereferences.
 */
auto Parser::parseDereferences() -> int {
    int deref = 0;
    while (accept(TokenKind::Ptr)) {
        deref++;
    }
    return deref;
}

/**
 * Return nullptr or an error depending on the value of allowIncompleteTypeExpr.
 */
template <typename T>
[[nodiscard]] auto Parser::handleIncompleteTypeExpr(const TypeParsingContext& context) const -> Result<T*> {
    if (context.allowIncompleteType) {
        return nullptr;
    }
    return makeError(Diag::expectedTypeExpression, m_token, m_token.lexeme());
}

/**
 * TypeOf = "TYPEOF" "(" (Expr | TypeExpr) ")"
 *        .
 */
auto Parser::kwTypeOf() -> Result<AstTypeOf*> {
    // assume m_token == "TYPEOF" | "SIZEOF" | "ALIGNOF"
    const auto start = m_token.getRange().Start;
    advance();

    // "("
    TRY(consume(TokenKind::ParenOpen))

    AstTypeOf::TypeExpr expr = m_token.getRange().Start;

    TypeParsingContext context { .allowIncompleteType = true };
    TRY_DECL(basic, basicTypeExpr(context))

    if (basic != nullptr) {
        expr = basic;
        TRY(consume(TokenKind::ParenClose))
    } else if (context.identifier != nullptr) {
        expr = context.identifier;
        TRY(consume(TokenKind::ParenClose))
    } else {
        TRY(skipUntilMatchingClosingParen(1, "type expression"))
    }

    return m_context.create<AstTypeOf>(llvm::SMRange { start, m_endLoc }, expr);
}

//----------------------------------------
// Expressions
//----------------------------------------

/**
 * expression = primary { <Unary op> | <Binary Op> expression }
 *            . [ ArgumentList ]
 */
auto Parser::expression(const ExprFlags flags) -> Result<AstExpr*> {
    RESTORE_ON_EXIT(m_exprFlags);
    m_exprFlags = flags;

    TRY_DECL(expr, primary())

    // This enables following syntax
    //   print "message"
    // without the need of parentheses
    //
    // There is some ambiguity however:
    //   "print -foo"  is seen as "(print - foo)" and not as "print (-foo)"
    // However, it is how BASIC is expected to work...
    if (flags::has(flags, ExprFlags::callWithoutParens) && expr->kind == AstKind::IdentExpr) {
        // if next token is a binary or a right to left operator, then we can assume it is not a sub call
        if (m_token.isBinary() || m_token.isRightToLeft()) {
            return expression(expr, 1);
        }

        const auto start = expr->range.Start;
        TRY_DECL(args, expressionList())

        return m_context.create<AstCallExpr>(
            llvm::SMRange { start, m_endLoc },
            expr,
            args
        );
    }

    // Normal expression
    return expression(expr, 1);
}

/**
 * Recursively climb operator precedence
 * https://en.wikipedia.org/wiki/Operator-precedence_parser#Precedence_climbing_method
 */
auto Parser::expression(AstExpr* lhs, const int precedence) -> Result<AstExpr*> {
    while (m_token.getPrecedence() >= precedence) {
        const auto op = m_token;

        if (m_token.isUnary()) {
            advance();
            TRY_ASSIGN(lhs, postfix({ lhs->range.Start, m_endLoc }, op, lhs));
            continue;
        }

        const auto current = m_token.getPrecedence();
        advance();
        TRY_DECL(rhs, primary())

        while (m_token.getPrecedence() > current || (m_token.isRightToLeft() && m_token.getPrecedence() == current)) {
            TRY_ASSIGN(rhs, expression(rhs, m_token.getPrecedence()))
        }

        auto start = lhs->range.Start;
        TRY_ASSIGN(lhs, binary({ start, m_endLoc }, op, lhs, rhs))
    }
    return lhs;
}

/**
 * primary = identifier
 *         | "(" expression ")"
 *         | TypeOf "is" TypeExpr
 *         | IfExpr
 *         | SizeOfExpr
 *         | AlignOfExpr
 *         | literal
 *         | <Unary Op> primary { <op> expression }
 *         .
 */
auto Parser::primary() -> Result<AstExpr*> {
    switch (m_token.getKind()) {
    case TokenKind::Identifier:
        return identifier();
    case TokenKind::ParenOpen: {
        // Sub expression
        advance();
        TRY_DECL(expr, expression())
        TRY(consume(TokenKind::ParenClose))
        return expr;
    }
    case TokenKind::If:
        return ifExpr();
    case TokenKind::TypeOf:
        return typeOfExpr();
    case TokenKind::SizeOf:
        return sizeOfExpr();
    case TokenKind::AlignOf:
        return alignOfExpr();
    case TokenKind::Multiply:
        // When '*' is a prefix operator, then it means dereference
        m_token.setKind(TokenKind::Dereference);
        break;
    case TokenKind::Minus:
        // When '-' is a prefix operator, then it means negate
        m_token.setKind(TokenKind::Negate);
        break;
    default:
        if (m_token.isLiteral()) {
            return literal();
        }
        break;
    }

    // is a prefix operator
    if (m_token.isUnary() && m_token.isLeftToRight()) {
        const auto tkn = m_token;
        advance();

        TRY_DECL(fact, primary())
        TRY_DECL(expr, expression(fact, tkn.getPrecedence()))

        return prefix({ tkn.getRange().Start, m_endLoc }, tkn, expr);
    }

    // unexpected token
    return makeError(Diag::expectedExpression, m_token, m_token.description());
}

/**
 * prefix = <Op> expression
 *        .
 */
auto Parser::prefix(llvm::SMRange range, const Token& tkn, AstExpr* expr) const -> Result<AstExpr*> {
    switch (tkn.getKind()) {
    case TokenKind::Dereference:
        return m_context.create<AstDereference>(range, expr);
    case TokenKind::AddressOf:
        return m_context.create<AstAddressOf>(range, expr);
    case TokenKind::Negate:
    case TokenKind::LogicalNot:
        return m_context.create<AstUnaryExpr>(range, tkn, expr);
    default:
        llvm_unreachable("Unexpected prefix operator");
    }
}

/**
 * postfix = expression <Op>
 *         .
 */
auto Parser::postfix(llvm::SMRange range, const Token& tkn, AstExpr* expr) -> Result<AstExpr*> {
    switch (tkn.getKind()) {
    case TokenKind::ParenOpen: {
        // Function call
        TRY_DECL(args, expressionList())
        TRY(consume(TokenKind::ParenClose))
        return m_context.create<AstCallExpr>(
            llvm::SMRange { range.Start, m_endLoc },
            expr,
            args
        );
    }
    case TokenKind::As: {
        // Syntax "a as type" is a cast
        TRY_DECL(type, typeExpr())
        return m_context.create<AstCastExpr>(
            llvm::SMRange { range.Start, m_endLoc },
            expr,
            type,
            false
        );
    }
    case TokenKind::Is: {
        // Syntax "a is type" is a type check
        auto* lhs = m_context.create<AstTypeOf>(
            expr->getRange(),
            expr
        );
        TRY_DECL(rhs, typeExpr())
        return m_context.create<AstIsExpr>(
            llvm::SMRange { range.Start, m_endLoc },
            lhs,
            rhs
        );
    }
    default:
        llvm_unreachable("Unexpected postfix operator");
    }
}

/**
 * binary = expression <Op> expression
 *        .
 */
auto Parser::binary(llvm::SMRange range, const Token& tkn, AstExpr* lhs, AstExpr* rhs) const -> Result<AstExpr*> {
    switch (tkn.getKind()) {
    case TokenKind::ConditionAnd: {
        auto copy = tkn;
        copy.setKind(TokenKind::LogicalAnd);
        return m_context.create<AstBinaryExpr>(range, copy, lhs, rhs);
    }
    case TokenKind::MemberAccess:
        return m_context.create<AstMemberExpr>(range, tkn, lhs, rhs);
    case TokenKind::Assign:
        return m_context.create<AstAssignExpr>(range, lhs, rhs);
    default:
        return m_context.create<AstBinaryExpr>(range, tkn, lhs, rhs);
    }
}

/**
 * TypeOf = TypeOf "is" TypeExpr .
 */
auto Parser::typeOfExpr() -> Result<AstIsExpr*> {
    const auto start = m_token.getRange().Start;
    TRY_DECL(typeOf, kwTypeOf())
    TRY(consume(TokenKind::Is))
    TRY_DECL(type, typeExpr())

    return m_context.create<AstIsExpr>(
        llvm::SMRange { start, m_endLoc },
        typeOf,
        type
    );
}

/**
 * AlignOfExpr = "AlignOf" "(" ( Expression | TypeExpr ) ")" .
 */
auto Parser::alignOfExpr() -> Result<AstAlignOfExpr*> {
    // assume m_token == ALIGNOF
    const auto start = m_token.getRange().Start;
    TRY_DECL(type, kwTypeOf())

    return m_context.create<AstAlignOfExpr>(
        llvm::SMRange { start, m_endLoc },
        type
    );
}

/**
 * SizeOf = "SIZEOF" "(" ( Expression | TypeExpr ) ")" .
 */
auto Parser::sizeOfExpr() -> Result<AstSizeOfExpr*> {
    // assume m_token == SIZEOF
    const auto start = m_token.getRange().Start;
    TRY_DECL(type, kwTypeOf())

    return m_context.create<AstSizeOfExpr>(
        llvm::SMRange { start, m_endLoc },
        type
    );
}

/**
 * IdentExpr
 *   = id
 *   .
 */
auto Parser::identifier() -> Result<AstIdentExpr*> {
    const auto start = m_token.getRange().Start;
    TRY(expect(TokenKind::Identifier))
    auto name = m_token.getStringValue();
    advance();

    return m_context.create<AstIdentExpr>(
        llvm::SMRange { start, m_endLoc },
        name
    );
}

[[nodiscard]] auto Parser::identifier(const Token& token) const -> AstIdentExpr* {
    return m_context.create<AstIdentExpr>(
        token.getRange(),
        token.getStringValue()
    );
}

/**
 * IfExpr = "IF" expr "THEN" expr "ELSE" expr .
 */
auto Parser::ifExpr() -> Result<AstIfExpr*> {
    // assume m_token == IF
    const auto start = m_token.getRange().Start;
    advance();

    TRY_DECL(expr, expression(ExprFlags::defaultSequence))

    TRY(consume(TokenKind::Then))
    TRY_DECL(trueExpr, expression())

    TRY(consume(TokenKind::Else))
    TRY_DECL(falseExpr, expression())

    return m_context.create<AstIfExpr>(
        llvm::SMRange { start, m_endLoc },
        expr,
        trueExpr,
        falseExpr
    );
}

/**
 * literal = stringLiteral
 *         | IntegerLiteral
 *         | FloatingPointLiteral
 *         | "TRUE"
 *         | "FALSE"
 *         | "NULL"
 *         .
 */
auto Parser::literal() -> Result<AstLiteralExpr*> {
    const auto token = m_token;
    advance();

    return m_context.create<AstLiteralExpr>(
        token.getRange(),
        token.getValue()
    );
}

/**
 * Parse comma separated list of expressions
 */
auto Parser::expressionList() -> Result<AstExprList*> {
    const auto start = m_token.getRange().Start;
    std::vector<AstExpr*> exprs;

    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose, TokenKind::EndOfStmt)) {
        TRY_DECL(expr, expression())

        exprs.emplace_back(expr);
        if (!accept(TokenKind::Comma)) {
            break;
        }
    }

    return m_context.create<AstExprList>(
        llvm::SMRange { start, m_endLoc },
        std::move(exprs)
    );
}

//----------------------------------------
// Helpers
//----------------------------------------

auto Parser::accept(const TokenKind kind) -> bool {
    if (m_token.is(kind)) {
        advance();
        return true;
    }
    return false;
}

auto Parser::acceptNext(const TokenKind kind) -> bool {
    Lexer peek { m_lexer };

    if (const Token token = peek.next(); token.is(kind)) {
        m_token = token;
        m_lexer = peek;
        return true;
    }

    return false;
}

auto Parser::expect(const TokenKind kind) const -> Result<void> {
    if (m_token.is(kind)) {
        return {};
    }

    return makeError(Diag::unexpectedToken, m_token, Token::description(kind), m_token.description());
}

auto Parser::consume(const TokenKind kind) -> Result<void> {
    TRY(expect(kind))
    advance();
    return {};
}

void Parser::advance() {
    m_endLoc = m_token.getRange().End;
    m_token = m_lexer.next();
    adjustTokenKind();
}

/**
 * Updates current token based on the parser state (e.f. m_exprFlags)
 *
 * This can modify parse state
 */
void Parser::adjustTokenKind() {
    using namespace flags;

    switch (m_token.getKind()) {
    case TokenKind::Assign:
        // This is to allow for the following syntax:
        //   a = b = c
        // Where the first '=' is treated as an assignment and the second as an equality check.
        //
        if (has(m_exprFlags, ExprFlags::useAssign)) {
            unset(m_exprFlags, ExprFlags::useAssign);
            set(m_exprFlags, ExprFlags::useEqual);
        } else if (has(m_exprFlags, ExprFlags::useEqual)) {
            m_token.setKind(TokenKind::Equal);
        }
        break;
    case TokenKind::Comma:
        // This is to allow for the following syntax:
        //   if a, b, c then
        // Where each ',' is treated as low precedence 'logical and' operator.
        if (has(m_exprFlags, ExprFlags::commaAsAnd)) {
            m_token.setKind(TokenKind::ConditionAnd);
        }
        break;
    default:
        break;
    }
}

/**
 * Lex until we match the closing paren
 *
 * @param parens number of closing parens to match
 * @param message message to show if there is an error
 * @return result error or void
 */
[[nodiscard]] auto Parser::skipUntilMatchingClosingParen(int parens, llvm::StringRef message) -> Result<void> {
    while (true) {
        switch (m_token.getKind()) {
        case TokenKind::ParenOpen:
            parens++;
            break;
        case TokenKind::ParenClose:
            parens--;
            if (parens == 0) {
                advance();
                return {};
            }
            if (parens < 0) {
                return makeError(Diag::unexpectedToken, m_token, message, m_token.description());
            }
            break;
        case TokenKind::EndOfStmt:
        case TokenKind::EndOfFile:
        case TokenKind::Invalid:
            return makeError(Diag::unexpectedToken, m_token, message, m_token.description());
        default:
            break;
        }
        advance();
    }
}