cmake -S . -B build -G "Ninja Multi-Config" \
    -D CMAKE_C_COMPILER=/opt/homebrew/opt/llvm@17/bin/clang \
    -D CMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm@17/bin/clang++ \
    -D LLVM_DIR=/opt/homebrew/Cellar/llvm@17/17.0.6/lib/cmake//llvm \
    -D Clang_DIR=/opt/homebrew/Cellar/llvm@17/17.0.6/lib/cmake/clang \
    -D CMAKE_EXPORT_COMPILE_COMMANDS=TRUE

cmake --build build/ --config Debug
