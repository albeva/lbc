''------------------------------------------------------------------------------
'' test-020-type-ptr.bas
'' - Pass type by ptr
''
'' CHECK:       name = Albert
'' CHECK-NEXT:  age = 36
''------------------------------------------------------------------------------
import cstd

Type Player
    name As ZString
    age As UInteger
End Type

Sub foo(p As Player Ptr)
    p.name = "Albert"
    p.age = 36
End Sub

Var p As Player
foo(@p)

printf "name = %s\n", p.name
printf "age = %d\n", p.age
