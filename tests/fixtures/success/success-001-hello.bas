''------------------------------------------------------------------------------
'' printf with a string and a global integer.
''
'' CHECK: Hello world! x = 42
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer
dim x = 42
printf "Hello %s! x = %ld\n", "world", x
