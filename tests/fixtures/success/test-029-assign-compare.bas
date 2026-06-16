'' SKIP: not yet supported: chained assignment/comparison
''------------------------------------------------------------------------------
'' test-029-assign-compare.bas
'' - test assignment and equality compare operator
''
'' CHECK: true
'' CHECK: true
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim i = 7
dim b1 as bool
b1 = i = 7
dump b1

dim b2 as bool
b2 = 7 = 7 = true
dump b2

sub dump(b as bool)
    printf "%s\n", if b then "true" else "false"
end sub
