'' SKIP: not yet supported: explicit AS cast
''------------------------------------------------------------------------------
'' test-014-cast.bas
'' - type cast
''
'' CHECK: 10.000000, 10
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i = 10
dim f = i as double
dim b = f as byte

printf "%lf, %hhi", f, b
