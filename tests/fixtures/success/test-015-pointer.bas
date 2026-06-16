''------------------------------------------------------------------------------
'' test-015-pointer.bas
'' - take address of
'' - dereference
''
'' CHECK: 10
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i = 10
dim ip = @i
dim iv = *ip
printf "%d\n", iv
