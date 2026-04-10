// IR instruction definitions for lbc
// Processed by lbc-tblgen --gen=lbc-ir-def

// =============================================================================
// Args
// =============================================================================

// base class for ir class data
class Member;

// -----------------------------------------------------------------------------
// Data field, has setter and getter, passed through ctor (if no default value)
// -----------------------------------------------------------------------------
class Arg<string type_, string name_, bit mutable_ = false, string default_ = "", bit reference_ = false>: Member {
    string type = type_;
    string name = name_;
    bit mutable = mutable_;
    string default = default_;
    bit reference = reference_;
}

// -----------------------------------------------------------------------------
// Function allows specifying custom code that gets emitted into generated ast
// -----------------------------------------------------------------------------
class Func<code func_>: Member {
    code func = func_;
}

// =============================================================================
// Tree data structure
// =============================================================================

// Group node
class Group;

// -----------------------------------------------------------------------------
// Base node.
// -----------------------------------------------------------------------------
class Node<string desc_ = "", Group parent_, list<Member> members_ = []> {
    string desc = desc_;
    Group parent = parent_;
    list<Member> members = members_;
}

// -----------------------------------------------------------------------------
// Group node — an abstract intermediate that concrete nodes inherit from.
// Groups define shared members and form the parent in the hierarchy.
// -----------------------------------------------------------------------------
class Group<
    string desc_,
    Group parent_,
    list<Member> members_ = []
> : Node<desc_, parent_, members_>;

// -----------------------------------------------------------------------------
// Leaf node — a concrete instruction class
// -----------------------------------------------------------------------------
class Leaf<
    string mnemonic_,
    string desc_,
    Group parent_,
    list<Member> members_ = []
> : Node<desc_, parent_, members_> {
    string mnemonic = mnemonic_;
}

// =============================================================================
// Root node
// =============================================================================
def Root : Group<"The root of IR instructions", ?>;

// =============================================================================
// Terminators
// =============================================================================
def Terminator : Group<"Terminating instructions", Root>;

// jmp @destination
def Jmp : Leaf<"jmp", "Unconditional branch", Terminator, [
    Arg<"BasicBlock*", "destination">,
]>;

// jmp %condition, @trueBlock, @falseBlock
def CondJmp : Leaf<"jmp", "Conditional branch", Terminator, [
    Arg<"Value*", "condition">,
    Arg<"BasicBlock*", "trueBlock">,
    Arg<"BasicBlock*", "falseBlock">,
]>;

// ret %value
def Ret : Leaf<"ret", "Return value", Terminator, [
    Arg<"Value*", "value">,
]>;

// =============================================================================
// Expressions
// =============================================================================

def Expression : Group<"Expression returning a result", Root, [
    Arg<"NamedValue*", "result">,
]>;

// -----------------------------------------------------------------------------
// Unary operations
// -----------------------------------------------------------------------------

def Unary : Group<"Unary expressions", Expression, [
    Arg<"Value*", "operand">,
]>;

// %result = neg %operand
def Neg : Leaf<"neg", "Arithmetic negation", Unary>;

// %result = not %operand
def Not : Leaf<"not", "Logical NOT", Unary>;

// -----------------------------------------------------------------------------
// Binary operations
// -----------------------------------------------------------------------------

def Binary : Group<"Binary expressions", Expression, [
    Arg<"Value*", "lhs">,
    Arg<"Value*", "rhs">,
]>;

// %result = add %lhs, %rhs
def Add : Leaf<"add", "Addition", Binary>;
def Sub : Leaf<"sub", "Subtraction", Binary>;
def Mul : Leaf<"mul", "Multiplication", Binary>;
def Div : Leaf<"div", "Division", Binary>;
def Mod : Leaf<"mod", "Modulo", Binary>;
def And : Leaf<"and", "Logical AND", Binary>;
def Or  : Leaf<"or",  "Logical OR", Binary>;

// -----------------------------------------------------------------------------
// Comparisons
// -----------------------------------------------------------------------------

def Comparison : Group<"Comparison expressions", Binary>;

// %result = eq %lhs, %rhs
def Eq : Leaf<"eq", "Equal", Comparison>;
def Ne : Leaf<"ne", "Not equal", Comparison>;
def Lt : Leaf<"lt", "Less than", Comparison>;
def Le : Leaf<"le", "Less than or equal", Comparison>;
def Gt : Leaf<"gt", "Greater than", Comparison>;
def Ge : Leaf<"ge", "Greater than or equal", Comparison>;

// -----------------------------------------------------------------------------
// Misc expressions
// -----------------------------------------------------------------------------

// %result = cast type, %operand
def Cast : Leaf<"cast", "Type conversion", Expression, [
    Arg<"const Type*", "targetType">,
    Arg<"Value*", "operand">,
]>;

// %result = load %source
def Load : Leaf<"load", "Load value through pointer", Expression, [
    Arg<"Value*", "source">,
]>;

// %result = addrof $operand
def AddrOf : Leaf<"addrof", "Take address of a variable", Expression, [
    Arg<"Value*", "operand">,
]>;

def Call : Leaf<"call", "Call a function", Expression, [
    Arg<"NamedValue*", "callee">,
    Arg<"std::span<Value*>", "args">,
]>;

// =============================================================================
// Declarations
// =============================================================================

def Declaration : Group<"Declaration instructions", Root, [
    Arg<"NamedValue*", "result">,
]>;

// $result = var type
def Var : Leaf<"var", "Variable declaration", Declaration, [
    Arg<"const Type*", "type">,
]>;

// =============================================================================
// Misc
// =============================================================================

// store %destination, %source
def Store : Leaf<"store", "Store value through pointer", Root, [
    Arg<"NamedValue*", "dest">,
    Arg<"Value*", "src">,
]>;

// retain %operand
def Retain : Leaf<"retain",  "Increment reference count", Root, [
    Arg<"NamedValue*", "operand">,
]>;

// release %operand
def Release : Leaf<"release", "Decrement reference count", Root, [
    Arg<"NamedValue*", "operand">,
]>;
