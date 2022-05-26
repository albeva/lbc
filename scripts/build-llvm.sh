VERSION="llvm-project-llvmorg-14.0.3"

mkdir $VERSION.build
cd $VERSION.build/

cmake -G "Unix Makefiles"           \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DCMAKE_CXX_STANDARD=20           \
  -DCMAKE_BUILD_TYPE=Release        \
  -DLLVM_ENABLE_ASSERTIONS=ON       \
  -DLLVM_TARGETS_TO_BUILD=X86       \
  -DLLVM_OPTIMIZED_TABLEGEN=ON      \
  -DLLVM_INSTALL_UTILS=ON           \
  -DLLVM_INCLUDE_BENCHMARKS=OFF     \
  -DLLVM_INCLUDE_TESTS=OFF          \
  ../$VERSION/llvm

make -j12 && sudo make install
cd ..
