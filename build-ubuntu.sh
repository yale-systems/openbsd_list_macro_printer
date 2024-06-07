cmake -S . -B build -G "Ninja Multi-Config" \
    -D CMAKE_C_COMPILER=/usr/bin/clang-17 \
    -D CMAKE_CXX_COMPILER=/usr/bin/clang++-17 \
    -D LLVM_DIR=/usr/lib/llvm-17/lib/cmake/llvm \
    -D Clang_DIR=/usr/lib/llvm-17/lib/cmake/clang \
    -D CMAKE_EXPORT_COMPILE_COMMANDS=TRUE

cmake --build build/ --config Debug