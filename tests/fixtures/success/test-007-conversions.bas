''------------------------------------------------------------------------------
'' test-007-conversions.bas
'' - declare variables of different types
'' - assign different types
''
'' CHECK: 3.140000, 3
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim d as double = 3.14
dim s as single = d as single
dim i as integer = d as integer

printf "%lf, %d", d, i
