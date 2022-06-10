VERSION="llvm-project-llvmorg-14.0.4"
BUILD_DIR="$VERSION.build"

mkdir -p "$BUILD_DIR"

if [[ $(uname -m) == 'arm64' ]]; then
  TARGETS="AArch64;X86"
else
  TARGETS="X86"
fi

cmake -G "Ninja" -S "$VERSION/llvm" -B "$BUILD_DIR" \
  -DCMAKE_INSTALL_PREFIX=/usr/local  \
  -DCMAKE_CXX_STANDARD=20            \
  -DCMAKE_BUILD_TYPE=Release         \
  -DLLVM_ENABLE_ASSERTIONS=ON        \
  -DLLVM_TARGETS_TO_BUILD="$TARGETS" \
  -DLLVM_OPTIMIZED_TABLEGEN=ON       \
  -DLLVM_INSTALL_UTILS=ON            \
  -DLLVM_INCLUDE_BENCHMARKS=OFF      \
  -DLLVM_INCLUDE_TESTS=OFF

if [ $? -eq 0 ]; then
    sudo cmake --build "$BUILD_DIR" --target install
fi
