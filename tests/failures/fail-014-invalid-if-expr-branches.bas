''------------------------------------------------------------------------------
'' Check error message when if expr is non boolean
''
'' CHECK: __FILE__:8:9: error: Mismatching types in IF expression branches 'INTEGER' and 'DOUBLE'
'' CHECK: dim r = if true then 1 else 0.0
'' CHECK:         ^~~~~~~~~~~~~~~~~~~~~~~
''------------------------------------------------------------------------------
dim r = if true then 1 else 0.0
