// Token definitions for lbc
// Processed by lbc-tblgen to generate TokenKinds.inc

class TokenCategory;

def Special  : TokenCategory;
def Punct    : TokenCategory;
def Operator : TokenCategory;
def Keyword  : TokenCategory;

class Token<string spelling, TokenCategory category> {
    string Spelling = spelling;
    TokenCategory Category = category;
}

class Keyword<string spelling>  : Token<spelling, Keyword>;
class Operator<string spelling> : Token<spelling, Operator>;
class Punct<string spelling>    : Token<spelling, Punct>;

// Special
def Invalid : Token<"Invalid",     Special>;
def Eol     : Token<"End-Of-Line", Special>;
def Eof     : Token<"End-Of-File", Special>;

// Keywords
def Do   : Keyword<"DO">;
def If   : Keyword<"IF">;
def Then : Keyword<"THEN">;

// Operators
def Plus     : Operator<"+">;
def Minus    : Operator<"-">;
def Multiply : Operator<"*">;
def Divide   : Operator<"/">;
def Equal    : Operator<"=">;


