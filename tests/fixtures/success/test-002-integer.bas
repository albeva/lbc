''------------------------------------------------------------------------------
'' test-002-integer.bas
'' - variadic function
'' - pass zstring and an integer as argument
''
'' CHECK: 42
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer
extern "C" declare sub puts(msg as zstring)
printf "%d", 42
