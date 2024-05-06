''------------------------------------------------------------------------------
'' Check error message for invalid operands to logical operation
''
'' CHECK: __FILE__:10:3: error: Binary operator 'AND' cannot be applied to operands of type 'ZSTRING' and 'DOUBLE'
'' CHECK: s and d
'' CHECK: ~~^~~~~
''------------------------------------------------------------------------------
dim s = ""
dim d = 1.0
s and d
