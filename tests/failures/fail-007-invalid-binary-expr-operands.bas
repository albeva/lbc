''------------------------------------------------------------------------------
'' Check that the error message is correct when trying to add incompatible types
''
'' CHECK: __FILE__:10:3: error: Binary operator '+' cannot be applied to operands of type 'INTEGER' and 'BOOL'
'' CHECK: i + b
'' CHECK: ~~^~~
''------------------------------------------------------------------------------
dim b = true
dim i = 0
i + b
