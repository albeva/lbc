VERSION="llvm-project-22.1.0-rc3"
SRC_DIR="$VERSION.src/llvm"
BUILD_DIR="$VERSION.build"
DIST_DIR="$VERSION.dst"

COMPILER_FLAGS=()
if [[ "$(uname)" == "Darwin" ]]; then
    COMPILER_FLAGS=(
        -DCMAKE_C_COMPILER="$(brew --prefix llvm)/bin/clang"
        -DCMAKE_CXX_COMPILER="$(brew --prefix llvm)/bin/clang++"
        -DDEFAULT_SYSROOT="$(xcrun --show-sdk-path)"
    )
fi

(
    set -e
    mkdir -p "$DIST_DIR"

    if [[ "${CLEAN_BUILD:-0}" == "1" ]]; then
        rm -rf "$BUILD_DIR"
    fi

    cmake -G "Ninja" -S "$SRC_DIR" -B "$BUILD_DIR"   \
        "${COMPILER_FLAGS[@]}"                       \
        -DCMAKE_INSTALL_PREFIX="$DIST_DIR"           \
        -DCMAKE_CXX_STANDARD=20                      \
        -DCMAKE_BUILD_TYPE=Release                   \
        -DLLVM_ENABLE_ASSERTIONS=ON                  \
        -DLLVM_TARGETS_TO_BUILD="host"               \
        -DLLVM_OPTIMIZED_TABLEGEN=ON                 \
        -DLLVM_INSTALL_UTILS=ON                      \
        -DLLVM_INCLUDE_TESTS=OFF                     \
        -DLLVM_INCLUDE_BENCHMARKS=OFF

    cmake --build "$BUILD_DIR" --target install
)
