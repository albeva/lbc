'' SKIP: not yet supported: type aliases
''------------------------------------------------------------------------------
'' test-030-func-type-alias.bas
'' - typeof function
'' - typeof function result
'' - forward referencing
''
'' CHECK: 42
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

type FooP as typeof(foo) ptr
type FooT as typeof(foo())

dim fun as FooP = @foo
dim res as FooT = (*fun)()

printf "%d\n", res

function foo() as integer
    return 42
end function
