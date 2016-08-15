## Options

# XPM
_CFLAGS+=-DXPM
_LFLAGS+=-lXpm
OFILES+=${BDIR}/image_xpm.o

# JPEG
_CFLAGS+=-DJPEG
_LFLAGS+=-ljpeg
OFILES+=${BDIR}/image_jpeg.o

# m4
_CFLAGS+=-DUSEM4
#_CFLAGS+=-DM4CMD=/usr/local/bin/my_special_m4
OFILES+=${BDIR}/parse_m4.o

# EWMH
_CFLAGS+=-DEWMH
OFILES+=${BDIR}/ewmh.o ${BDIR}/ewmh_atoms.o
