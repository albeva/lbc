'dim a as typeof(Foo.value)  ' integer
'dim b as typeof(Foo.Nested) ' Foo.Nested
dim c as typeof(bar) ptr    ' (function() as integer) ptr
dim d as typeof(bar())      ' integer
dim e as typeof(Foo)        ' Foo
dim f as typeof(e)          ' Foo
dim g as typeof(f.name)     ' zstring
dim h as typeof(d + 10)     ' integer
dim i as typeof(d + 3.14)   ' double

type Foo
    'const value = 10
    name as zstring
    'type Nested
    'end type
end type

function bar() as integer
end function
