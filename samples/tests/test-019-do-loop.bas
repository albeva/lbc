''------------------------------------------------------------------------------
'' test-019-do-loop.bas
'' - DO ... LOOP statement
'' - PRE, POST and NO conditions
'' - UNTIL and WHILE conditions
'' - EXIT / CONTINUE statements
''
'' CHECK:      1, 2, 3, 4
'' CHECK-NEXT: 1, 2, 3, 4
'' CHECK-NEXT: 1, 2, 3, 4
'' CHECK-NEXT: 1, 2, 3, 4
'' CHECK-NEXT: 2, 4, 6, 8, 10
'' CHECK-NEXT: 1, 2, 3, 4
''------------------------------------------------------------------------------
import cstd

'' post until
do dim i = 1
    if i > 1 then printf ", "
    printf "%d", i
    i = i + 1
loop until i = 5
printf "\n"

'' post while
do dim i = 1
    if i > 1 then printf ", "
    printf "%d", i
    i = i + 1
loop while i < 5
printf "\n"

'' pre until
do dim i = 1 until i = 5
    if i > 1 then printf ", "
    printf "%d", i
    i = i + 1
loop
printf "\n"

'' pre while
do dim i = 1 while i < 5
    if i > 1 then printf ", "
    printf "%d", i
    i = i + 1
loop
printf "\n"

'' do exit
do
    exit do
    printf "unreachable"
loop

'' do continue
do dim i = 1
    i = i + 1

    if i mod 2 <> 0 then continue do
    if i > 10 then exit

    if i > 2 then printf ", "
    printf "%d", i
loop
printf "\n"

'' One liner
function advance(i as integer) as integer
    printf "%d, ", i
    return i + 1
end function
do dim i = 1 while i < 5 => i = advance(i)
