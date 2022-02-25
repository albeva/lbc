''------------------------------------------------------------------------------
'' test-024-typealias.bas
'' - alias a type to a identifier
''
'' CHECK:       10
''------------------------------------------------------------------------------
import cstd

type IntPtr as integer ptr
var i = 10
var ip As IntPtr = @i
printf "%d", *ip
