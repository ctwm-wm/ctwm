BASE=ctwm.1

HTML=${BASE}.html

html: ${HTML}
man: ${BASE}

all: html man

clean:
	rm -f ${BASE} ${HTML}

#${BASE}.adoc: ../ctwm.man convert.pl
#	./convert.pl < ../ctwm.man > ${BASE}.adoc

# HTML output
${HTML}: ${BASE}.adoc
	@${MAKE} html-asciidoc

html-asciidoc: ${BASE}.adoc
	asciidoc -atoc -anumbered -o ${BASE}.html ${BASE}.adoc


# Manpage outputs
${BASE}: ${BASE}.adoc
	@${MAKE} man-asciidoc-a2x

man-asciidoc-a2x: ${BASE}.adoc
	a2x --doctype manpage --format manpage ${BASE}.adoc

man-asciidoc-adoc: ${BASE}.adoc
	asciidoc -b docbook -d manpage -o ${BASE}.xml ${BASE}.adoc
	xmlto man ${BASE}.xml


# asciidoctor variants
# Note that asciidoctor has a slightly changed dialect, so there will be
# some glitches in the output.  Also, the manpage generation requires
# 1.5.3+ (not yet released as of this writing).  Since the currently
# worldview seems to be that asciidoctor is the primary movement in the
# adoc world, we'll probably transition at some point.
html-asciidoctor: ${BASE}.adoc
	asciidoctor -atoc -anumbered -o ${BASE}.html ${BASE}.adoc

man-asciidoctor: ${BASE}.adoc
	asciidoctor -b manpage -o ${BASE} ${BASE}.adoc
