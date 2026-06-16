''------------------------------------------------------------------------------
'' test-001-hello.bas
'' - extern C function
'' - alias attribute
'' - pass zstring argument
''
'' CHECK: Hello World!
''------------------------------------------------------------------------------
extern "C" declare sub puts(msg as zstring)
puts "Hello World!"
