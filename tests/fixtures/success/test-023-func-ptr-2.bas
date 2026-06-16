'' SKIP: not yet supported: function pointers
''------------------------------------------------------------------------------
'' test-023-func-ptr-2.bas
'' - get & call func by ptr
''
'' CHECK: hello, world!
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

function getMessage as zstring
    return "world"
end function

sub say(getter as (function() as zstring) ptr)
    printf "hello, %s!\n", (*getter)()
end sub

dim sayp as sub((function as zstring) ptr) ptr = @say
(*sayp)(@getMessage)
