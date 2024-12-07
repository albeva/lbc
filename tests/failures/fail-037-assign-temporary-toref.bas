''------------------------------------------------------------------------------
'' fail-037-assign-temporary-toref.bas
''
'' CHECK: __FILE__:8:5: error: Assigning non-addressable expression to reference variable 'I'
'' CHECK: dim i as integer ref = 10
'' CHECK:     ^                  ~~
''------------------------------------------------------------------------------
dim i as integer ref = 10
