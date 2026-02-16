// Diagnostic definitions for lbc
// Processed by lbc-tblgen to generate Diagnostics.hpp

// =============================================================================
// Severity levels
// =============================================================================

// note -> warning -> error:
// - note: informational, never stops compilation, can be silenced with a flag
// - warning: does not stop compilation by default, can be silenced or promoted
//   to error with a flag
// - error: always stops compilation

class Severity;
def error   : Severity;
def warning : Severity;
def note    : Severity;

// =============================================================================
// Diagnostic categories
// =============================================================================

class Category;
def System : Category;
def Lex    : Category;
def Parse  : Category;
def Sema   : Category;
def IR     : Category;
def LLVM   : Category;

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

def fileNotFound    : Error<System, "E0001", "file not found: {path}">;
def includeNotFound : Error<System, "E0002", "included file not found: {path}">;

// =============================================================================
// Lexer diagnostics
// =============================================================================

def invalidCharacter   : Error<Lex, "E0100", "invalid character {char}">;
def unterminatedString : Error<Lex, "E0101", "unterminated string literal">;
def invalidNumber      : Error<Lex, "E0102", "invalid number literal {text}">;

// =============================================================================
// Parser diagnostics
// =============================================================================

def unexpectedToken     : Error<Parse, "E0200", "unexpected {found}, expected {expected}">;
def unexpectedEndOfFile : Error<Parse, "E0201", "unexpected end of file">;
def expectedExpression  : Error<Parse, "E0202", "expected expression, found {found}">;
def expectedIdentifier  : Error<Parse, "E0203", "expected identifier, found {found}">;
def expectedType        : Error<Parse, "E0204", "expected type expression">;

// =============================================================================
// Semantic diagnostics
// =============================================================================

def undeclaredIdentifier : Error<Sema, "E0300", "use of undeclared identifier {name}">;
def redefinition         : Error<Sema, "E0301", "redefinition of {name}">;
def typeMismatch         : Error<Sema, "E0302", "cannot convert {from} to {to}">;
def invalidOperands      : Error<Sema, "E0303", "invalid operands to {op}: {left} and {right}">;
def tooManyArguments     : Error<Sema, "E0304", "too many arguments: expected {expected:int}, got {got:int}">;
def tooFewArguments      : Error<Sema, "E0305", "too few arguments: expected {expected:int}, got {got:int}">;
def returnTypeMismatch   : Error<Sema, "E0306", "return type {found} does not match expected {expected}">;
// Warnings
def unusedVariable       : Warning<Sema, "unused-variable", "unused variable {name}">;
def unusedParameter      : Warning<Sema, "unused-parameter", "unused parameter {name}">;
// Notes
def declaredHere         : Note<Sema, "N0001", "{name} declared here">;
def previousDefinition   : Note<Sema, "N0002", "previous definition of {name} is here">;

// =============================================================================
// IR diagnostics
// =============================================================================

def invalidModule : Error<IR, "E0400", "failed to load module {name}: {reason}">;

// =============================================================================
// LLVM diagnostics
// =============================================================================

def codegenFailure : Error<LLVM, "E0500", "code generation failed: {reason}">;
