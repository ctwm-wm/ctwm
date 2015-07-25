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
set(ADOC_SRC ${SRCDOCDIR}/ctwm.1.adoc)

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

# Pregen'd doc file paths we might have, in case we can't build them
# ourselves.
set(MAN_PRESRC ${SRCDOCDIR}/ctwm.1)
set(HTML_PRESRC ${SRCDOCDIR}/ctwm.1.html)





#
# The build process
#

# Can we build the manual?  And what tools do we use for it?
set(CAN_BUILD_MANUAL 0)
set(MANUAL_BUILD_MANPAGE)
set(MANUAL_BUILD_HTML)

if(ASCIIDOC AND A2X)
	set(CAN_BUILD_MANUAL 1)
	set(MANUAL_BUILD_MANPAGE a2x)
	# HTML build disabled for time reasons
	#set(MANUAL_BUILD_HTML asciidoc)
endif(ASCIIDOC AND A2X)


# If we can build stuff, prep for doing it.
if(MANUAL_BUILD_MANPAGE OR MANUAL_BUILD_HTML)
	# Setup a temp dir under the build for our processing
	file(MAKE_DIRECTORY ${MAN_TMPDIR})

	# We hop through a temporary file to process in definitions for e.g.
	# $ETCDIR.
	add_custom_command(OUTPUT ${ADOC_TMPSRC}
		DEPENDS ${ADOC_SRC}
		COMMAND ${MANSED_CMD} < ${ADOC_SRC} > ${ADOC_TMPSRC}
		COMMENT "Processing ${ADOC_SRC} -> ${ADOC_TMPSRC}"
	)
endif(MANUAL_BUILD_MANPAGE OR MANUAL_BUILD_HTML)


#
# Building the manpage variant
#
set(HAS_MAN 0)
if(MANUAL_BUILD_MANPAGE)
	# Got the tool to build it
	message(STATUS "Building manpage with ${MANUAL_BUILD_MANPAGE}.")
	set(HAS_MAN 1)

	# We have to jump through a few hoops here, because a2x gives us no
	# control whatsoever over where the output file goes or what it's
	# named.  Thanks, guys.  So since $ADOC_TMPSRC is in $MAN_TMPDIR,
	# the build output will also be there.  Copy it into place
	set(MANPAGE_TMP ${MAN_TMPDIR}/ctwm.1)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${ADOC_TMPSRC}
		COMMAND ${A2X} --doctype manpage --format manpage ${ADOC_TMPSRC}
		COMMAND mv ${MANPAGE_TMP} ${MANPAGE}
		COMMENT "Generating ${ADOC_TMPSRC} -> ${MANPAGE}"
	)
elseif(EXISTS ${MAN_PRESRC})
	# Can't build it ourselves, but we've got a prebuilt version.
	message(STATUS "Can't build manpage, using prebuilt version.")
	set(HAS_MAN 1)

	# We still have to do the substitutions like above, but we're doing
	# it on the built version now, rather than the source.
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${MAN_PRESRC}
		COMMAND ${MANSED_CMD} < ${MAN_PRESRC} > ${MANPAGE}
		COMMENT "Processing ${MAN_PRESRC} > ${MANPAGE}"
	)
else()
	# Can't build it, no prebuilt.  Not quite fatal, but very bad.
	message(WARNING "Can't build manpage, and no prebuilt version "
		"available.  You won't get one.")
endif(MANUAL_BUILD_MANPAGE)


# Assuming we have it, compress manpage (conditionally).  We could add
# more magic to allow different automatic compression, but that's
# probably way more involved than we need to bother with.  Most systems
# use gzip, and for the few that don't, the packagers can use
# NOMANCOMPRESS and handle it out of band.
if(HAS_MAN)
	if(NOT NOMANCOMPRESS)
		find_program(GZIP_CMD gzip)
		add_custom_command(OUTPUT "${MANPAGE}.gz"
			DEPENDS ${MANPAGE}
			COMMAND ${GZIP_CMD} -nc ${MANPAGE} > ${MANPAGE}.gz
			COMMENT "Compressing ${MANPAGE}.gz"
		)
		add_custom_target(man ALL DEPENDS "${MANPAGE}.gz")
		set(INSTMAN "${MANPAGE}.gz")
	else()
		add_custom_target(man ALL DEPENDS ${MANPAGE})
		set(INSTMAN ${MANPAGE})
	endif(NOT NOMANCOMPRESS)
endif(HAS_MAN)



#
# Do the HTML manual
#
set(HAS_HTML 0)
if(MANUAL_BUILD_HTML)
	# Got the tool to build it
	message(STATUS "Building HTML manual with ${MANUAL_BUILD_HTML}.")
	set(HAS_HTML 1)

	add_custom_command(OUTPUT ${MANHTML}
		DEPENDS ${ADOC_TMPSRC}
		COMMAND ${ASCIIDOC} -atoc -anumbered -o ${MANHTML} ${ADOC_TMPSRC}
		COMMENT "Generating ${ADOC_TMPSRC} -> ${MANHTML}"
	)

elseif(EXISTS ${HTML_PRESRC})
	# Can't build it ourselves, but we've got a prebuilt version.
	message(STATUS "Can't build HTML manual , using prebuilt version.")
	set(HAS_HTML 1)

	# As with the manpage above, we need to do the processing on the
	# generated version for build options.
	add_custom_command(OUTPUT ${MANHTML}
		DEPENDS ${HTML_PRESRC}
		COMMAND ${MANSED_CMD} < ${HTML_PRESRC} > ${MANHTML}
		COMMENT "Processing ${HTML_PRESRC} > ${MANHTML}"
	)

else()
	# Can't build it, no prebuilt.
	# Left as STATUS, since this is "normal" for now.
	message(STATUS "Can't build HTML manual, and no prebuilt version "
		"available.")
endif(MANUAL_BUILD_HTML)


# If we have (or are building) the HTML, add an easy target for it, and
# define a var for the install process to notice.
if(HAS_HTML)
	add_custom_target(man-html ALL DEPENDS ${MANHTML})
	set(INSTHTML ${MANHTML})
endif(HAS_HTML)
