''------------------------------------------------------------------------------
'' test-007-conversions.bas
'' - declare variables of different types
'' - assign different types
''
'' CHECK: 3.140000, 3
''------------------------------------------------------------------------------
import cstd

dim d as double = 3.14
dim s as single = d
dim i as integer = d

printf "%lf, %d", d, i
