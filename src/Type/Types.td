// Types definitions for lbc
// Processed by lbc-tblgen to generate Types.inc

// =============================================================================
// Define type structures
// =============================================================================

// TokenKind defines Type as a token group.
#ifndef DEFINES_TYPE_GROUP
class Type<string str_> {
    string str = str_;
}
#endif

// Type kind categories
class TypeKind;
// Represent special types such as void, any, null
def Sentinel : TypeKind;
// Primitive are types that lbc does not categorize as numeric types. e.g. bool, zstring
def Primitive : TypeKind;
// All signed and unsigned integral types
def Integral : TypeKind;
// All floating point types
def FloatingPoint : TypeKind;
// pointers, references, etc.
def Compound : TypeKind;
// functions, user defined types, etc.
def Aggregate : TypeKind;

// Base type structure
class BaseType<TypeKind kind_> {
    TypeKind kind = kind_;
}

// Type that is exposed as lexical keyword
class KeywordType<string str_, TypeKind kind_>: BaseType<kind_>, Type<str_>;

// Concrete types are backed by their own class that inherit Type
class ConcreteType<string class_, TypeKind kind_>: BaseType<kind_> {
    string cls = class_;
}

// -----------------------------------------------------------------------------
// Basic types
// -----------------------------------------------------------------------------

def Bool    : KeywordType<"BOOL", Primitive>;
def ZString : KeywordType<"ZSTRING", Primitive>;

// -----------------------------------------------------------------------------
// Integral types
// -----------------------------------------------------------------------------

def Byte     : KeywordType<"BYTE", Integral>;
def UByte    : KeywordType<"UBYTE", Integral>;
def Short    : KeywordType<"SHORT", Integral>;
def UShort   : KeywordType<"USHORT", Integral>;
def Integer  : KeywordType<"INTEGER", Integral>;
def UInteger : KeywordType<"UINTEGER", Integral>;
def Long     : KeywordType<"LONG", Integral>;
def ULong    : KeywordType<"ULONG", Integral>;

// -----------------------------------------------------------------------------
// Floating point values
// -----------------------------------------------------------------------------

def Single : KeywordType<"SINGLE", FloatingPoint>;
def Double : KeywordType<"DOUBLE", FloatingPoint>;

// -----------------------------------------------------------------------------
// Compound types
// -----------------------------------------------------------------------------

def PointerType   : ConcreteType<"PointerType", Compound>;
def ReferenceType : ConcreteType<"ReferenceType", Compound>;
def QualifiedType : ConcreteType<"QualifiedType", Compound>;

// -----------------------------------------------------------------------------
// Aggregate types
// -----------------------------------------------------------------------------

def FunctionType : ConcreteType<"FunctionType", Aggregate>;

// -----------------------------------------------------------------------------
// Sentinel types
// -----------------------------------------------------------------------------

def VoidType : BaseType<Sentinel>;
def NullType : BaseType<Sentinel>;
def AnyType  : BaseType<Sentinel>;
