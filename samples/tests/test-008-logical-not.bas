''------------------------------------------------------------------------------
'' test-008-logical-not.bas
'' - unary NOT to bool literal
'' - unary NOT to bool variable
''
'' CHECK: 0, 1, 0, 1
''------------------------------------------------------------------------------
import cstd

dim f = not true
dim t = not f
dim ff = not not false
dim tt = not not t

dim int_f = if f then 1 else 0
dim int_t = if t then 1 else 0
dim int_ff = if ff then 1 else 0
dim int_tt = if tt then 1 else 0
printf "%d, %d, %d, %d", int_f, int_t, int_ff, int_tt