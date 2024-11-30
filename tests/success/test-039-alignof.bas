''------------------------------------------------------------------------------
'' test-039-alignof.bas
''
'' CHECK: ints:   1, 1, 2, 2, 4, 4, 8, 8
'' CHECK: floats: 4, 8
'' CHECK: misc:   8, 1, 8
'' CHECK: udt:    8, 8, 4, 8, 1, 8, 8
''------------------------------------------------------------------------------
import cstd
dim first = true

rem integer types
printf "ints:   "
check alignof(byte)
check alignof(ubyte)
check alignof(short)
check alignof(ushort)
check alignof(integer)
check alignof(uinteger)
check alignof(long)
check alignof(ulong)

rem floating point types
newLine "floats: "
check alignof(single)
check alignof(double)

newLine "misc:   "
check alignof(zstring)
check alignof(bool)
check alignof(null)

rem UDT
type Foo
    a as integer
    b as double
    c as bool
end type

[packed] _
type Bar
    a as integer
    b as double
    c as bool
end type
Dim myFoo as Foo
dim myBar as Bar

newLine "udt:    "
check alignof(Foo)
check alignof(myFoo)
check alignof(myFoo.a)
check alignof(myFoo.b)
check alignof(myFoo.c)

check alignof(Bar)
check alignof(myBar)
printf "\n"

sub check(size as integer)
    if first then
        first = false
    else
        printf ", "
    end if
    printf "%d", size
end sub

sub newLine(cat as zstring)
    first = true
    printf "\n%s", cat
end sub
