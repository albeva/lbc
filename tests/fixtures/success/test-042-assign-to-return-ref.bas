'' SKIP: not yet supported: assigning through a returned reference
''------------------------------------------------------------------------------
'' test-042-assign-to-return-ref.bas
'' - test assigning to return ref value
''
'' CHECK: 5
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i as integer
getRef() = 5
printf "%d\n", i

function getRef() as integer ref
    return i
end function
