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
${HTML}: html-asciidoc

html-asciidoc: ${BASE}.txt
	asciidoc -atoc -anumbered -o ${BASE}.html ${BASE}.txt


# Manpage outputs
${BASE}: man-asciidoc-a2x

man-asciidoc-a2x: ${BASE}.txt
	a2x --doctype manpage --format manpage ${BASE}.txt

man-asciidoc-adoc: ${BASE}.txt
	asciidoc -b docbook -d manpage -o ${BASE}.xml ${BASE}.txt
	xmlto man ${BASE}.xml
