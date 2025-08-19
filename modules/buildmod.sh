#!/bin/bash

OS="linux"
REQ_FILES="../src/moddef.c"
INC_FLAGS="-I ./ -I ../src"
MOD=$1
shift
FLAGS=""
while [[ $# -ne 0 ]]; do
    if [[ $1 == "--windows" ]]; then
        OS="windows"
    else
        FLAGS="$FLAGS $1"
    fi
    shift
done

echo "compiling for $OS..."
if [[ $OS == "linux" ]]; then
    gcc -fPIC -shared -o "$MOD.so" "$MOD.c" $REQ_FILES $INC_FLAGS $FLAGS
else
    x86_64-w64-mingw32-gcc -shared -o "$MOD.dll" "$MOD.c" $REQ_FILES $INC_FLAGS $FLAGS
fi
