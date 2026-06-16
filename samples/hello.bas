extern "C" declare function printf(fmt as zstring, ...) as integer
dim x = 42
printf "Hello %s! x = %d", "world", x
