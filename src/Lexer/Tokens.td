// Token definitions for lbc
// Processed by lbc-tblgen to generate Tokens.inc

// Define token groups
class Group;
def General  : Group;
def Literal  : Group;
def Symbol   : Group;
def Operator : Group;
def Keyword  : Group;
def Type     : Group;

// Define Operator categories
class Category;
def Arithmetic : Category;
def Assignment : Category;
def Call       : Category;
def Cast       : Category;
def Comparison : Category;
def Logical    : Category;
def Memory     : Category;

// Define Token base class
class Token<string str_, Group group_> {
    string str = str_;
    Group group = group_;
}
class General<string str>  : Token<str, General>;
class Literal<string str>  : Token<str, Literal>;
class Symbol<string str>   : Token<str, Symbol>;
class Operator<
    string str,
    int prec_,
    bit isBinary_,
    bit isLeftAssociative_,
    Category cat_
> : Token<str, Operator> {
    int prec = prec_;
    bit isBinary = isBinary_;
    bit isLeftAssociative = isLeftAssociative_;
    Category category = cat_;
}
class Keyword<string str>  : Token<str, Keyword>;
class Type<string str> : Token<str, Type>;

// General
def Invalid    : General<"<Invalid>">;
def EndOfFile  : General<"<End-Of-File>">;
def EndOfStmt  : General<"<End-Of-Stmt>">;
def Identifier : General<"<Identifier>">;

// Literals
def IntegerLiteral : Literal<"<Integer-Literal>">;
def FloatLiteral   : Literal<"<Float-Literal>">;
def StringLiteral  : Literal<"<String-Literal>">;
def BooleanLiteral : Literal<"<Boolean-Literal>">;
def NullLiteral    : Literal<"<Null-Literal>">;

// Symbols
def Comma        : Symbol<",">;
def ParenClose   : Symbol<")">;
def BracketOpen  : Symbol<"[">;
def BracketClose : Symbol<"]">;
def Ellipsis     : Symbol<"...">;

// Operators
def ParenOpen      : Operator<"(",   14, 0, 0, Call>;
def MemberAccess   : Operator<".",   13, 1, 1, Memory>;
def PointerAccess  : Operator<"->",  13, 1, 1, Memory>;
def AddressOf      : Operator<"@",   12, 0, 0, Memory>;
def Dereference    : Operator<"*",   12, 0, 0, Memory>;
def Negate         : Operator<"-",   11, 0, 0, Arithmetic>;
def LogicalNot     : Operator<"NOT", 11, 0, 1, Logical>;
def Is             : Operator<"IS",  10, 0, 0, Comparison>;
def As             : Operator<"AS",  10, 0, 0, Cast>;
def Multiply       : Operator<"*",    9, 1, 1, Arithmetic>;
def Divide         : Operator<"/",    9, 1, 1, Arithmetic>;
def Modulus        : Operator<"MOD",  8, 1, 1, Arithmetic>;
def Plus           : Operator<"+",    7, 1, 1, Arithmetic>;
def Minus          : Operator<"-",    7, 1, 1, Arithmetic>;
def Equal          : Operator<"=",    6, 1, 1, Comparison>;
def NotEqual       : Operator<"<>",   6, 1, 1, Comparison>;
def LessThan       : Operator<"<",    5, 1, 1, Comparison>;
def LessOrEqual    : Operator<"<=",   5, 1, 1, Comparison>;
def GreaterThan    : Operator<">",    5, 1, 1, Comparison>;
def GreaterOrEqual : Operator<">=",   5, 1, 1, Comparison>;
def LogicalAnd     : Operator<"AND",  4, 1, 1, Logical>;
def LogicalOr      : Operator<"OR",   3, 1, 1, Logical>;
def Assign         : Operator<"=",    2, 1, 1, Assignment>;
def ConditionAnd   : Operator<",",    1, 1, 1, Logical>;

// Keywords
def AlignOf   : Keyword<"ALIGNOF">;
def Any       : Keyword<"ANY">;
def Const     : Keyword<"CONST">;
def Continue  : Keyword<"CONTINUE">;
def Declare   : Keyword<"DECLARE">;
def Dim       : Keyword<"DIM">;
def Do        : Keyword<"DO">;
def Else      : Keyword<"ELSE">;
def End       : Keyword<"END">;
def Exit      : Keyword<"EXIT">;
def Extern    : Keyword<"EXTERN">;
def False     : Keyword<"FALSE">;
def For       : Keyword<"FOR">;
def Function  : Keyword<"FUNCTION">;
def If        : Keyword<"IF">;
def Import    : Keyword<"IMPORT">;
def Loop      : Keyword<"LOOP">;
def Next      : Keyword<"NEXT">;
def Null      : Keyword<"NULL">;
def Ptr       : Keyword<"PTR">;
def Ref       : Keyword<"REF">;
def Rem       : Keyword<"REM">;
def Return    : Keyword<"RETURN">;
def SizeOf    : Keyword<"SIZEOF">;
def Step      : Keyword<"STEP">;
def Sub       : Keyword<"SUB">;
def Then      : Keyword<"THEN">;
def To        : Keyword<"TO">;
def True      : Keyword<"TRUE">;
def TypeKw    : Keyword<"TYPE">;
def TypeOf    : Keyword<"TYPEOF">;
def Until     : Keyword<"UNTIL">;
def While     : Keyword<"WHILE">;

// Types
include "Type/Types.td"
