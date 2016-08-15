## Options

# XPM
_CFLAGS+=-DXPM
_LFLAGS+=-lXpm
OFILES+=${BDIR}/image_xpm.o
ALLSRC+=${RTDIR}/image_xpm.c

# JPEG
_CFLAGS+=-DJPEG
_LFLAGS+=-ljpeg
OFILES+=${BDIR}/image_jpeg.o
ALLSRC+=${RTDIR}/image_jpeg.c

# m4
_CFLAGS+=-DUSEM4
#_CFLAGS+=-DM4CMD=/usr/local/bin/my_special_m4
OFILES+=${BDIR}/parse_m4.o
ALLSRC+=${RTDIR}/parse_m4.c

# EWMH
_CFLAGS+=-DEWMH
OFILES+=${BDIR}/ewmh.o ${BDIR}/ewmh_atoms.o
GENSRC+=${BDIR}/ewmh_atoms.c



### Rules for generating various files

## Autogen'd files

# Stand-in ctwm_config.h
GENSRC+=${BDIR}/ctwm_config.h
${BDIR}/ctwm_config.h:
	( \
		echo '#define SYSTEM_INIT_FILE "/not/yet/set/system.ctwmrc"' ; \
		echo '#define PIXMAP_DIRECTORY "/not/yet/set/pixmaps"' ; \
		echo '#define M4CMD "m4"' ; \
	) > ${BDIR}/ctwm_config.h


# Atom lists are script-generated
GENSRC+=${BDIR}/ctwm_atoms.c
${BDIR}/ctwm_atoms.o: ${BDIR}/ctwm_atoms.c
${BDIR}/ctwm_atoms.c: ${RTDIR}/ctwm_atoms.in
	(cd ${BDIR} && \
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ctwm_atoms.in ctwm_atoms CTWM)

# Only when EWMH
GENSRC+=${BDIR}/ewmh_atoms.c
${BDIR}/ewmh_atoms.o: ${BDIR}/ewmh_atoms.c
${BDIR}/ewmh_atoms.c: ${RTDIR}/ewmh_atoms.in
	(cd ${BDIR} && \
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ewmh_atoms.in ewmh_atoms EWMH)


# Just make null version file
GENSRC+=${BDIR}/version.c
${BDIR}/version.o: ${BDIR}/version.c
${BDIR}/version.c: ${RTDIR}/version.c.in ${RTDIR}/VERSION
	( \
		vstr=`sed -E \
			-e 's/([0-9]+)\.([0-9]+)\.([0-9]+)(.*)/\1 \2 \3 \4/' \
			${RTDIR}/VERSION` ; \
		set -- junk $$vstr ; shift ; \
		maj=$$1;  \
		min=$$2;  \
		pat=$$3;  \
		addl=$$4;  \
		sed \
			-e "s/%%[A-Z]*%%/NULL/" \
			-e "s/@ctwm_version_major@/$$maj/" \
			-e "s/@ctwm_version_minor@/$$min/" \
			-e "s/@ctwm_version_patch@/$$pat/" \
			-e "s/@ctwm_version_addl@/$$addl/" \
			${RTDIR}/version.c.in > ${BDIR}/version.c \
	)


# Table of event names
GENSRC+=${BDIR}/event_names_table.h
${BDIR}/event_names_table.h: ${RTDIR}/event_names.list
	${RTDIR}/tools/mk_event_names.sh ${RTDIR}/event_names.list \
		> ${BDIR}/event_names_table.h


# Default config
GENSRC+=${BDIR}/deftwmrc.c
${BDIR}/deftwmrc.c: ${RTDIR}/system.ctwmrc
	${RTDIR}/tools/mk_deftwmrc.sh ${RTDIR}/system.ctwmrc > ${BDIR}/deftwmrc.c


# lex/yacc inputs
GENSRC+=${BDIR}/lex.c
${BDIR}/lex.o: ${BDIR}/lex.c
${BDIR}/lex.c: ${RTDIR}/lex.l
	${FLEX} -o ${BDIR}/lex.c ${RTDIR}/lex.l

GENSRC+=${BDIR}/gram.tab.c
${BDIR}/gram.tab.o ${BDIR}/gram.tab.h: ${BDIR}/gram.tab.c
${BDIR}/gram.tab.c: ${RTDIR}/gram.y
	(cd ${BDIR} && ${YACC_CMD} ${RTDIR}/../gram.y)



## Main build
ALLSRC+=${GENSRC}
gen: ${GENSRC}

ctwm: ${BDIR} ${GENSRC} ${OFILES}
	cc -o ctwm ${OFILES} ${_LFLAGS}

.c.o:
	${CC} ${_CFLAGS} ${CFLAGS} -c -o ${@} ${<}
