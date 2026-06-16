'' SKIP: not yet supported: reference/address features
''------------------------------------------------------------------------------
'' test-043-ref-address-match-var-addr.bas
'' Test that the address of a reference and the variable it references are the same
''
'' CHECK: true
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i as integer = 0
dim iref as integer ref = i
printf "%s\m", if @iref = @i then "true" else "false"
