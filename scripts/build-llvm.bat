@echo off

set VERSION=llvm-project-14.0.5

set SRC=%cd%\%VERSION%.src\llvm
set BUILD=%cd%\%VERSION%.build
set INSTALL=%cd%\%VERSION%.dist

if not exist "%BUILD%" mkdir %BUILD%

cmake -G "Ninja" -S "%SRC%" -B "%BUILD%" ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DLLVM_ENABLE_ASSERTIONS=ON ^
    -DLLVM_TARGETS_TO_BUILD=X86 ^
    -DLLVM_OPTIMIZED_TABLEGEN=ON ^
    -DLLVM_INSTALL_UTILS=ON ^
    -DLLVM_INCLUDE_BENCHMARKS=OFF ^
    -DLLVM_INCLUDE_TESTS=OFF
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build "%BUILD%" --target install ^
    && setx LLVM_DIR "%INSTALL%\lib\cmake\llvm"
