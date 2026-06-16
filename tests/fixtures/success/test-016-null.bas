'' SKIP: not yet supported: IF
''------------------------------------------------------------------------------
'' test-016-null.bas
'' - assign null
'' - compare pointer to null
''
'' CHECK: true, true
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim ip as integer ptr = null

if ip = null then printf "true"
if ip <> null then printf "unreachable"

dim i = 10
ip = @i

if ip = null then printf "unreachable"
if ip <> null then printf ", true"
