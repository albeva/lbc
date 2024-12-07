''
'' Print
''
import cstd

Const Sub print(args As Any...)
    Dim format As StaticString = ""
    Dim varArgs As ...
    For arg In args
        If arg Is Integer Then
            format += "%d"
            varArgs += arg
        Else If arg Is Bool Then
            format += "%s"
            varArgs += if arg Then "true" Else "false"
        Else If arg Is ZString Then
            format += "%s"
            varArgs += arg
        End If
    Next
    format += "\n"

    printf format, varArgs
End Sub
