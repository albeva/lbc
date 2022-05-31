''------------------------------------------------------------------------------
'' test-029-assign-compare.bas
'' - test assignment and equality compare operator
''
'' CHECK:       true
'' CHECK-NEXT:  true
''------------------------------------------------------------------------------
import cstd

dim i = 7
dim b1 as bool
b1 = i = 7
dump b1

dim b2 as bool
b2 = 7 = 7 = true
dump b2

sub dump(b as bool) => printf "%s\n", if b then "true" else "false"
