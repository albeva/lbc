''------------------------------------------------------------------------------
'' test-003-functions.bas
'' - explicit main function
'' - declare local functions
'' - call functions
'' - pass arguments
'' - return getLlvmValue
''
'' CHECK: Hello, World!
''------------------------------------------------------------------------------
import cstd

function main as integer
    say "Hello"
    return 0
end function

sub say(prefix as zstring)
    printf "%s, %s!\n", prefix, getSuffix()
end sub

function getSuffix() as zstring
    return "World"
end function
