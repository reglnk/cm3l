#!/bin/bash
gcc -g -c source/cm3l/Lib/Hash.c -o "obj/0.o" -I include
gcc -g -c source/cm3l/Lib/DHTMap.c -o "obj/1.o" -I include
gcc -g -c source/cm3l/Lib/SLList.c -o "obj/2.o" -I include
gcc -g -c source/cm3l/Lib/IdMap.c -o "obj/3.o" -I include
gcc -g -c source/cm3l/Lib/Vector.c -o "obj/4.o" -I include

gcc -g -c source/cm3l/Composer.c -o "obj/cm.o" -I include
gcc -g -c source/cm3l/Lexer.c -o "obj/lex.o" -I include
gcc -g -c source/main.c -o "obj/main.o" -I include

gcc obj/main.o obj/0.o obj/1.o obj/2.o obj/3.o obj/4.o obj/cm.o obj/lex.o -o cm3l
