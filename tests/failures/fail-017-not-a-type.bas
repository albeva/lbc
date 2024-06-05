''------------------------------------------------------------------------------
'' Test not a type
''
'' CHECK: __FILE__:9:12: error: 'FOO' is not a type
'' CHECK: dim bar as foo
'' CHECK:            ^~~
''------------------------------------------------------------------------------
dim foo as integer
dim bar as foo
