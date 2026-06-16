''------------------------------------------------------------------------------
'' puts writes a line to stdout.
''
'' CHECK: hello via puts
''------------------------------------------------------------------------------
extern "C" declare sub puts(msg as zstring)
puts "hello via puts"
