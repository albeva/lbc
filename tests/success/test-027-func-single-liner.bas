''------------------------------------------------------------------------------
'' test-027-func-single-liner.bas
'' - function / sub declared on single line
''
'' CHECK: 42
''------------------------------------------------------------------------------
import cstd

function foo() as integer => 42
sub printInt(i as integer) => printf "%d", i

printf "%d", foo()