''------------------------------------------------------------------------------
'' fail-020-must-be-constant.bas
''
'' CHECK: __FILE__:9:11: error: Expression must be constant
'' CHECK: dim bar = foo + " world"
'' CHECK:           ^~~~~~~~~~~~~~
''------------------------------------------------------------------------------
dim foo = "hello"
dim bar = foo + " world"
