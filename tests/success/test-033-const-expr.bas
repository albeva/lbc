''------------------------------------------------------------------------------
'' test-0330-const-expr.bas
'' Test constant expressions
''
'' CHECK: good
''------------------------------------------------------------------------------
import cstd

const foo = true
const bar = false
const message = if foo <> bar and foo = true and false = bar then "good" else "bad"

puts message
