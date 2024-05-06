''------------------------------------------------------------------------------
'' Check error message when comparing incompatible types
''
'' CHECK: __FILE__:10:11: error: Comparison operator '=' cannot be applied to operands of type 'BOOL' and 'INTEGER'
'' CHECK: dim r = b = i
'' CHECK:         ~~^~~
''------------------------------------------------------------------------------
dim b = true
dim i = 1
dim r = b = i
