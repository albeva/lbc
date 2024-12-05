''------------------------------------------------------------------------------
'' fail-036-ref-assign-null.bas
''
'' CHECK: __FILE__:8:24: error: Invalid implicit conversion 'ANY PTR' to 'INTEGER REF'
'' CHECK: dim i as integer ref = null
'' CHECK:                        ^~~~
''------------------------------------------------------------------------------
dim i as integer ref = null
