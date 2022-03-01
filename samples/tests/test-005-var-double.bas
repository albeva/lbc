''------------------------------------------------------------------------------
'' test-005-var-double.bas
'' - declare floating point value
'' - pass floating point
''
'' CHECK: 3.141590
''------------------------------------------------------------------------------
import cstd

dim pi = 3.14159
printf "%lf", pi
