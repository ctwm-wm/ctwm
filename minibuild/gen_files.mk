## Rules for generating various files

## Autogen'd files

# Stand-in ctwm_config.h
gen: ${BDIR}/ctwm_config.h
${BDIR}/ctwm_config.h:
	( \
		echo '#define SYSTEM_INIT_FILE "/not/yet/set/system.ctwmrc"' ; \
		echo '#define PIXMAP_DIRECTORY "/not/yet/set/pixmaps"' ; \
		echo '#define M4CMD "m4"' ; \
	) > ${BDIR}/ctwm_config.h


# Atom lists are script-generated
gen: ${BDIR}/ctwm_atoms.c
${BDIR}/ctwm_atoms.o: ${BDIR}/ctwm_atoms.c
${BDIR}/ctwm_atoms.c: ${RTDIR}/ctwm_atoms.in
	(cd ${BDIR} && \
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ctwm_atoms.in ctwm_atoms CTWM)

# Only when EWMH
${BDIR}/ewmh_atoms.o: ${BDIR}/ewmh_atoms.c
${BDIR}/ewmh_atoms.c: ${RTDIR}/ewmh_atoms.in
	(cd ${BDIR} && \
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ewmh_atoms.in ewmh_atoms EWMH)


# Just make null version file
# XXX Doesn't sub version yet
${BDIR}/version.o: ${BDIR}/version.c
${BDIR}/version.c: ${RTDIR}/version.c.in
	sed \
		-e "s/%%[A-Z]*%%/NULL/" \
		${RTDIR}/version.c.in > ${BDIR}/version.c


# Table of event names
gen: ${BDIR}/event_names_table.h
${BDIR}/event_names_table.h: ${RTDIR}/event_names.list
	${RTDIR}/tools/mk_event_names.sh ${RTDIR}/event_names.list \
		> ${BDIR}/event_names_table.h


# Default config
${BDIR}/deftwmrc.c: ${RTDIR}/system.ctwmrc
	${RTDIR}/tools/mk_deftwmrc.sh ${RTDIR}/system.ctwmrc > ${BDIR}/deftwmrc.c


# lex/yacc inputs
${BDIR}/lex.o: ${BDIR}/lex.c
${BDIR}/lex.c: ${RTDIR}/lex.l
	${FLEX} -o ${BDIR}/lex.c ${RTDIR}/lex.l

${BDIR}/gram.tab.o ${BDIR}/gram.tab.h: ${BDIR}/gram.tab.c
${BDIR}/gram.tab.c: ${RTDIR}/gram.y
	(cd ${BDIR} && ${YACC_CMD} ${RTDIR}/../gram.y)



## Main build
ctwm: ${BDIR} gen ${OFILES}
	cc -o ctwm ${OFILES} ${_LFLAGS}

.c.o:
	${CC} ${_CFLAGS} ${CFLAGS} -c -o ${@} ${<}
