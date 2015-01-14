BASE=ctwm.1

HTML=${BASE}.html

html: ${HTML}
man: ${BASE}

all: html man

clean:
	rm -f ${BASE} ${HTML}

#${BASE}.txt: ../ctwm.man convert.pl
#	./convert.pl < ../ctwm.man > ${BASE}.txt

${HTML}: ${BASE}.txt
	asciidoc -atoc -anumbered -o ${BASE}.html ${BASE}.txt

${BASE}: ${BASE}.txt
	a2x --doctype manpage --format manpage ${BASE}.txt
