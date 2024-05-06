''------------------------------------------------------------------------------
'' Check error message when implicitly converting to incompatible type
''
'' CHECK: __FILE__:8:17: error: Invalid implicit conversion 'ZSTRING' to 'BOOL'
'' CHECK: dim b as bool = ""
'' CHECK:                 ^~
''------------------------------------------------------------------------------
dim b as bool = ""
