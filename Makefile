# This just shortcuts stuff through to cmake
all build ctwm man install clean: build/Makefile
	( cd build && ${MAKE} ${@} )

build/Makefile cmake: CMakeLists.txt
	( cd build && \
		cmake -DCMAKE_C_FLAGS:STRING="${CFLAGS}" ${CMAKE_EXTRAS} .. )

allclean distclean:
	rm -rf build/*


# Reindent files
indent:
	astyle -n --options=tools/ctwm.astyle *.h *.c


# Build documentation files
DOC_FILES=README.html CHANGES.html STYLE.html
docs: ${DOC_FILES}
doc_clean:
	rm -f ${DOC_FILES}

README.html: README
	multimarkdown -ao README.html README
CHANGES.html: CHANGES
	multimarkdown -ao CHANGES.html CHANGES
STYLE.html: STYLE
	multimarkdown -ao STYLE.html STYLE


# Prebuild these files for releases
YACC?=/usr/bin/yacc
YFLAGS=-d -b gram
RELEASE_FILES=gram.tab.c gram.tab.h lex.c deftwmrc.c

release_files: ${RELEASE_FILES} ${DOC_FILES}
release_clean: doc_clean
	rm -f ${RELEASE_FILES}

gram.tab.c: gram.tab.h
gram.tab.h: gram.y
	${YACC} ${YFLAGS} gram.y

# Standard make transformations give us lex.c

deftwmrc.c: system.ctwmrc
	tools/mk_deftwmrc.sh system.ctwmrc > deftwmrc.c
