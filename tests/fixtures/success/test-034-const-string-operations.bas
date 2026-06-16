'' SKIP: not yet supported: CONST declarations
''------------------------------------------------------------------------------
'' test-034-const-string-operations.bas
''
'' CHECK: good
''------------------------------------------------------------------------------
extern "C" declare sub puts(msg as zstring)

const hello = "hello"
const world = "world"
const message = if hello + " " + world = "hello world" then "good" else "bad"

puts message
