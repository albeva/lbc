''------------------------------------------------------------------------------
'' test-035-const-conversions.bas
''
'' CHECK: 6.0
''------------------------------------------------------------------------------
import cstd

const b as ubyte = 1
const c as ushort = 2
const d as double = 3.0
const e = b + c + d

printf "%0.1lf", e
