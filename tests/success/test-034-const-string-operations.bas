''------------------------------------------------------------------------------
'' test-034-const-string-operations.bas
''
'' CHECK: good
''------------------------------------------------------------------------------
import cstd

const hello = "hello"
const world = "world"
const message = if hello + " " + world = "hello world" then "good" else "bad"

puts message
