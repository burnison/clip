CC=gcc
CFLAGS=-g -Wall -O2 -std=c99
CFLAGS+=`pkg-config --cflags --libs gtk+-2.0`
CFLAGS+=`pkg-config --cflags --libs glib-2.0`
#CFLAGS_LIBS=$(shell pkg-config --cflags --libs sqlite3)

PROJECT=clipboard

SRC=src
OUT=${SRC}/${PROJECT}

OBJECTS=
OBJECTS+=${SRC}/clipboard.c
OBJECTS+=${SRC}/main.c

all:
		${CC} ${CFLAGS} ${CFLAGS_LIBS} ${OBJECTS} -o ${OUT}
		chmod 755 ${OUT}

clean:
		rm -f ${OUT}
