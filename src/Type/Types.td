// Types definitions for lbc
// Processed by lbc-tblgen to generate Types.inc

class TypeKind;
def Primitive: TypeKind;
def Integral: TypeKind;
def FloatingPoint: TypeKind;

class DataType<string str_, TypeKind kind_>: Type<str_> {
    TypeKind kind = kind_;
}

def BoolType    : DataType<"BOOL", Primitive>;
def ZString     : DataType<"ZSTRING", Primitive>;

def Byte     : DataType<"BYTE", Primitive>;
def UByte    : DataType<"UBYTE", Integral>;
def Short    : DataType<"SHORT", Integral>;
def UShort   : DataType<"USHORT", Integral>;
def Integer  : DataType<"INTEGER", Integral>;
def UInteger : DataType<"UINTEGER", Integral>;
def Long     : DataType<"LONG", Integral>;
def ULong    : DataType<"ULONG", Integral>;

def Single : DataType<"SINGLE", FloatingPoint>;
def Double : DataType<"DOUBLE", FloatingPoint>;
