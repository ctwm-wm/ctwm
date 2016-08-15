# Defs
CC?=cc
YACC?=yacc
YACC_FLAGS?=-d -b gram
YACC_CMD?=${YACC} ${YACC_FLAGS}
FLEX?=flex

# Alt: -xc99 for Sun Studio
C99FLAG?=-std=c99

# Basic flags for build and pulling in X
_CFLAGS=${C99FLAG} -O -I${RTDIR} -I${RTDIR}/ext -I${BDIR} -I/usr/local/include
_LFLAGS=-L/usr/local/lib -lSM -lICE -lX11 -lXext -lXmu -lXt

# glibc headers desire these when in C99 mode
_CFLAGS+=-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -D_GNU_SOURCE

# And BSD headers, when they see the above, need this
_CFLAGS+=-D__BSD_VISIBLE

# Solaris b0rkedness needs this for gethostbyname
#_LFLAGS+=-lnsl


# Default target
all: ${BDIR} ctwm

# Build dir
${BDIR}:
	mkdir -p ${BDIR}

clean:
	rm -rf ${BDIR} ctwm

allclean: clean
	rm -f Makefile*

gen: ${BDIR}
GENSRC=
