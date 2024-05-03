''------------------------------------------------------------------------------
'' test-031-do-for-continuation.bas
'' - check exit do and continue for do
''
'' CHECK: 1, 2, 3, 1, 2, 3
''------------------------------------------------------------------------------
import cstd

do dim i = 0
    i = i + 1
    for x = 1 to 5
        if i = 3 then exit do
        if i > 1 or x > 1 then printf ", "
        printf "%d", x
        if x = 3 then continue for do
    next
loop
