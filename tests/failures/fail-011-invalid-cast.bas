''------------------------------------------------------------------------------
'' Check error message casting to incompatible type
''
'' CHECK: __FILE__:9:1: error: Invalid cast from 'ZSTRING' to 'BOOL'
'' CHECK: z as bool
'' CHECK: ^~~~~~~~~
''------------------------------------------------------------------------------
dim z = ""
z as bool