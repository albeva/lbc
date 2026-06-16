''------------------------------------------------------------------------------
'' test-005-var-double.bas
'' - declare floating point value
'' - pass floating point
''
'' CHECK: 3.141590
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim pi = 3.14159
printf "%lf", pi
