// Token definitions for lbc
// Processed by lbc-tblgen to generate TokenKinds.inc

// Define token groups
class Group;
def General  : Group;
def Literal  : Group;
def Symbol   : Group;
def Operator : Group;
def Keyword  : Group;

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
class Literal<string str> : Token<str, Literal>;
class Symbol<string str> : Token<str, Symbol>;
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

// General
def Invalid : General<"Invalid">;
def Eol     : General<"End-Of-Line">;
def Eof     : General<"End-Of-File">;

// Operators
def Plus     : Operator<"+", 7, 1, 1, Arithmetic>;
def Minus    : Operator<"-", 7, 1, 1, Arithmetic>;
def Multiply : Operator<"*", 9, 1, 1, Arithmetic>;
def Divide   : Operator<"/", 9, 1, 1, Arithmetic>;
def Equal    : Operator<"=", 6, 1, 1, Assignment>;

// Keywords
def Do   : Keyword<"DO">;
def If   : Keyword<"IF">;
def Then : Keyword<"THEN">;
