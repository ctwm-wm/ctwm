## Rules for generating various files

## Autogen'd files

# Atom lists are script-generated
${BDIR}/ctwm_atoms.o: ${BDIR}/ctwm_atoms.c
${BDIR}/ctwm_atoms.c: ${RTDIR}/ctwm_atoms.in
	(cd ${BDIR} && 
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ctwm_atoms.in ctwm_atoms)

# Only when EWMH
${BDIR}/ewmh_atoms.o: ${BDIR}/ewmh_atoms.c
${BDIR}/ewmh_atoms.c: ${RTDIR}/ewmh_atoms.in
	(cd ${BDIR} && \
		${RTDIR}/../tools/mk_atoms.sh ${RTDIR}/../ewmh_atoms.in ewmh_atoms)


# Just make null version file
# XXX Doesn't sub version yet
${BDIR}/version.o: ${BDIR}/version.c
${BDIR}/version.c: ${RTDIR}/version.c.in
	sed \
		-e "s/%%[A-Z]*%%/NULL/" \
		${RTDIR}/version.c > ${BDIR}/version.c


# Table of event names
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
	cc -o ${CTWM} ${OFILES} ${LFLAGS}
