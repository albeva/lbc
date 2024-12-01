''------------------------------------------------------------------------------
'' fail-023-func-type-expects-ptr.bas
'' - test function type expects a trailing PTR
''
'' CHECK: __FILE__:9:48: error: FUNCTION type missing a trailing PTR
'' CHECK: type FuncPtr as (function (integer) as integer)
'' CHECK:                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
''------------------------------------------------------------------------------
type FuncPtr as (function (integer) as integer)
