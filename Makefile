CC=gcc
CFLAGS=-g -Wall -O2 -std=c99
CFLAGS+=`pkg-config --cflags --libs gtk+-3.0`
CFLAGS+=`pkg-config --cflags --libs glib-2.0`
CFLAGS+=`pkg-config --cflags --libs x11`
#CFLAGS_LIBS=$(shell pkg-config --cflags --libs sqlite3)

PROJECT=clip

SRC=src
OUT=${SRC}/${PROJECT}

OBJECTS=
OBJECTS+=${SRC}/clipboard.c
OBJECTS+=${SRC}/daemon.c
OBJECTS+=${SRC}/gui.c
OBJECTS+=${SRC}/keybinder.c
OBJECTS+=${SRC}/gtk_provider.c
OBJECTS+=${SRC}/main.c

all:
		${CC} ${CFLAGS} ${CFLAGS_LIBS} ${OBJECTS} -o ${OUT}
		chmod 755 ${OUT}

clean:
		rm -f ${OUT}
