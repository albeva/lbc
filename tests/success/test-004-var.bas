''------------------------------------------------------------------------------
'' test-004-var.bas
'' - declare global variable
'' - declare local variable
'' - assign variables
'' - pass variables as arguments
''
'' CHECK: Hello, World!
''------------------------------------------------------------------------------
import cstd

dim a = "Hello"
dim b = "World"
dim c = a
sendMessage c

sub sendMessage(greeting as zstring)
    dim exclamation = getExclamation()
    printf "%s, %s%s", greeting, b, exclamation
end sub

function getExclamation() as zstring
    return "!"
end function
