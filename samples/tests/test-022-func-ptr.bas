''------------------------------------------------------------------------------
'' test-022-func-ptr.bas
'' - get & call func by ptr
''
'' CHECK:       hello, world!
''------------------------------------------------------------------------------
import cstd

sub say(msg as zstring)
    printf "hello, %s!", msg
end sub

dim sayp = @say
(*sayp)("world")
