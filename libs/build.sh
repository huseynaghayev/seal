#!/usr/bin/env bash

set -xe

IN="$1"
OUT="$2"
shift 2

CC="gcc"

$CC -I../src -fPIC -shared -o "$OUT.so" "$IN" "$@"
