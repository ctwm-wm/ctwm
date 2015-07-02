BASE=ctwm.1

HTML=${BASE}.html

html: ${HTML}
man: ${BASE}

all: html man

clean:
	rm -f ${BASE} ${HTML}

#${BASE}.txt: ../ctwm.man convert.pl
#	./convert.pl < ../ctwm.man > ${BASE}.txt

# HTML output
${HTML}: ${BASE}.txt
	@${MAKE} html-asciidoc

html-asciidoc: ${BASE}.txt
	asciidoc -atoc -anumbered -o ${BASE}.html ${BASE}.txt


# Manpage outputs
${BASE}: ${BASE}.txt
	@${MAKE} man-asciidoc-a2x

man-asciidoc-a2x: ${BASE}.txt
	a2x --doctype manpage --format manpage ${BASE}.txt

man-asciidoc-adoc: ${BASE}.txt
	asciidoc -b docbook -d manpage -o ${BASE}.xml ${BASE}.txt
	xmlto man ${BASE}.xml


# asciidoctor variants
# Note that asciidoctor has a slightly changed dialect, so there will be
# some glitches in the output.  Also, the manpage generation requires
# 1.5.3+ (not yet released as of this writing).  Since the currently
# worldview seems to be that asciidoctor is the primary movement in the
# adoc world, we'll probablt transition at some point.
html-asciidoctor: ${BASE}.txt
	asciidoctor -atoc -anumbered -o ${BASE}.html ${BASE}.txt

man-asciidoctor: ${BASE}.txt
	asciidoctor -b manpage -o ${BASE} ${BASE}.txt
