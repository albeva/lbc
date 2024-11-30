''------------------------------------------------------------------------------
'' test-036-is-expr.bas
''
'' CHECK: 1, 1, 1,
''------------------------------------------------------------------------------
import cstd

const hello = "hello"
check hello is zstring  ' true
type MyInt as integer
check MyInt is MyInt    ' true
check 1 + 2 is integer  ' true

sub check(res as bool)
    printf "%d, ", if res then 1 else 0
end sub
