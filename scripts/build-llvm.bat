@echo off

set VERSION=llvm-project-22.1.0-rc3
set SRC_DIR=%cd%\%VERSION%.src\llvm
set BUILD_DIR=%cd%\%VERSION%.build
set DIST_DIR=%cd%\%VERSION%.dist

if not exist "%BUILD_DIR%" mkdir %BUILD_DIR%

cmake -G "Ninja" -S "%SRC_DIR%" -B "%BUILD_DIR%" ^
    -DCMAKE_INSTALL_PREFIX="%DIST_DIR%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DLLVM_ENABLE_ASSERTIONS=ON ^
    -DLLVM_TARGETS_TO_BUILD=host ^
    -DLLVM_OPTIMIZED_TABLEGEN=ON ^
    -DLLVM_INSTALL_UTILS=ON ^
    -DLLVM_INCLUDE_BENCHMARKS=OFF ^
    -DLLVM_INCLUDE_TESTS=OFF ^
    && cmake --build "%BUILD_DIR%" --target install ^
    && setx LLVM_DIR "%DIST_DIR%\lib\cmake\llvm"
