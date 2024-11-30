''------------------------------------------------------------------------------
'' fail-020-must-be-constant.bas
''
'' CHECK: __FILE__:9:7: error: Expected a constant expression when initialising CONST variable
'' CHECK: const bar = foo
'' CHECK:       ^~~
''------------------------------------------------------------------------------
dim foo = 1
const bar = foo
