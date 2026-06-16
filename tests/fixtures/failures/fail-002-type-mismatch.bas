''------------------------------------------------------------------------------
'' A string literal cannot initialise an INTEGER variable.
''
'' CHECK: __FILE__:8:20: error: cannot convert ZSTRING to INTEGER
'' CHECK: dim x as integer = "hello"
'' CHECK:                    ^~~~~~~
''------------------------------------------------------------------------------
dim x as integer = "hello"
