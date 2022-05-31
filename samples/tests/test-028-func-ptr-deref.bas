''------------------------------------------------------------------------------
'' test-028-func-ptr-deref.bas
'' - typedef function ptr
'' - single liner functions
'' - deref a func ptr
'' - chained call
''
'' CHECK:       42
''------------------------------------------------------------------------------
import cstd

type Func as (function() as integer) ptr
function foo() as integer => 42
function get() as Func => @foo

printf "%d", (*get())()
