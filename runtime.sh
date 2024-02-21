#!/bin/bash
rm -rf obj
mkdir obj

export CC=gcc
export CPP=g++
export CFLAGS="-g"
$CPP $CFLAGS -c source/runtime.cpp -o obj/runtime.o -I include
$CC $CFLAGS -c source/cm3l/Lib/Vector.c -o obj/Vector.o -I include
$CC $CFLAGS -c source/cm3l/Lexer.c -o obj/Lexer.o -I include
$CC $CFLAGS -c source/cm3l/Composer.c -o obj/Composer.o -I include

$CPP obj/runtime.o obj/Vector.o obj/Lexer.o obj/Composer.o -o runtime

