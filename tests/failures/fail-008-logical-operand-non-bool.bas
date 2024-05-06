''------------------------------------------------------------------------------
'' Check error message for invalid operands to logical operation
''
'' CHECK: __FILE__:10:11: error: Binary operator '=' cannot be applied to operands of type 'BOOL' and 'INTEGER'
'' CHECK: dim r = b = i
'' CHECK:         ~~^~~
''------------------------------------------------------------------------------
dim s = ""
dim d = 1.0
s and d
