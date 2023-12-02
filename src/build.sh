#!/usr/bin/env sh
SRC_PATH=.
CC=g++
LD=g++
CFLAGS="-DDEBUG -I${SRC_PATH}/lib -std=c++11 -g -O0 -Wall"
LDFLAGS="-lc"

# Just build everything -- this project isn't big
${CC} ${CFLAGS} -c -o main.o main.cpp
${CC} ${CFLAGS} -c -o fonda_lib/readelf.o fonda_lib/readelf.cpp

${LD} ${LDFLAGS} fonda_lib/readelf.o main.o -o fonda


