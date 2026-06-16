'' SKIP: not yet supported: FOR/DO loops
''------------------------------------------------------------------------------
'' test-017-flow.bas
'' - continue for statement
'' - exit for statement
''
'' CHECK: 1, 3
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

for i = 1 to 10
    if i mod 2 = 0 then continue
    if i = 5 then exit
    if i <> 1 then printf ", "
    printf "%d", i
next
