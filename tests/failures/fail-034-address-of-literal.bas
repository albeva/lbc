''------------------------------------------------------------------------------
'' fail-034-address-of-literal.bas
'' - test can't take address of a literal
''
'' CHECK: __FILE__:9:26: error: Cannot take the address of value of type 'INTEGER'
'' CHECK: dim ip as integer ptr = @10
'' CHECK:                          ^~
''------------------------------------------------------------------------------
dim ip as integer ptr = @10
