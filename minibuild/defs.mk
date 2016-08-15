# Defs
CC?=cc
YACC?=yacc
YFLAGS?=-d -b gram
YACC_CMD?=${YACC} ${YFLAGS}
FLEX?=flex

# Alt: -xc99 for Sun Studio
C99FLAG?=-std=c99

# Basic flags for build and pulling in X
_CFLAGS=${C99FLAG} -O -I${rtdir} -I${rtdir}/ext -I/usr/local/include
_LFLAGS=-L/usr/local/include -lSM -lICE -lX11 -lXext -lXmu -lXt

# Currently non-optional but still needed
_CFLAGS+=-DUSE_SYS_REGEX

# glibc headers desire these when in C99 mode
_CFLAGS+=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_GNU_SOURCE


# Default target
all: ${BDIR} ctwm

# Build dir
${BDIR}:
	mkdir -p ${BDIR}

clean:
	rm -rf ${BDIR} ctwm

allclean: clean
	rm -f Makefile
