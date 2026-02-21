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
// All unsigned integral types
def UnsignedIntegral : TypeKind;
// All signed integral types
def SignedIntegral : TypeKind;
// All floating point types
def FloatingPoint : TypeKind;
// pointers, references, etc.
def Compound : TypeKind;
// functions, user defined types, etc.
def Aggregate : TypeKind;

// Base type structure
class BaseType<TypeKind kind_, string cls_ = ""> {
    // Type category
    TypeKind kind = kind_;
    // Type backing class
    string cls = cls_;
}

// Type that is exposed as lexical keyword
class KeywordType<string str_, TypeKind kind_, string cls_ = "">: BaseType<kind_, cls_>, Type<str_>;

// -----------------------------------------------------------------------------
// Basic types
// -----------------------------------------------------------------------------

def Bool    : KeywordType<"BOOL", Primitive>;
def ZString : KeywordType<"ZSTRING", Primitive>;

// -----------------------------------------------------------------------------
// Integral types
// -----------------------------------------------------------------------------

def Byte     : KeywordType<"BYTE",     SignedIntegral,   "TypeIntegral">;
def UByte    : KeywordType<"UBYTE",    UnsignedIntegral, "TypeIntegral">;
def Short    : KeywordType<"SHORT",    SignedIntegral,   "TypeIntegral">;
def UShort   : KeywordType<"USHORT",   UnsignedIntegral, "TypeIntegral">;
def Integer  : KeywordType<"INTEGER",  SignedIntegral,   "TypeIntegral">;
def UInteger : KeywordType<"UINTEGER", UnsignedIntegral, "TypeIntegral">;
def Long     : KeywordType<"LONG",     SignedIntegral,   "TypeIntegral">;
def ULong    : KeywordType<"ULONG",    UnsignedIntegral, "TypeIntegral">;

// -----------------------------------------------------------------------------
// Floating point values
// -----------------------------------------------------------------------------

def Single : KeywordType<"SINGLE", FloatingPoint, "TypeFloatingPoint">;
def Double : KeywordType<"DOUBLE", FloatingPoint, "TypeFloatingPoint">;

// -----------------------------------------------------------------------------
// Compound types
// -----------------------------------------------------------------------------

def PointerType   : BaseType<Compound, "TypePointer">;
def ReferenceType : BaseType<Compound, "TypeReference">;
def QualifiedType : BaseType<Compound, "TypeQualified">;

// -----------------------------------------------------------------------------
// Aggregate types
// -----------------------------------------------------------------------------

def FunctionType : BaseType<Aggregate, "TypeFunction">;

// -----------------------------------------------------------------------------
// Sentinel types
// -----------------------------------------------------------------------------

def VoidType : BaseType<Sentinel>;
def NullType : BaseType<Sentinel>;
def AnyType  : BaseType<Sentinel>;
