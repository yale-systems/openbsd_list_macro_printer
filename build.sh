cmake -S . -B build -G "Ninja Multi-Config" \
    -D CMAKE_C_COMPILER=clang \
    -D CMAKE_CXX_COMPILER=clang++ \
    -D LLVM_DIR="/usr/include/llvm-17" \
    -D Clang_DIR="/usr/lib/cmake/clang-17" \
    -D CMAKE_EXPORT_COMPILE_COMMANDS=TRUE

cmake --build build/ --config Debug
