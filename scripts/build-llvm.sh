VERSION="llvm-project-18.1.7"
SRC_DIR="$VERSION.src/llvm"
BUILD_DIR="$VERSION.build"

cmake -G "Ninja" -S "$SRC_DIR" -B "$BUILD_DIR"  \
  -DCMAKE_INSTALL_PREFIX=/usr/local             \
  -DCMAKE_CXX_STANDARD=20                       \
  -DCMAKE_BUILD_TYPE=Release                    \
  -DDEFAULT_SYSROOT="$(xcrun --show-sdk-path)"  \
  -DLLVM_ENABLE_ASSERTIONS=ON                   \
  -DLLVM_TARGETS_TO_BUILD="host"                \
  -DLLVM_OPTIMIZED_TABLEGEN=ON                  \
  -DLLVM_INSTALL_UTILS=ON                       \
  -DLLVM_INCLUDE_TESTS=OFF

if [ $? -eq 0 ]; then
    cmake --build "$BUILD_DIR"
fi

if [ $? -eq 0 ]; then
    sudo cmake --install "$BUILD_DIR"
fi
