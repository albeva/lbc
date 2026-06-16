'' SKIP: explicit function main collides with generated main
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
extern "C" declare function printf(fmt as zstring, ...) as integer

function main() as integer
    say "Hello"
    return 0
end function

sub say(prefix as zstring)
    printf "%s, %s!\n", prefix, getSuffix()
end sub

function getSuffix() as zstring
    return "World"
end function

dim x = main()
