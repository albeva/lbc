''------------------------------------------------------------------------------
'' Implement dynamic arrays
''------------------------------------------------------------------------------
import cstd

'' A generic, dynamically allocated array
''
'' This array is similar to std::vector in C++.
''
'' Usage:
''   Dim array = DynamicArray Of Integer
''   array.push 1
''   array.push 2
''   Print array[0]   '' prints 1
''   Print array[1]   '' prints 2
''   Print array.size '' prints 2
''   For i = 0 To array.size - 1
''       Print array[i]
''   Next i
Type DynamicArray Of T
    '' Pointer to the begining of T
    Private(set) Dim data As T Ptr
    '' Number of elements in the array
    Private(set) Dim size As UInteger
    '' Number of elements that can be stored in the array without reallocation
    Private(set) Dim capacity As UInteger
    '' Provide information about type T by the compiler
    Type Info As TypeInfo Of T

    '' Default constructor
    Constructor
        data     = null
        size     = 0
        capacity = 0
    End Constructor

    '' Copy constructor
    Copy Constructor ' copy As Const DynamicArray Ref
        If copy.isEmpty Then
            Constructor
        Else
            This.data = cstd.malloc(copy.capacity * Info.size)
            Info.copy This.data, copy.data, copy.size
            This.size = copy.size
        End If
    End Constructor

    '' Move constructor
    Move Constructor ' move As DynamicArray Ref
        This.data     = move.data
        This.size     = move.size
        This.capacity = move.capacity
        move.data     = null
        move.size     = 0
        move.capacity = 0
    End Constructor

    '' Clean up and release memory
    Destructor
        If data = null Then Return
        Info.destroy data, size
        cstd.free data
    End Destructor

    '' Check if the array is empty
    Property isEmpty As Bool => size = 0

    '' Access element at index
    Function [index As UInteger]() As T Ref
        Return data[index]
    End Function

    '' Set element at index
    Sub [index As UInteger](value As T)
        data[index] = value
    End Sub

    '' Get reference to the first element
    Property first As T Ref
        If size = 0 Then exit(1)
        Return data[0]
    End Property

    '' Get reference to the last element
    Property last As T Ref
        If size = 0 Then exit(1)
        Return data[size - 1]
    End Property

    '' Push new value to the back of the array
    Sub push(value As T)
        If size = capacity Then
            reserve Max(capacity * 2, 1)
        End If
        data[size] = value
        size += 1
    End Sub

    '' Remove the last element from the array
    Function pop() As T
        If size = 0 Then exit(1)
        size -= 1
        return Info.take(@data[size])
    End Function

    '' Copies content of an array to the end of this array
    Sub append(other As DynamicArray Ref)
        If size + other.size > capacity Then
            reserve size + other.size
        End If
        Info.copy @data[size], other.data, other.size
        size += other.size
    End Sub

    '' Add new element to the front of the array
    Sub pushFront(value As T)
        If size = capacity Then
            reserve Max(capacity * 2, 1)
        End If
        ' There might be double copy here if array was also resized, but is fine for now
        Info.move @data[1], data, size
        data[0] = value
        size += 1
    End Sub

    '' Remove the first element from the array
    Function popFront() As T
        If size = 0 Then exit(1)
        Dim result As T = Info.take(data)
        Info.move data, @data[1], size - 1
        size -= 1
        Return result
    End Function

    '' Insert element at given index
    Sub insert(index As UInteger, value As T)
        If size = capacity Then
            reserve Max(capacity * 2, 1)
        End If
        Info.move @data[index + 1], @data[index], size - index
        data[index] = value
        size += 1
    End Sub

    '' Removes an element at given index
    Sub remove(index As UInteger)
        Info.destroy(@data[index])
        Info.move @data[index], @data[index + 1], size - index - 1
        size -= 1
    End Sub

    '' Reserve memory for at least newCapacity elements
    Sub reserve(newCapacity As UInteger)
        '' no reducing the size
        If newCapacity <= capacity Then Return

        '' amount of memory in bytes we need to allocate
        Dim allocate = size * Info.size

        Const If Info.isTriviallyCopyable Then
            data = cstd.realloc(data, allocate)
        Else If allocate = Then
            data = cstd.realloc(data, allocate)
        Else
            Dim newData As T Ptr = cstd.malloc(allocate)
            Info.move newData, data, size
            cstd.free data
            data = newData
        End If
        capacity = newCapacity
    End Sub
End Type
