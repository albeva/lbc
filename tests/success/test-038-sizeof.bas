''------------------------------------------------------------------------------
'' test-038-sizeof.bas
''
'' CHECK: ints:   1, 1, 2, 2, 4, 4, 8, 8
'' CHECK: floats: 4, 8
'' CHECK: misc:   8, 1, 8
'' CHECK: udt:    24, 24, 4, 8, 1, 13, 13
''------------------------------------------------------------------------------
import cstd
dim first = true

rem integer types
printf "ints:   "
check sizeof(byte)
check sizeof(ubyte)
check sizeof(short)
check sizeof(ushort)
check sizeof(integer)
check sizeof(uinteger)
check sizeof(long)
check sizeof(ulong)

rem floating point types
newLine "floats: "
check sizeof(single)
check sizeof(double)

newLine "misc:   "
check sizeof(zstring)
check sizeof(bool)
check sizeof(null)

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
check sizeof(Foo)
check sizeof(myFoo)
check sizeof(myFoo.a)
check sizeof(myFoo.b)
check sizeof(myFoo.c)

check sizeof(Bar)
check sizeof(myBar)
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
