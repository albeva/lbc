Import cstd

Sub assert(cond As Bool, msg As ZString)
    If Not cond Then
        printf "Assertion failed: %s\n", msg
        terminate EXIT_FAILURE
    End If
End Sub
