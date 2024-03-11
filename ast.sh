#!/bin/bash
# rm -rf obj
# mkdir obj

export CC=gcc
export CFLAGS="-g -fsanitize=address"
$CC $CFLAGS -c source/ast.c -o obj/ast.o -I include
$CC $CFLAGS -c source/cm3l/Lib/Vector.c -o obj/Vector.o -I include
$CC $CFLAGS -c source/cm3l/Lib/Hash.c -o obj/Hash.o -I include
$CC $CFLAGS -c source/cm3l/Lib/HashMap.c -o obj/HashMap.o -I include
$CC $CFLAGS -c source/cm3l/Lib/HashMapEv.c -o obj/HashMapEv.o -I include
$CC $CFLAGS -c source/cm3l/Lib/SLList.c -o obj/SLList.o -I include
$CC $CFLAGS -c source/cm3l/Lib/DLList.c -o obj/DLList.o -I include
$CC $CFLAGS -c source/cm3l/Lexer.c -o obj/Lexer.o -I include
$CC $CFLAGS -c source/cm3l/Parser.c -o obj/Parser.o -I include
$CC $CFLAGS -c source/cm3l/Loadtime/NameResolver.c -o obj/NameResolver.o -I include

$CC obj/ast.o obj/Vector.o obj/Lexer.o obj/Parser.o obj/Hash.o obj/HashMap.o obj/HashMapEv.o obj/SLList.o obj/DLList.o obj/NameResolver.o -o ast -lasan

