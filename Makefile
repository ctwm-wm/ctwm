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
docs_clean doc_clean:
	rm -f ${DOC_FILES}

README.html: README.md
	multimarkdown -ao README.html README.md
CHANGES.html: CHANGES.md
	multimarkdown -ao CHANGES.html CHANGES.md
STYLE.html: STYLE.md
	multimarkdown -ao STYLE.html STYLE.md


# asciidoc files
adocs:
	(cd doc && make all)
adoc_clean:
	(cd doc && make clean)


#
# Pre-build some files for tarballs
#
GEN=gen
${GEN}:
	mkdir -p ${GEN}

# All the generated source files
_RELEASE_FILES=gram.tab.c gram.tab.h lex.c
RELEASE_FILES=${_RELEASE_FILES:%=${GEN}/%}

# Build those, the .html versions of the above docs, and the HTML/man
# versions of the manual
release_files: ${RELEASE_FILES} ${DOC_FILES} adocs
release_clean: doc_clean adoc_clean
	rm -rf ${GEN}

# The config grammar
YACC?=/usr/bin/yacc
YFLAGS=-d -b ${GEN}/gram
${GEN}/gram.tab.c: ${GEN}/gram.tab.h
${GEN}/gram.tab.h: ${GEN} gram.y
	${YACC} ${YFLAGS} gram.y

LEX?=/usr/bin/flex
LFLAGS=-o ${GEN}/lex.c
${GEN}/lex.c: ${GEN} lex.l
	${LEX} ${LFLAGS} lex.l
