''------------------------------------------------------------------------------
'' test-026-forward-udt.bas
'' - forward declare UDT
''
'' CHECK: name = Albert
'' CHECK: age = 35
'' CHECK: score = 99
''------------------------------------------------------------------------------
import cstd

dim p As Player
p.name = "Albert"
p.age = 35
p.details.score = 99

printf "name = %s\n", p.name
printf "age = %d\n", p.age
printf "score = %d\n", p.details.score

Type Player
    name As ZString
    age As UInteger
    details As MyPlayerDetails
End Type

type MyPlayerDetails as PlayerDetails

Type PlayerDetails
    score As Integer
End Type
