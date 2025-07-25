#!/bin/bash

CC="gcc"
LD="gcc"

CFLAGS="-O2 -Wall -Wextra -g"
LFLAGS="-lpthread"

SRC=src
OBJ=obj
BUILD=build

SRCS=($(find "$SRC" -name "*.c" -o -path "$SRC" -name "*.c"))
OBJSK=()

for src in "${SRCS[@]}"; do
    obj="${OBJ}/$(basename "$src" .c).o"
    OBJSK+=("$obj")
done

OUTPUT=$BUILD/server

function build {
    mkdir -p $OBJ
    mkdir -p $BUILD

    for i in "${!SRCS[@]}"; do
        src="${SRCS[$i]}"
        obj="${OBJSK[$i]}"
        # echo $src $obj
        comp $src $obj
    done

    $LD $LFLAGS "${OBJSK[@]}" -o $OUTPUT
}

function run {
    if [ ! -e "./$OUTPUT" ]; then
        build
    fi
    ./$OUTPUT
}

function comp {
    $CC $CFLAGS -c $1 -o $2
}

function clean {
    clear

    rm -rf $OBJ
    rm -rf $BUILD
}

for arg in "$@"; do
    if [ "$arg" = "build" ]; then
        build
    elif [ "$arg" = "run" ]; then
        run
    elif [ "$arg" = "clean" ]; then
        clean
    fi
done
