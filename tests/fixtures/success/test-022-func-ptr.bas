'' SKIP: not yet supported: function pointers
''------------------------------------------------------------------------------
'' test-022-func-ptr.bas
'' - get & call func by ptr
''
'' CHECK: hello, world!
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

sub say(msg as zstring)
    printf "hello, %s!", msg
end sub

dim sayp = @say
(*sayp)("world")
