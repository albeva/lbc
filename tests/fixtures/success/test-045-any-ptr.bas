''------------------------------------------------------------------------------
'' test-045-any-ptr.bas
'' - ANY PTR accepts any typed pointer implicitly
'' - typed pointer recovered from ANY PTR via cast
''
'' CHECK: 42
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

dim x = 42
dim ap as any ptr = null
ap = @x                                        '' implicit typed-ptr -> any ptr
dim back as integer ptr = ap as integer ptr    '' explicit any ptr -> typed ptr
printf "%ld", *back
