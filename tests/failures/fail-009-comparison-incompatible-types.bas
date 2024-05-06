''------------------------------------------------------------------------------
'' Check error message when comparing incompatible types
''
'' CHECK: __FILE__:10:3: error: Binary operator 'AND' cannot be applied to operands of type 'ZSTRING' and 'DOUBLE'
'' CHECK: s and d
'' CHECK: ~~^~~~~
''------------------------------------------------------------------------------
dim b = true
dim i = 1
dim r = b = i
