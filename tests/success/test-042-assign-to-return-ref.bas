''------------------------------------------------------------------------------
'' test-042-assign-to-return-ref.bas
'' - test assigning to return ref value
''
'' CHECK: 5
''------------------------------------------------------------------------------
import cstd

dim i as integer
getRef() = 5
printf "%d\n", i

function getRef() as integer ref
    return i
end function
