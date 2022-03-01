''------------------------------------------------------------------------------
'' test-025-forward-alias.bas
'' - forward declare type aliases
''
'' CHECK:       10
''------------------------------------------------------------------------------
import cstd

var i as MyInt = 10
var ip as MyIntPtr = @i
printf "%d\n", *ip

type MyIntPtr as MyInt Ptr
type MyInt as MyInt2
type MyInt2 as MyInt3
type MyInt3 as MyInt4
type MyInt4 as integer
