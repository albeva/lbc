''------------------------------------------------------------------------------
'' Assigning to an undeclared identifier is an error.
''
'' CHECK: __FILE__:8:1: error: use of undeclared identifier FOO
'' CHECK: foo = 10
'' CHECK: ^~~
''------------------------------------------------------------------------------
foo = 10
