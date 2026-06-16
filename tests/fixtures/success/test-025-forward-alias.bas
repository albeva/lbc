'' SKIP: not yet supported: type aliases
''------------------------------------------------------------------------------
'' test-025-forward-alias.bas
'' - forward declare type aliases
''
'' CHECK: 10
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i as MyInt = 10
dim ip as MyIntPtr = @i
printf "%d\n", *ip

type MyIntPtr as MyInt Ptr
type MyInt as MyInt2
type MyInt2 as MyInt3
type MyInt3 as MyInt4
type MyInt4 as integer
