''------------------------------------------------------------------------------
'' Accessing member on a non UDT type
''
'' CHECK: __FILE__:9:1: error: Accessing a member on 'INTEGER' which is not a user defined type
'' CHECK: foo.bar = 20
'' CHECK: ^~~~~~~
''------------------------------------------------------------------------------
dim foo = 0
foo.bar = 20
