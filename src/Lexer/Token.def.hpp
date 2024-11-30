//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Type/Type.def.hpp"

// clang-format off

#define TOKEN_GENERAL(_) \
    _( Invalid,    "Invalid"     ) \
    _( EndOfFile,  "End-Of-File" ) \
    _( EndOfStmt,  "End-Of-Stmt" ) \
    _( Identifier, "Identifier"  )

#define TOKEN_LITERALS(_) \
    _( StringLiteral,        "String-Literal"         ) \
    _( IntegerLiteral ,      "Integer-Literal"        ) \
    _( FloatingPointLiteral, "Floating-Point-Literal" ) \
    _( BooleanLiteral,       "Boolean-Literal"        ) \
    _( NullLiteral,          "Null-Literal"           )

#define TOKEN_SYMBOLS(_) \
    _( Comma,        ","   ) \
    _( ParenClose,   ")"   ) \
    _( BracketOpen,  "["   ) \
    _( BracketClose, "]"   ) \
    _( Ellipsis,     "..." ) \
    _( LambdaBody,   "=>"  )

#define TOKEN_OPERATORS(_) \
    /* ID               Str     Prec    Type    Assoc   Kind       */ \
    _( ParenOpen,       "(",    13,     Unary,  Right,  Call        ) \
                                                                      \
    _( MemberAccess,    ".",    12,     Binary, Left,   Memory      ) \
                                                                      \
    _( AddressOf,       "@",    11,     Unary,  Left,   Memory      ) \
    _( Dereference,     "*",    11,     Unary,  Left,   Memory      ) \
                                                                      \
    _( Negate,          "-",    10,     Unary,  Left,   Arithmetic  ) \
    _( LogicalNot,      "NOT",  10,     Unary,  Left,   Logical     ) \
                                                                      \
    _( Multiply,        "*",    9,      Binary, Left,   Arithmetic  ) \
    _( Divide,          "/",    9,      Binary, Left,   Arithmetic  ) \
                                                                      \
    _( Modulus,         "MOD",  8,      Binary, Left,   Arithmetic  ) \
                                                                      \
    _( Plus,            "+",    7,      Binary, Left,   Arithmetic  ) \
    _( Minus,           "-",    7,      Binary, Left,   Arithmetic  ) \
                                                                      \
    _( Equal,           "=",    6,      Binary, Left,   Comparison  ) \
    _( NotEqual,        "<>",   6,      Binary, Left,   Comparison  ) \
                                                                      \
    _( LessThan,        "<",    5,      Binary, Left,   Comparison  ) \
    _( LessOrEqual,     "<=",   5,      Binary, Left,   Comparison  ) \
    _( GreaterThan,     ">",    5,      Binary, Left,   Comparison  ) \
    _( GreaterOrEqual,  ">=",   5,      Binary, Left,   Comparison  ) \
                                                                      \
    _( Is,              "IS",   5,      Unary,  Right,  Comparison  ) \
    _( As,              "AS",   5,      Unary,  Right,  Cast        ) \
                                                                      \
    _( LogicalAnd,      "AND",  4,      Binary, Left,   Logical     ) \
                                                                      \
    _( LogicalOr,       "OR",   3,      Binary, Left,   Logical     ) \
                                                                      \
    _( Assign,          "=",    2,      Binary, Left,   Assignment  ) \
                                                                      \
    _( ConditionAnd,    ",",    1,      Binary, Left,   Logical     )

#define TOKEN_KEYWORDS(_) \
    _( Any,      "ANY"      ) \
    _( Const,    "CONST"    ) \
    _( Continue, "CONTINUE" ) \
    _( Declare,  "DECLARE"  ) \
    _( Dim,      "DIM"      ) \
    _( Do,       "DO"       ) \
    _( Else,     "ELSE"     ) \
    _( End,      "END"      ) \
    _( Exit,     "EXIT"     ) \
    _( Extern,   "EXTERN"   ) \
    _( False,    "FALSE"    ) \
    _( For,      "FOR"      ) \
    _( Function, "FUNCTION" ) \
    _( If,       "IF"       ) \
    _( Import,   "IMPORT"   ) \
    _( Loop,     "LOOP"     ) \
    _( Next,     "NEXT"     ) \
    _( Null,     "NULL"     ) \
    _( Ptr,      "PTR"      ) \
    _( Rem,      "REM"      ) \
    _( Return,   "RETURN"   ) \
    _( SizeOf,   "SIZEOF"   ) \
    _( Step,     "STEP"     ) \
    _( Sub,      "SUB"      ) \
    _( Then,     "THEN"     ) \
    _( To,       "TO"       ) \
    _( True,     "TRUE"     ) \
    _( Type,     "TYPE"     ) \
    _( TypeOf,   "TYPEOF"   ) \
    _( Until,    "UNTIL"    ) \
    _( While,    "WHILE"    )

#define TOKEN_OPERATOR_KEYWORD_MAP(_) \
    _( As         ) \
    _( Is         ) \
    _( LogicalNot ) \
    _( Modulus    ) \
    _( LogicalAnd ) \
    _( LogicalOr  )

// All tokens combined
#define ALL_TOKENS(_) \
    TOKEN_GENERAL(_)   \
    TOKEN_LITERALS(_)  \
    TOKEN_SYMBOLS(_)   \
    TOKEN_OPERATORS(_) \
    TOKEN_KEYWORDS(_)  \
    ALL_TYPES(_)
