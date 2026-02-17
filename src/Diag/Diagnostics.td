// Diagnostic definitions for lbc
// Processed by lbc-tblgen to generate Diagnostics.hpp

// =============================================================================
// Severity levels
// =============================================================================

// note -> remark -> warning -> error:
// - note: informational, never stops compilation, can be silenced with a flag
// - remark: unused, but defined for parity with llvm
// - warning: does not stop compilation by default, can be silenced or promoted
//   to error with a flag
// - error: always stops compilation
//
// These values correspond to llvm::SourceMgr::DiagKind

class Severity;
def error   : Severity;
def warning : Severity;
def remark  : Severity;
def note    : Severity;

// =============================================================================
// Diagnostic categories
// =============================================================================

class Category;
def System : Category;
def Lex    : Category;
def Parse  : Category;
//def Sema   : Category;
//def IR     : Category;
//def LLVM   : Category;

// =============================================================================
// Diagnostic base
// =============================================================================

// Format string placeholders:
//   {name}       — parameter with default Loggable type
//   {name:type}  — parameter with explicit C++ type

class Diag<Severity sev_, Category cat_, string diagCode_, string msg_> {
    Severity severity = sev_;
    Category category = cat_;
    string diagCode = diagCode_;
    string message = msg_;
}

class Error<Category cat, string id, string msg>   : Diag<error, cat, id, msg>;
class Warning<Category cat, string id, string msg> : Diag<warning, cat, id, msg>;
class Note<Category cat, string id, string msg>    : Diag<note, cat, id, msg>;

// =============================================================================
// System diagnostics
// =============================================================================

def notImplemented  : Error<System, "E0001", "Not implemented">;
//def fileNotFound    : Error<System, "E0002", "file not found: {path}">;
//def includeNotFound : Error<System, "E0003", "included file not found: {path}">;

// =============================================================================
// Lexer diagnostics
// =============================================================================

def invalid            : Error<Lex, "E0100", "invalid input">;
def unterminatedString : Error<Lex, "E0101", "unterminated string literal">;
def invalidNumber      : Error<Lex, "E0102", "invalid number">;

// =============================================================================
// Parser diagnostics
// =============================================================================

def unexpected          : Error<Parse, "E0200", "unexpected {found}">;
def expected            : Error<Parse, "E0201", "expected {expected}, found {found}">;
//def unexpectedEndOfFile : Error<Parse, "E0202", "unexpected end of file">;
//def expectedType        : Error<Parse, "E0203", "expected type expression">;

// =============================================================================
// Semantic diagnostics
// =============================================================================

//def undeclaredIdentifier : Error<Sema, "E0300", "use of undeclared identifier {name}">;
//def redefinition         : Error<Sema, "E0301", "redefinition of {name}">;
//def typeMismatch         : Error<Sema, "E0302", "cannot convert {from} to {to}">;
//def invalidOperands      : Error<Sema, "E0303", "invalid operands to {op}: {left} and {right}">;
//def tooManyArguments     : Error<Sema, "E0304", "too many arguments: expected {expected:int}, got {got:int}">;
//def tooFewArguments      : Error<Sema, "E0305", "too few arguments: expected {expected:int}, got {got:int}">;
//def returnTypeMismatch   : Error<Sema, "E0306", "return type {found} does not match expected {expected}">;
// Warnings
//def unusedVariable       : Warning<Sema, "unused-variable", "unused variable {name}">;
//def unusedParameter      : Warning<Sema, "unused-parameter", "unused parameter {name}">;
// Notes
//def declaredHere         : Note<Sema, "N0001", "{name} declared here">;
//def previousDefinition   : Note<Sema, "N0002", "previous definition of {name} is here">;

// =============================================================================
// IR diagnostics
// =============================================================================

//def invalidModule : Error<IR, "E0400", "failed to load module {name}: {reason}">;

// =============================================================================
// LLVM diagnostics
// =============================================================================

//def codegenFailure : Error<LLVM, "E0500", "code generation failed: {reason}">;
