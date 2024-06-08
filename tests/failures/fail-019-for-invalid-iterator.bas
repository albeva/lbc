''------------------------------------------------------------------------------
'' Test invalid for iterator type
''
'' CHECK: __FILE__:8:5: error: FOR iterator type must be numeric, got ZSTRING
'' CHECK: for x = "a" to "z"
'' CHECK:     ^~~~~~~
''------------------------------------------------------------------------------
for x = "a" to "z"
next
