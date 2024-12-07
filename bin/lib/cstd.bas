extern "C"
    Declare Function printf(fmt As ZString, ...) As Integer
    Declare Function puts(str As ZString) As Integer
    Declare Function getchar() As Integer
    Declare Function scanf(fmt As ZString, ...) As Integer
    Declare Sub srand(seed As UInteger)
    Declare Function rand As Integer
    Declare Function time(time_t As Any Ptr) As ULong
    [alias="exit"] Declare Sub terminate(code As Integer)

    Const EXIT_SUCCESS = 0
    Const EXIT_FAILURE = 1
end extern
