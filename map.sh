#!/bin/bash
#rm -rf obj
#mkdir obj

export CC=gcc
export CPP=g++
export CFLAGS="-O3"
export LDFLAGS=""
$CC $CFLAGS -c source/cm3l/Lib/Vector.c -o obj/Vector.o -I include
$CC $CFLAGS -c source/cm3l/Lib/SLList.c -o obj/SLList.o -I include
$CC $CFLAGS -c source/cm3l/Lib/HashMap.c -o obj/HashMap.o -I include
$CC $CFLAGS -c source/cm3l/Lib/HashMapEv.c -o obj/HashMapEv.o -I include
$CC $CFLAGS -c source/cm3l/Lib/Hash.c -o obj/Hash.o -I include
# $CC $CFLAGS -c source/cm3l/Lexer.c -o obj/Lexer.o -I include
# $CC $CFLAGS -c source/cm3l/Composer.c -o obj/Composer.o -I include

#$CC $CFLAGS -c source/map.c -o obj/map.o -I include
#$CC $CFLAGS -c source/mapev.c -o obj/mapev.o -I include
$CC $CFLAGS -c source/mapbench.c -o obj/mapbench.o -I include
$CC $CFLAGS -c source/mapevbench.c -o obj/mapevbench.o -I include
$CPP $CFLAGS -c source/stlbench.cpp -o obj/stlbench.o -I include

#$CC $LDFLAGS obj/map.o obj/Vector.o obj/SLList.o obj/HashMap.o obj/Hash.o -o map
#$CC $LDFLAGS obj/mapev.o obj/Vector.o obj/SLList.o obj/HashMap.o obj/HashMapEv.o obj/Hash.o -o mapev
$CC $LDFLAGS obj/mapbench.o obj/Vector.o obj/SLList.o obj/HashMap.o obj/Hash.o -o mapbench
$CC $LDFLAGS obj/mapevbench.o obj/Vector.o obj/SLList.o obj/HashMap.o obj/HashMapEv.o obj/Hash.o -o mapevbench
$CPP $LDFLAGS obj/stlbench.o obj/Hash.o -o stlbench

