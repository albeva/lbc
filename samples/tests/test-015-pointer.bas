''------------------------------------------------------------------------------
'' test-015-pointer.bas
'' - take address of
'' - dereference
''
'' CHECK: 10
''------------------------------------------------------------------------------
import cstd

dim i = 10
dim ip = @i
dim iv = *ip
printf "%d\n", iv
