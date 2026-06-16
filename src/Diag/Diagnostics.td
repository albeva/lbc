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

def notImplemented            : Error<System, "E0001", "Not implemented">;
def noInputFiles              : Error<System, "E0002", "no input files">;
def inputFileNotFound         : Error<System, "E0003", "input file not found: {path}">;
def ambiguousOutput           : Error<System, "E0004", "cannot direct multiple input files to a single output file">;
def cannotOpenOutput          : Error<System, "E0005", "cannot open output file {path}: {reason}">;
def backendVerificationFailed : Error<System, "E0006", "internal error: generated LLVM module failed verification">;
def optimizerFailed           : Error<System, "E0007", "optimizer failed: {reason}">;
def codegenFailed             : Error<System, "E0008", "code generation failed: {reason}">;
def linkerFailed              : Error<System, "E0009", "linking failed: {reason}">;
def toolNotFound              : Error<System, "E0010", "cannot find tool {tool}; pass --toolchain or add it to PATH">;

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
def referenceNotLast : Error<Parse, "E0202", "a reference must be the last qualifier in a type">;
def unsupportedLinkage : Error<Parse, "E0203", "unsupported language linkage {lang}; only C is supported">;

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
def notCallable            : Error<Sema, "E0313", "expression not a callable">;
def invalidUnaryOperand    : Error<Sema, "E0314", "invalid operand to {op}: {type}">;
def dereferencingAnyPtr    : Error<Sema, "E0315", "dereferencing any ptr">;
def invalidReferenceInit   : Error<Sema, "E0316", "Cannot initialize reference from given expression">;
def constToReference       : Error<Sema, "E0317", "cannot apply CONST to a reference">;
def notAssignable          : Error<Sema, "E0318", "expression is not assignable">;
def assignToConst          : Error<Sema, "E0319", "cannot assign to a constant value">;
def invalidMoveOperand     : Error<Sema, "E0320", "cannot MOVE a temporary value">;
def returnOutsideFunction  : Error<Sema, "E0321", "RETURN outside of a function or subroutine">;
def returnValueInSub       : Error<Sema, "E0322", "a subroutine cannot return a value">;
def returnMissingValue     : Error<Sema, "E0323", "a function must return a value">;
def variadicRequiresC      : Error<Sema, "E0324", "variadic '...' parameters are only allowed on extern C declarations">;
