cmake -S . -B build -G "Ninja Multi-Config" \
    -D CMAKE_C_COMPILER=/usr/local/llvm17/bin/clang \
    -D CMAKE_CXX_COMPILER=/usr/local/llvm17/bin/clang++ \
    -D Clang_DIR=/usr/local/llvm17/lib/cmake/clang \
    -D LLVM_DIR=/usr/local/llvm17/lib/cmake/llvm

cmake --build build/ --config Release