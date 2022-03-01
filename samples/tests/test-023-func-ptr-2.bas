''------------------------------------------------------------------------------
'' test-023-func-ptr-2.bas
'' - get & call func by ptr
''
'' CHECK:       hello, world!
''------------------------------------------------------------------------------
import cstd

function getMessage as zstring
    return "world"
end function

sub say(getter as (function() as zstring) ptr)
    printf "hello, %s!\n", (*getter)()
end sub

dim sayp as sub(function as zstring) = @say
(*sayp)(@getMessage)
