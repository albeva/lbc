// IR instruction definitions for lbc
// Processed by lbc-tblgen --gen=lbc-ir-def

// =============================================================================
// Operands
// =============================================================================

class OperandKind<string cpp_> {
    string cpp = cpp_;
}

def Value    : OperandKind<"Value*">;         /// any IR value
def Block    : OperandKind<"Block*">;         /// block reference (@label)
def Function : OperandKind<"Function*">;      /// function reference
def Type     : OperandKind<"const Type*">;    /// compiler type

// Base class for instruction operands.
class Operand;

// Single named operand of a given type.
class Arg<OperandKind kind_, string name_, bit vararg_ = false> : Operand {
    OperandKind kind = kind_;
    string name = name_;
    bit vararg = vararg_;
}

// Result operand — the output value produced by an instruction.
class Result<OperandKind kind_> : Operand {
    OperandKind kind = kind_;
}

// =============================================================================
// Tree data structure
// =============================================================================

// Group node
class Group;

// -----------------------------------------------------------------------------
// Base node.
// -----------------------------------------------------------------------------
class Node<string desc_ = "", Group parent_, list<Operand> operands_ = []> {
    string desc = desc_;
    Group parent = parent_;
    list<Operand> operands = operands_;
}

// -----------------------------------------------------------------------------
// Group node — an abstract intermediate that concrete nodes inherit from.
// Groups define shared members and form the parent in the hierarchy.
// -----------------------------------------------------------------------------
class Group<
    string desc_,
    Group parent_,
    list<Operand> operands_ = []
> : Node<desc_, parent_, operands_>;

// -----------------------------------------------------------------------------
// Leaf node — a concrete instruction class
// -----------------------------------------------------------------------------
class Instr<
    string mnemonic_,
    string desc_,
    Group parent_,
    list<Operand> operand_ = []
> : Node<desc_, parent_, operand_> {
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
def Jmp : Instr<"jmp", "Unconditional branch", Terminator, [
    Arg<Block, "destination">,
]>;

// jmp %condition, @trueBlock, @falseBlock
def CondJmp : Instr<"jmp", "Conditional branch", Terminator, [
    Arg<Value, "condition">,
    Arg<Block, "trueBlock">,
    Arg<Block, "falseBlock">,
]>;

// ret
def Ret : Instr<"ret", "Return void", Terminator>;

// ret %value
def RetVal : Instr<"ret", "Return value", Terminator, [
    Arg<Value, "value">,
]>;

// =============================================================================
// Unary operations
// =============================================================================
def Unary : Group<"Unary expressions", Root, [
    Result<Value>,
    Arg<Value, "operand">,
]>;

// %result = neg %operand
def Neg : Instr<"neg", "Arithmetic negation", Unary>;

// %result = not %operand
def Not : Instr<"not", "Logical NOT", Unary>;

// =============================================================================
// Binary arithmetic
// =============================================================================
def Binary : Group<"Unary expressions", Root, [
    Result<Value>,
    Arg<Value, "lhs">,
    Arg<Value, "rhs">,
]>;

// %result = add %lhs, %rhs
def Add : Instr<"add", "Addition", Binary>;
def Sub : Instr<"sub", "Subtraction", Binary>;
def Mul : Instr<"mul", "Multiplication", Binary>;
def Div : Instr<"div", "Division", Binary>;
def Mod : Instr<"mod", "Modulo", Binary>;
def And : Instr<"and", "Logical AND", Binary>;
def Or  : Instr<"or",  "Logical OR", Binary>;

// =============================================================================
// Comparisons
// =============================================================================
def Comparison : Group<"Comparison expressions", Binary>;

// %result = eq %lhs, %rhs
def Eq : Instr<"eq", "Equal", Comparison>;
def Ne : Instr<"ne", "Not equal", Comparison>;
def Lt : Instr<"lt", "Less than", Comparison>;
def Le : Instr<"le", "Less than or equal", Comparison>;
def Gt : Instr<"gt", "Greater than", Comparison>;
def Ge : Instr<"ge", "Greater than or equal", Comparison>;

// =============================================================================
// Declarations
// =============================================================================
def Declaration : Group<"Declaration instructions", Root>;

// $result = var type
def Var : Instr<"var", "Variable declaration", Declaration, [
    Result<Value>,
    Arg<Type, "type">,
]>;

// =============================================================================
// Memory operations
// =============================================================================
def Memory : Group<"Declaration instructions", Root>;

// %result = cast type, %operand
def Cast : Instr<"cast", "Type conversion", Memory, [
    Result<Value>,
    Arg<Type, "targetType">,
    Arg<Value, "operand">,
]>;

// %result = load %source
def Load : Instr<"load", "Load value through pointer", Memory, [
    Result<Value>,
    Arg<Value, "source">,
]>;

// store %destination, %source
def Store : Instr<"store", "Store value through pointer", Memory, [
    Arg<Value, "destination">,
    Arg<Value, "source">,
]>;

// %result = addrof $operand
def AddrOf : Instr<"addrof", "Take address of a variable", Memory, [
    Result<Value>,
    Arg<Value, "operand">,
]>;

// retain %operand
def Retain  : Instr<"retain",  "Increment reference count", Memory, [
    Arg<Value, "operand">,
]>;

// release %operand
def Release : Instr<"release", "Decrement reference count", Memory, [
    Arg<Value, "operand">,
]>;

// =============================================================================
// Function calls
// =============================================================================

def Call : Instr<"call", "Void function call", Root, [
    Arg<Function, "callee">,
    Arg<Value, "arguments", true>,
]>;
