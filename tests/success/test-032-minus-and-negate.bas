''------------------------------------------------------------------------------
'' test-032-minus-and-negate.bas
'' - minus and negation are parsed correctly
''
'' CHECK: -1, -2, 1, 0, 0, -3, 1, -1, 1
''------------------------------------------------------------------------------
import cstd

dim i = -1              ' -1
dim j = i - 1           ' -2
dim k = -i              ' 1
dim l =  i - -1         ' 0
dim m = -i - 1          ' 0
dim n = i - -i - 1      ' -3
dim o = -(i - 1) - -i   ' 1
dim p = - 1             ' -1
dim q = - - 1           ' 1

printf "%d, %d, %d, %d, %d, %d, %d, %d, %d", i, j, k, l, m, n, o, p, q
