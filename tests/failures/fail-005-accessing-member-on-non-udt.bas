''------------------------------------------------------------------------------
'' Accessing member on a non UDT type
''
'' CHECK: __FILE__:12:8: error: Accessing a member on 'INTEGER' which is not a user defined type
'' CHECK: foo.bar.zoo = 10
'' CHECK: ~~~~~~~^~~~
''------------------------------------------------------------------------------
type TFoo
    bar as integer
end type
dim foo as TFoo
foo.bar.zoo = 10
