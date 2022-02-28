red='\033[31m'
green='\033[32m'
reset='\033[0m'

if grep -q microsoft <<< `uname -a`; then
    LBC=../../bin/lbc.exe
    FILECHECK=../../bin/toolchain/win64/bin/FileCheck.exe
    ECHO="echo -e"
elif grep -q Darwin <<< `uname -a`; then
    LBC=../../bin/lbc
    FILECHECK=FileCheck
    ECHO=echo
else
    LBC=../../bin/lbc
    FILECHECK=FileCheck
    ECHO="echo -e"
fi

#
# test files that should succeed
#
# ../lbc test-01.bas
# ./test-01 | FileCheck test-01.bas
for file in `ls test-*.bas`
do
    # the output file
    output=`basename $file .bas`
    # remove output file if it already exists
    if [ -e $output ]; then
        rm $output
    fi
    # compile
    $ECHO "$red\c"
    $LBC $file -o $output
    $ECHO "$reset\c"
    if [ -e $output ]; then
        $ECHO "$red\c"
        ./$output | $FILECHECK $file --dump-input=never
        if [ $? = 0 ]; then
            $ECHO "$reset\c"
            printf "%s%*s${green}Ok$reset\n" $file "$((30-${#file}))";
        else
            $ECHO "$reset\c"
        fi
        rm $output
    else
        printf "%s%*s${red}Failed$reset\n" $file "$((30-${#file}))";
    fi
done
