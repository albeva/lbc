import cstd

'' Declare vars
dim secret as integer
dim answer as integer

'' initialize a random seed
srand time(null)

'' Get the random number
secret = rand() Mod 100

'' show help
printf "Guess a number between 1 and 100. 0 to exit. You have 25 tries\n"

'' Try for 25 times
For i = 1 To 25
    printf "Attempt %d: ", i
    scanf "%d", @answer
    If answer = 0 Then
        Exit For
    Else If answer = secret Then
        printf "Correct\n"
        Return
    Else If answer > secret Then
        printf "Nope. Too big!\n"
    Else
        printf "Nooo. Try bigger!\n"
    End If
Next

'' too bad
printf "Game Over. Correct number was %d\n", secret
