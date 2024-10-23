@echo off

set VERSION=llvm-project-19.1.1

set SRC=%cd%\%VERSION%.src\llvm
set BUILD=%cd%\%VERSION%.build
set INSTALL=%cd%\%VERSION%.dist

if not exist "%BUILD%" mkdir %BUILD%

cmake -G "Ninja" -S "%SRC%" -B "%BUILD%" ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DLLVM_ENABLE_ASSERTIONS=ON ^
    -DLLVM_TARGETS_TO_BUILD=host ^
    -DLLVM_OPTIMIZED_TABLEGEN=ON ^
    -DLLVM_INSTALL_UTILS=ON ^
    -DLLVM_INCLUDE_BENCHMARKS=OFF ^
    -DLLVM_INCLUDE_TESTS=OFF ^
    && cmake --build "%BUILD%" --target install ^
    && setx LLVM_DIR "%INSTALL%\lib\cmake\llvm"

if defined LBC_DIR if %errorlevel% == 0 (
    xcopy /y "%BUILD%\bin\llc.exe" "%LBC_DIR%\bin\toolchain\win64\bin"
    xcopy /y "%BUILD%\bin\opt.exe" "%LBC_DIR%\bin\toolchain\win64\bin"
)
