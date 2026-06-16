'' SKIP: not yet supported: type aliases
''------------------------------------------------------------------------------
'' test-024-typealias.bas
'' - alias a type to a identifier
''
'' CHECK: 10
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

type IntPtr as integer ptr
dim i = 10
dim ip As IntPtr = @i
printf "%d", *ip
