''------------------------------------------------------------------------------
'' Test undefined variable
''
'' CHECK: __FILE__:8:1: error: Unknown identifier 'FOO'
'' CHECK: foo = 10
'' CHECK: ^~~
''------------------------------------------------------------------------------
foo = 10
