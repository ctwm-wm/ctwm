#
# Handle stuff related to building the manual in various ways
#


# It's written in asciidoc, so first see if we can find that.  If we can,
# we'll use it to [re]build the manual.  If not, we may have pregen'd
# versions we can use.  If neither, well, the user's in trouble...
find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)


#
# Setup some vars for the various steps in the process
#

# The original source
set(ADOC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/doc/ctwm.1.adoc)

# Where we build stuff
set(MAN_TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/mantmp)

# Where we copy the source to during rewrites, and then do the actual
# build from.
set(ADOC_TMPSRC ${MAN_TMPDIR}/ctwm.1.adoc)

# Where the end products wind up
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1)
set(MANHTML ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.html)

# How we rewrite vars in the manual.  I decided not to use
# configure_file() for this, as it opens up too many chances for
# something to accidentally get sub'd, since we assume people will write
# pretty freeform in the manual.
# \-escaped @ needed for pre-3.1 CMake compat and warning avoidance;
# x-ref cmake --help-policy CMP0053
set(MANSED_CMD sed -e "s,\@ETCDIR@,${ETCDIR},")

# Flags for what we wind up having
set(HAS_MAN 0)
set(HAS_HTML 0)



#
# The build process
#

# If we have asciidoc, use it
if(ASCIIDOC AND A2X)
	set(HAS_MAN 1)

	message(STATUS "Found asciidoc (${A2X}) for building manpage")

	# Setup a temp dir under the build for our processing
	file(MAKE_DIRECTORY ${MAN_TMPDIR})

	# We hop through a temporary file to process in definitions for e.g.
	# $ETCDIR.
	add_custom_command(OUTPUT ${ADOC_TMPSRC}
		DEPENDS ${ADOC_SRC}
		COMMAND ${MANSED_CMD} < ${ADOC_SRC} > ${ADOC_TMPSRC}
		COMMENT "Processing ${ADOC_SRC} -> ${ADOC_TMPSRC}"
	)

	# We have to jump through a few hoops here, because a2x gives us no
	# control whatsoever over where the output file goes or what it's
	# named.  Thanks, guys.  So since $ADOC_TMPSRC is in $MAN_TMPDIR,
	# the build output will also be there.  Copy it into place
	set(MANPAGE_TMP ${MAN_TMPDIR}/ctwm.1)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${ADOC_TMPSRC}
		COMMAND ${A2X} --doctype manpage --format manpage ${ADOC_TMPSRC}
		COMMAND mv ${MANPAGE_TMP} ${MANPAGE}
		COMMENT "Processing ${ADOC_TMPSRC} -> ${MANPAGE}"
	)

	# Should do HTML at some point, but asciidoc is so slow (~5 secs) I'm
	# not bothering.  When we switch to asciidoctor (~.2 secs), I'll
	# revisit.

else()
	# No ability to generate it ourselves.  See if we have prebuilt.
	find_file(MAN_PRESRC ctwm.1
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)

	if(MAN_PRESRC)
		# Got it, so use that
		set(HAS_MAN 1)
		message(STATUS "No asciidoc/a2x found, using prebuilt manpage.")
		# Later target handles the rewrites to $MAN_PRESRC -> $MANPAGE
	else()
		# Ruh ro
		message(WARNING "Can't find asciidoc/a2x, and no prebuilt manpage available.")
	endif(MAN_PRESRC)

	# Install prebuilt HTML if we have it generated too.
	find_file(HTML_PRESRC ctwm.1.html
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)

	if(HTML_PRESRC)
		set(HAS_HTML 1)
		message(STATUS "Installing prebuilt HTML manual.")
		# Later target does rewrite/move
	endif(HTML_PRESRC)
endif(ASCIIDOC AND A2X)




# If we're using pregen'd versions, we have to do the subs of build vars
# on the output files, instead of doing it on the asciidoc source like we
# do above when we're building them.
if(MAN_PRESRC)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${MAN_PRESRC}
		COMMAND ${MANSED_CMD} < ${MAN_PRESRC} > ${MANPAGE}
		COMMENT "Processing ${MAN_PRESRC} > ${MANPAGE}"
	)
endif(MAN_PRESRC)
if(HTML_PRESRC)
	add_custom_command(OUTPUT ${MANHTML}
		DEPENDS ${HTML_PRESRC}
		COMMAND ${MANSED_CMD} < ${HTML_PRESRC} > ${MANHTML}
		COMMENT "Processing ${HTML_PRESRC} > ${MANHTML}"
	)
endif(HTML_PRESRC)




# Compress manpage (conditionally).  We could add more magic to allow
# different automatic compression, but that's probably way more involved
# than we need to bother with.  Most systems use gzip, and for the few
# that don't, the packagers can use NOMANCOMPRESS and handle it out of
# band.
if(HAS_MAN AND NOT NOMANCOMPRESS)
	find_program(GZIP_CMD gzip)
	add_custom_command(OUTPUT "${MANPAGE}.gz"
		DEPENDS ${MANPAGE}
		COMMAND ${GZIP_CMD} -nc ${MANPAGE} > ${MANPAGE}.gz
		COMMENT "Building ${MANPAGE}.gz"
	)
	add_custom_target(man ALL DEPENDS "${MANPAGE}.gz")
	set(INSTMAN "${MANPAGE}.gz")
elseif(HAS_MAN)
	add_custom_target(man ALL DEPENDS ${MANPAGE})
	set(INSTMAN ${MANPAGE})
endif(HAS_MAN AND NOT NOMANCOMPRESS)




# If we have (or are building) the HTML, add an easy target for it, and
# define a var for the install process to notice.
if(HAS_HTML)
	add_custom_target(man-html ALL DEPENDS ${MANHTML})
	set(INSTHTML ${MANHTML})
endif(HAS_HTML)
