''------------------------------------------------------------------------------
'' test-044-assign-ptr-deref-to-ref.bas
'' Test assigning pointer derreferenced value to reference
''
'' CHECK: 42, 5, 5
''------------------------------------------------------------------------------
import cstd

dim a as integer = 5
dim b as integer = 10
dim p as integer ptr = @a
dim r as integer ref = b
r = *getPtr()
*p = 42

printf "%d, %d, %d\n", a, b, r

function getPtr() as integer ptr
    return p
end function
