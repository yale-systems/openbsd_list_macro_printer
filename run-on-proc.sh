/usr/local/llvm17/bin/clang \
    -fplugin=./build/lib/Debug/libopenbsd_list_macro_printer.so \
    -fsyntax-only \
    -Wno-implicit-int \
    -Wno-implicit-function-declaration \
    -Wno-macro-redefined \
    -I /usr/src/sys \
    -I /usr/obj/usr/src/arm64.aarch64/tmp/usr/include \
    /usr/src/sys/kern/kern_proc.c


    #-nostdinc \
    #-nostdlib \


