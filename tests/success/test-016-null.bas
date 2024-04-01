''------------------------------------------------------------------------------
'' test-016-null.bas
'' - assign null
'' - compare pointer to null
''
'' CHECK: true, true
''------------------------------------------------------------------------------
import cstd

dim ip as integer ptr = null

if ip = null then printf "true"
if ip <> null then printf "unreachable"

dim i = 10
ip = @i

if ip = null then printf "unreachable"
if ip <> null then printf ", true"
