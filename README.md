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
        -D CMAKE_C_COMPILER=/usr/bin/clang-17 \
        -D CMAKE_CXX_COMPILER=/usr/bin/clang++-17 \
        -D Clang_DIR=/usr/lib/cmake/clang-17 \
        -D LLVM_DIR=/usr/include/llvm-17
    ```

    or (using
    [`CMakePresets.json`](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html))

    ```shell
    cmake --preset configure-ninja-multi-config
    ```

1. Build the plugin:

    ```shell
    cmake --build build --config=Release
    ```

    or (using `CMakePresets.json`)

    ```shell
    cmake --build --preset build-ninja-multi-config-release
    ```

## Usage

Run the wrapper script like so:

```shell
./build/bin/Release/openbsd_list_macro_printer test/slist.c
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
