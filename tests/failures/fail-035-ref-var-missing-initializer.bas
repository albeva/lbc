''------------------------------------------------------------------------------
'' fail-035-ref-var-missing-initializer.bas
'' - test missing initializer for ref var
''
'' CHECK: __FILE__:9:5: error: Declaration of reference variable 'I' requires an initializer
'' CHECK: dim i as integer ref
'' CHECK: ~~~~^~~~~~~~~~~~~~~~
''------------------------------------------------------------------------------
dim i as integer ref
