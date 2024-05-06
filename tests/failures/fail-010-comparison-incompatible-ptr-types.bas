''------------------------------------------------------------------------------
'' Check error message when comparing incompatible pointer types
''
'' CHECK: __FILE__:10:12: error: Comparison operator '=' cannot be applied to operands of type 'INTEGER PTR' and 'DOUBLE PTR'
'' CHECK: dim r = ip = dp
'' CHECK:         ~~~^~~~
''------------------------------------------------------------------------------
dim ip as integer ptr
dim dp as double ptr
dim r = ip = dp
