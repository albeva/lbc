''------------------------------------------------------------------------------
'' test-009-if-expr.bas
'' - if expression
'' - comparisons
''
'' CHECK: true, 0, -1, 3.140000
''------------------------------------------------------------------------------
import cstd

dim b = true
dim s = if b then "true" else "false"
dim i1 = if not b then 1 else 0
dim i2 = if i1 = 0 then -1 else 1
dim d1 = (if (i2 < 0) then (3.14) else (6.28))

printf "%s, %d, %d, %lf\n", s, i1, i2, d1
