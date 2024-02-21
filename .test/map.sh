#!/bin/bash
gcc -g -c source/dht.c -o dht.o -I include
# gcc -g -c source/dhtbench.c -o dhtbench.o -I include
gcc -g -c source/map.c -o map.o -I include
gcc -g -c source/mapbench.c -o mapbench.o -I include
gcc -g -c source/cm3l/Lib/Hash.c -o "0.o" -I include
gcc -g -c source/cm3l/Lib/DHTMap.c -o "1.o" -I include
gcc -g -c source/cm3l/Lib/SLList.c -o "2.o" -I include
gcc -g -c source/cm3l/Lib/HashMap.c -o "4.o" -I include
gcc -g -c source/cm3l/Lib/Vector.c -o "5.o" -I include

gcc dht.o "0.o" "1.o" "2.o" -o dht
gcc map.o "0.o" "4.o" "2.o" "5.o" -o map
# gcc dhtbench.o "0.o" "1.o" "2.o" -o dhtbench
gcc mapbench.o "0.o" "4.o" "2.o" "5.o" -o mapbench
