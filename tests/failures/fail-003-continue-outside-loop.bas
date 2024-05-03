''------------------------------------------------------------------------------
'' Continue outside of FOR/DO loop
''
'' CHECK: __FILE__:8:1: error: CONTINUE not allowed outside FOR or DO loops
'' CHECK: continue
'' CHECK: ^~~~~~~~
''------------------------------------------------------------------------------
continue
