''------------------------------------------------------------------------------
'' fail-022-expected-type.bas
'' - test unexpected strign literal where a type is required
''
'' CHECK: __FILE__:9:10: error: expected type expression, got '"hello"'
'' CHECK: dim s as "hello"
'' CHECK:          ^~~~~~~
''------------------------------------------------------------------------------
dim s as "hello"
