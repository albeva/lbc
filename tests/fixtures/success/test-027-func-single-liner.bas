'' SKIP: not yet supported: single-line functions
''------------------------------------------------------------------------------
'' test-027-func-single-liner.bas
'' - function / sub declared on single line
''
'' CHECK: 42
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer
extern "C" declare sub puts(msg as zstring)
function foo() as integer => 42
sub printInt(i as integer) => printf "%d", i

printf "%d", foo()