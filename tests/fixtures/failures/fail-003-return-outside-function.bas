''------------------------------------------------------------------------------
'' RETURN is only valid inside a function or subroutine.
''
'' CHECK: __FILE__:8:1: error: RETURN outside of a function or subroutine
'' CHECK: return 5
'' CHECK: ^~~~~~~~
''------------------------------------------------------------------------------
return 5
