AM_CFLAGS=-Wall -O2 -std=c99
AM_CFLAGS+=$(GTK_CFLAGS) ${GLIB_CFLAGS} ${X11_CFLAGS} ${SQLITE3_CFLAGS}

LDADD = $(GTK_LIBS) ${GLIB_LIBS} ${X11_LIBS} ${SQLITE3_LIBS}

bin_PROGRAMS = clip

clip_SOURCES = 	daemon.c gtk_provider.c \
			   	clipboard.c history.c \
				gui.c keybinder.c \
				utils.c main.c

