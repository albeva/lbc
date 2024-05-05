''------------------------------------------------------------------------------
'' Check that error message is correct when accessing non-existing UDT member
''
'' CHECK: __FILE__:14:5: error: Unknown identifier 'FOO'
'' CHECK: bar.foo = 10
'' CHECK:     ^~~
''------------------------------------------------------------------------------
dim foo = 0
type TBar
    x as integer
end type

dim bar as TBar
bar.foo = 10
