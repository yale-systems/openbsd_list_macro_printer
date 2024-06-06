# C macro and function call finder

A Clang plugin for creating printers for OpenBSD list macro declarations.

## Quickstart

### Download prerequisites

The following instructions assume an Ubuntu 22.04 LTS operating system:

- Ninja

  ```shell
  sudo apt install ninja-build
  ```

- CMake 3.29.1 (click [here](https://apt.kitware.com/) for download and install
  instructions)

- [LLVM 17 and Clang 17](https://apt.llvm.org/):

  ```shell
  wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc
  sudo apt install llvm-17 clang-17 libclang-17-dev
  ```

### Building the Clang plugin

1. Configure the plugin:

    ```shell
    cmake -S . -B build -G "Ninja Multi-Config" \
        -D CMAKE_C_COMPILER=<path to clang-17 compiler> \
        -D CMAKE_CXX_COMPILER=<path to clang++-17 compiler> \
        -D Clang_DIR=<path to ClangConfig.cmake> \
        -D LLVM_DIR=<path to LLVMConfig.cmake>
    ```

1. Build the plugin:

    ```shell
    cmake --build build --config=Release
    ```

## Usage

Assuming you are currently at the project root, then you can run the shared
library on the following platforms as follows:

Ubuntu:

```shell
/usr/bin/clang-17 \
    -fplugin=./build/lib/Debug/libopenbsd_list_macro_printer.so \
    -fsyntax-only \
    test/slist.c
```

MacOS:

```shell
/opt/homebrew/opt/llvm@17/bin/clang \
    -fplugin=./build/lib/Debug/libopenbsd_list_macro_printer.dylib \
    -fsyntax-only \
    test/slist.c
```

The output should be:

```txt
{
    struct entry * __openbsd_list_iterator;
    SLIST_FOREACH(__openbsd_list_iterator, &head, entries) {
        printf("%d", __openbsd_list_iterator->a);
        printf("%d", __openbsd_list_iterator->b);
        printf("%s", __openbsd_list_iterator->s);
        printf("%s", __openbsd_list_iterator->t);
    }
}
```

## Development

We recommend downloading the [`clangd`](https://clangd.llvm.org/installation)
C/C++ language server to ease development.

This project also provides a `.clang-format` file for formatting. If you would
like to contribute, please ensure that you've formatted your code with
[`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) before committing
it. Assuming you are on Ubuntu, you can download `clang-format` with the
following command:

```shell
sudo apt install clang-format
```
