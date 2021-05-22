''------------------------------------------------------------------------------
'' test-016-null.bas
'' - assign null
'' - compare pointer to null
''
'' CHECK: true, true
''------------------------------------------------------------------------------
[alias="printf"] _
declare function printf(fmt as zstring, ...) as integer

var ip as integer ptr = null

if ip = null then printf "true"
if ip <> null then printf "unreachable"

var i = 10
ip = @i

if ip = null then printf "unreachable"
if ip <> null then printf ", true"
