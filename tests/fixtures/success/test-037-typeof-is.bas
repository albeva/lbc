'' SKIP: not yet supported: IS / TYPEOF
''------------------------------------------------------------------------------
'' test-037-typeof-is.bas
''
'' CHECK: 1, 1, 0, 1, 1, 1, 1,
''------------------------------------------------------------------------------
extern "C" declare function printf(fmt as zstring, ...) as integer

check typeof(1) is integer              ' true
check typeof("hello") is zstring        ' true
check typeof(integer) is typeof(bool)   ' false

type MyInt as integer
check typeof(MyInt) is integer          ' true
check MyInt is typeof(MyInt)            ' true
check typeof(1 + 2) is integer          ' true
check typeof(1 = 2) is bool             ' true

sub check(res as bool)
    printf "%d, ", if res then 1 else 0
end sub
