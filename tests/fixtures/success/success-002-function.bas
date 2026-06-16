''------------------------------------------------------------------------------
'' Call a user-defined function and print the result.
''
'' CHECK: 3 + 4 = 7
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

function add(a as integer, b as integer) as integer
    return a + b
end function

printf "3 + 4 = %ld\n", add(3, 4)
