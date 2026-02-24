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
def Sema   : Category;

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

// =============================================================================
// Lexer diagnostics
// =============================================================================

def invalid            : Error<Lex, "E0100", "invalid input">;
def unterminatedString : Error<Lex, "E0101", "unterminated string literal">;
def invalidNumber      : Error<Lex, "E0102", "invalid number">;

// =============================================================================
// Parser diagnostics
// =============================================================================

def unexpected : Error<Parse, "E0200", "unexpected {found}">;
def expected   : Error<Parse, "E0201", "expected {expected}, found {found}">;

// =============================================================================
// Semantic diagnostics
// =============================================================================

def undeclaredIdentifier   : Error<Sema, "E0300", "use of undeclared identifier {name}">;
def useBeforeDefinition    : Error<Sema, "E0301", "use of identifier before definition {name}">;
def redefinition           : Error<Sema, "E0302", "redefinition of {name}">;
def circularDependency     : Error<Sema, "E0303", "circular dependency on {name}">;
def typeMismatch           : Error<Sema, "E0304", "cannot convert {from} to {to}">;
def invalidOperands        : Error<Sema, "E0305", "invalid operands to {op}: {left} and {right}">;
def tooManyArguments       : Error<Sema, "E0306", "too many arguments: expected {expected:std::size_t}, got {got:std::size_t}">;
def tooFewArguments        : Error<Sema, "E0307", "too few arguments: expected {expected:std::size_t}, got {got:std::size_t}">;
def uninitializedReference : Error<Sema, "E0308", "reference variable {name} must be initialised">;
def referenceToReference   : Error<Sema, "E0309", "cannot create a reference to a reference">;
def pointerToReference     : Error<Sema, "E0310", "cannot create a pointer to a reference">;
def nullVariable           : Error<Sema, "E0311", "cannot declare a variable of type null">;
def nonAddressableExpr     : Error<Sema, "E0312", "cannot take address from expression">;

