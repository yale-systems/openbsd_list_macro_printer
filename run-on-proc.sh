/usr/local/llvm17/bin/clang \
    -fplugin=./build/lib/Debug/libopenbsd_list_macro_printer.so \
    -fsyntax-only \
    -nostdinc \
    -nostdlib \
    /usr/src/sys/kern/kern_proc.c



