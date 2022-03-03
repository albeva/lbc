dim f as Foo

dim a as typeof(Foo.value)	' integer
dim b as typeof(Foo.Nested)	' Foo.Nested
dim c as typeof(bar) ptr	' (function() as integer) ptr
dim d as typeof(bar())		' integer
dim e as typeof(f)			' Foo
dim f as typeof(f.name)		' zstring
dim g as typeof(a + 10)		' integer
dim h as typeof(g + 3.14)	' double

type Foo
	const value = 10
	name as zstring

	type Nested
	end type
end type

declare function bar() as integer
