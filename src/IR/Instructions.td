// IR instruction definitions for lbc
// Processed by lbc-tblgen --gen=lbc-ir-def

// =============================================================================
// Operand types — encode C++ types for type-safe operand access
// =============================================================================

class OperandKind<string cpp_> {
    string cpp = cpp_;
}

def Value    : OperandKind<"Value*">;         /// any IR value
def Block    : OperandKind<"Block*">;         /// block reference (@label)
def Function : OperandKind<"Function*">;      /// function reference
def Type     : OperandKind<"const Type*">;    /// compiler type

// =============================================================================
// Operand definitions
// =============================================================================

// Base class for instruction operands.
class Operand;

// Single named operand of a given type.
class Arg<OperandKind kind_, string name_> : Operand {
    OperandKind kind = kind_;
    string name = name_;
}

// Variadic operand — maps to std::span in generated code.
class Args<OperandKind kind_, string name_> : Operand {
    OperandKind kind = kind_;
    string name = name_;
}

// Result operand — the output value produced by an instruction.
class Result<OperandKind kind_> : Operand {
    OperandKind kind = kind_;
}

// =============================================================================
// Instruction base class
// =============================================================================

// Base class for all IR instructions.
// mnemonic: textual name used in IR printing (may be shared across instructions)
// desc: human-readable description
// category: shared operand pattern (or ? for standalone instructions)
// operands: additional instruction-specific operands
class Instruction<string mnemonic_, string desc_, list<Operand> operands_ = []> {
    string mnemonic = mnemonic_;
    string desc = desc_;
    list<Operand> operands = operands_;
}

//=============================================================================
// Terminators
// =============================================================================
class Terminator<string mnemonic_, string desc_, list<Operand> operands_ = []> : Instruction<mnemonic_, desc_, operands_>;

// jmp @destination
def Jmp : Terminator<"jmp", "Unconditional branch", [
    Arg<Block, "destination">,
]>;

// jmp %condition, @trueBlock, @falseBlock
def CondJmp : Terminator<"jmp", "Conditional branch", [
    Arg<Value, "condition">,
    Arg<Block, "trueBlock">,
    Arg<Block, "falseBlock">,
]>;

// ret
def Ret : Terminator<"ret", "Return void">;

// ret %value
def RetVal : Terminator<"ret", "Return value", [
    Arg<Value, "value">,
]>;

// =============================================================================
// Unary operations
// =============================================================================
class Unary<string mnemonic_, string desc_> : Instruction<mnemonic_, desc_, [
    Result<Value>,
    Arg<Value, "operand">,
]>;

// %result = neg %operand
def Neg : Unary<"neg", "Arithmetic negation">;

// %result = not %operand
def Not : Unary<"not", "Logical NOT">;

// =============================================================================
// Binary arithmetic
// =============================================================================
class Binary<string mnemonic_, string desc_> : Instruction<mnemonic_, desc_, [
    Result<Value>,
    Arg<Value, "lhs">,
    Arg<Value, "rhs">,
]>;

// %result = add %lhs, %rhs
def Add : Binary<"add", "Addition">;
def Sub : Binary<"sub", "Subtraction">;
def Mul : Binary<"mul", "Multiplication">;
def Div : Binary<"div", "Division">;
def Mod : Binary<"mod", "Modulo">;
def And : Binary<"and", "Logical AND">;
def Or  : Binary<"or",  "Logical OR">;

// =============================================================================
// Comparisons
// =============================================================================
class Comparison<string mnemonic_, string desc_> : Binary<mnemonic_, desc_>;

// %result = eq %lhs, %rhs
def Eq : Comparison<"eq", "Equal">;
def Ne : Comparison<"ne", "Not equal">;
def Lt : Comparison<"lt", "Less than">;
def Le : Comparison<"le", "Less than or equal">;
def Gt : Comparison<"gt", "Greater than">;
def Ge : Comparison<"ge", "Greater than or equal">;

// =============================================================================
// Declarations
// =============================================================================
class Declaration<string mnemonic_, string desc_, list<Operand> operands_ = []> : Instruction<mnemonic_, desc_, operands_>;

// $result = var type
def Var : Declaration<"var", "Variable declaration", [
    Result<Value>,
    Arg<Type, "type">,
]>;

// =============================================================================
// Memory operations
// =============================================================================
class Memory<string mnemonic_, string desc_, list<Operand> operands_ = []> : Instruction<mnemonic_, desc_, operands_>;

// %result = cast type, %operand
def Cast : Memory<"cast", "Type conversion", [
    Result<Value>,
    Arg<Type, "targetType">,
    Arg<Value, "operand">,
]>;

// %result = load %source
def Load : Memory<"load", "Load value through pointer", [
    Result<Value>,
    Arg<Value, "source">,
]>;

// store %destination, %source
def Store : Memory<"store", "Store value through pointer", [
    Arg<Value, "destination">,
    Arg<Value, "source">,
]>;

// %result = addrof $operand
def AddrOf : Memory<"addrof", "Take address of a variable", [
    Result<Value>,
    Arg<Value, "operand">,
]>;

// retain %operand
def Retain  : Memory<"retain",  "Increment reference count", [
    Arg<Value, "operand">,
]>;

// release %operand
def Release : Memory<"release", "Decrement reference count", [
    Arg<Value, "operand">,
]>;

// =============================================================================
// Function calls
// =============================================================================

// call $callee, %arg0, %arg1, ...
def Call : Instruction<"call", "Void function call", [
    Arg<Function, "callee">,
    Args<Value, "arguments">,
]>;

// %result = call $callee, %arg0, %arg1, ...
def CallVal : Instruction<"call", "Function call with return value", [
    Result<Value>,
    Arg<Function, "callee">,
    Args<Value, "arguments">,
]>;
