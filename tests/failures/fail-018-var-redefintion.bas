''------------------------------------------------------------------------------
'' Test variable redefinition
''
'' CHECK: __FILE__:9:5: error: Symbol 'FOO' is already defined
'' CHECK: dim foo as integer
'' CHECK:     ^~~
''------------------------------------------------------------------------------
dim foo as integer
dim foo as integer
