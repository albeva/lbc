''------------------------------------------------------------------------------
'' Check error message when if expr is non boolean
''
'' CHECK: __FILE__:8:12: error: No viable conversion from 'ZSTRING' to 'BOOL'
'' CHECK: dim r = if "" then 1 else 0
'' CHECK:            ^~
''------------------------------------------------------------------------------
dim r = if "" then 1 else 0
