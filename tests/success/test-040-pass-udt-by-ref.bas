''------------------------------------------------------------------------------
'' test-039-alignof.bas
''
'' CHECK: kind:   10
'' CHECK: lexeme: hello
'' CHECK: kind:   10
'' CHECK: lexeme: hello
''------------------------------------------------------------------------------
import cstd

Dim t As TToken
dump init(t)
dump t

type TToken
    kind as integer
    lexeme as zstring
end type

function init(t as TToken ref) as TToken ref
    t.kind = 10
    t.lexeme = "hello"
    return t
end function

sub dump(t as TToken Ref)
    printf "kind:   %d\n", t.kind
    printf "lexeme: %s\n", t.lexeme
end sub