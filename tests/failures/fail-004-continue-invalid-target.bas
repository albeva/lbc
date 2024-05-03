''------------------------------------------------------------------------------
'' Continue with invalid target
''
'' CHECK: __FILE__:9:14: error: Unexpected CONTINUE target 'FOR'
'' CHECK:     continue for
'' CHECK:              ^~~
''------------------------------------------------------------------------------
do
    continue for
loop
