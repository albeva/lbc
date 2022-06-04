''------------------------------------------------------------------------------
'' test-030-func-type-alias.bas
'' - typeof function
''
'' CHECK:       42
''------------------------------------------------------------------------------
import cstd

type FooP as typeof(foo) ptr
dim fp as FooP = @foo
printf "%d\n", (*fp)()

function foo() as integer => 42
