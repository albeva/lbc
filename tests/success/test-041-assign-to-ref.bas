''------------------------------------------------------------------------------
'' test-041-assign-to-ref.bas
'' - test declaring a ref
'' - test assigning to a ref
'' - test comparing a ref to a variable
''
'' CHECK: true
''------------------------------------------------------------------------------
import cstd

dim a as integer = 0
dim b as integer ref = a
b = 10

printf "%s\n", if a = b, b = 10 then "true" else "false"