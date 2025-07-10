#!/bin/bash

CC="gcc"
OUT="seal"
DIR="src"
OBJ="obj"
FLAGS="-std=c99 -O2"

usage() {
  echo "Usage: $0 [--windows]"
  echo "  --windows    Compile for Windows (using MinGW)"
  echo "  (default)    Compile for Linux"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --windows)
      CC="x86_64-w64-mingw32-gcc"
      shift
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

mkdir -p $OBJ

for c in $DIR/*.c; do
	$CC -c $c -I$DIR -o "$OBJ/$(basename $c ".c").o" $FLAGS
done

$CC $OBJ/* -o $OUT $FLAGS

rm -rf $OBJ
