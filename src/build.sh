#!/usr/bin/env sh
set -e
SRC_PATH=.
CC=g++
LD=g++
CFLAGS="-DDEBUG -I${SRC_PATH}/lib -std=c++11 -g -O0 -Wall"
LDFLAGS="-lc"

rm -f fonda

set -x
# Just build everything -- this project isn't big

# Library files
${CC} ${CFLAGS} -c -o fonda_lib/readelf.o fonda_lib/readelf.cpp
${CC} ${CFLAGS} -c -o fonda_lib/readtos.o fonda_lib/readtos.cpp

# Application file
${CC} ${CFLAGS} -c -o main.o main.cpp

${LD} ${LDFLAGS} fonda_lib/readelf.o fonda_lib/readtos.o main.o -o fonda


