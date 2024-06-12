/opt/homebrew/opt/llvm@17/bin/clang \
    -fplugin=./build/lib/Debug/libopenbsd_list_macro_printer.dylib \
    -fsyntax-only \
    -nostdinc \
    -nostdlib \
    -I /Volumes/Repos/Yale/freebsd-src/include \
    -I /Volumes/Repos/Yale/freebsd-src/sys \
    -I /Volumes/Repos/Yale/freebsd-src/machine \
    /Volumes/Repos/Yale/freebsd-src/sys/sys/proc.h

