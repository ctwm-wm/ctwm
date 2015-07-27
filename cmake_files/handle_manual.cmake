#
# Handle stuff related to building the manual in various ways
#


# It's written in asciidoc, so first see if we can find tools to build
# it.  If we can, we'll use it to [re]build the manual.  If not, we may
# have pregen'd versions we can use.  If neither, well, the user's in
# trouble...
find_program(ASCIIDOCTOR asciidoctor)
find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)


# If we have asciidoctor, we need to figure out the version, as manpage
# output is relatively new (unreleased, at the time of this writing).
if(ASCIIDOCTOR)
	execute_process(
		COMMAND asciidoctor --version
		RESULT_VARIABLE _adoctor_result
		OUTPUT_VARIABLE _adoctor_verout
		ERROR_QUIET
	)
	if(NOT ${_adoctor_result} EQUAL "0")
		# Err...
		message(WARNING "Unexpected result trying asciidoctor --version.")
		set(_adoctor_verout "Asciidoctor 0.0.0 FAKE")
	endif()
	unset(_adoctor_result)

	# Break out the version.
	set(_adoctor_veregex "Asciidoctor ([0-9]+\\.[0-9]+\\.[0-9]+).*")
	string(REGEX REPLACE ${_adoctor_veregex} "\\1"
		ASCIIDOCTOR_VERSION ${_adoctor_verout})
	unset(_adoctor_verout)
	unset(_adoctor_veregex)
	#message(STATUS "Found asciidoctor version ${ASCIIDOCTOR_VERSION}")

	# 1.5.3 is the first release that can write manpages natively.  This
	# means 1.5.3 dev versions after a certain point can as well; assume
	# anybody running a 1.5.3 dev is keeping up well enough that it can
	# DTRT too.  We assume any version can do HTML.
	set(ASCIIDOCTOR_CAN_MAN  0)
	set(ASCIIDOCTOR_CAN_HTML 1)
	if(${ASCIIDOCTOR_VERSION} VERSION_GREATER "1.5.2")
		set(ASCIIDOCTOR_CAN_MAN 1)
	elseif(${ASCIIDOCTOR_VERSION} VERSION_LESS "0.0.1")
		set(ASCIIDOCTOR_CAN_HTML 0)
	endif()
endif(ASCIIDOCTOR)



#
# Setup some vars for the various steps in the process
#

# The original source.
set(ADOC_SRC ${SRCDOCDIR}/ctwm.1.adoc)

# Where we build stuff.  Because we need to process the ADOC_SRC to
# replace build paths etc, we need to dump it somewhere.  We could just
# leave it right in the build dir root, but is cleaner.
set(MAN_TMPDIR ${CMAKE_BINARY_DIR}/mantmp)

# Where we copy the source to during rewrites, and then do the actual
# build from.
set(ADOC_TMPSRC ${MAN_TMPDIR}/ctwm.1.adoc)

# Where the end products wind up
set(MANPAGE ${CMAKE_BINARY_DIR}/ctwm.1)
set(MANHTML ${CMAKE_BINARY_DIR}/ctwm.1.html)

# How we rewrite vars in the manual.  I decided not to use
# configure_file() for this, as it opens up too many chances for
# something to accidentally get sub'd, since we assume people will write
# pretty freeform in the manual.
# \-escaped @ needed for pre-3.1 CMake compat and warning avoidance;
# x-ref `cmake --help-policy CMP0053`
set(MANSED_CMD sed -e "s,\@ETCDIR@,${ETCDIR},")

# Pregen'd doc file paths we might have, in case we can't build them
# ourselves.
set(MAN_PRESRC ${SRCDOCDIR}/ctwm.1)
set(HTML_PRESRC ${SRCDOCDIR}/ctwm.1.html)





# Figure what we can build
#
# These are both boolean "We can build this type of output" flags, and
# enums for later code for "What method we use to build this type of
# output".

# If we have asciidoctor, use it to build the HTML.  Else, we could use
# asciidoc, but leave it disabled because it's very slow.
set(MANUAL_BUILD_HTML)
if(ASCIIDOCTOR AND ASCIIDOCTOR_CAN_HTML)
	set(MANUAL_BUILD_HTML asciidoctor)
elseif(ASCIIDOC)
	#set(MANUAL_BUILD_HTML asciidoc)
	message(STATUS "Not enabling HTML manual build; asciidoc is slow.")
endif()

# For the manpage output, asciidoctor has to be of a certain version.  If
# it's not there or not high enough version, we fall back to asciidoc/a2x
# (which is very slow at this too, but we need to build a manpage, so eat
# the expense).
set(MANUAL_BUILD_MANPAGE)
if(ASCIIDOCTOR AND ASCIIDOCTOR_CAN_MAN)
	set(MANUAL_BUILD_MANPAGE asciidoctor)
elseif(A2X)
	set(MANUAL_BUILD_MANPAGE a2x)
endif()


# If we can build stuff, prepare bits for it.
if(MANUAL_BUILD_MANPAGE OR MANUAL_BUILD_HTML)
	# Setup a temp dir under the build for our processing
	file(MAKE_DIRECTORY ${MAN_TMPDIR})

	# We hop through a temporary file to process in definitions for e.g.
	# $ETCDIR.
	add_custom_command(OUTPUT ${ADOC_TMPSRC}
		DEPENDS ${ADOC_SRC}
		COMMAND ${MANSED_CMD} < ${ADOC_SRC} > ${ADOC_TMPSRC}
		COMMENT "Processing ctwm.1.adoc -> mantmp/ctwm.1.adoc"
	)

	# We can't actually make other targets depend on that generated
	# source file, because cmake will try to multi-build it in parallel.
	# To work around, we add a do-nothing custom target that depends on
	# $ADOC_TMPSRC, that uses of it depend on instead of it.
	#
	# x-ref http://public.kitware.com/Bug/view.php?id=12311
	add_custom_target(mk_adoc_tmpsrc DEPENDS ${ADOC_TMPSRC})
endif(MANUAL_BUILD_MANPAGE OR MANUAL_BUILD_HTML)




#
# Building the manpage variant
#
set(HAS_MAN 0)
if(MANUAL_BUILD_MANPAGE)
	# Got the tool to build it
	message(STATUS "Building manpage with ${MANUAL_BUILD_MANPAGE}.")
	set(HAS_MAN 1)

	if(${MANUAL_BUILD_MANPAGE} STREQUAL "asciidoctor")
		# We don't need the hoops for a2x here, since asciidoctor lets us
		# specify the output directly.
		add_custom_command(OUTPUT ${MANPAGE}
			DEPENDS mk_adoc_tmpsrc
			COMMAND ${ASCIIDOCTOR} -b manpage -o ${MANPAGE} ${ADOC_TMPSRC}
			COMMENT "Generating ctwm.1 with asciidoctor."
		)
	elseif(${MANUAL_BUILD_MANPAGE} STREQUAL "a2x")
		# We have to jump through a few hoops here, because a2x gives us no
		# control whatsoever over where the output file goes or what it's
		# named.  Thanks, guys.  So since $ADOC_TMPSRC is in $MAN_TMPDIR,
		# the build output will also be there.  Just move it manually
		# afterward.
		set(MANPAGE_TMP ${MAN_TMPDIR}/ctwm.1)
		add_custom_command(OUTPUT ${MANPAGE}
			DEPENDS mk_adoc_tmpsrc
			COMMAND ${A2X} --doctype manpage --format manpage ${ADOC_TMPSRC}
			COMMAND mv ${MANPAGE_TMP} ${MANPAGE}
			COMMENT "Generating ctwm.1 with a2x."
		)
	else()
		message(FATAL_ERROR "I don't know what to do with that manpage "
			"building type!")
	endif()
elseif(EXISTS ${MAN_PRESRC})
	# Can't build it ourselves, but we've got a prebuilt version.
	message(STATUS "Can't build manpage, using prebuilt version.")
	set(HAS_MAN 1)

	# We still have to do the substitutions like above, but we're doing
	# it on the built version now, rather than the source.
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${MAN_PRESRC}
		COMMAND ${MANSED_CMD} < ${MAN_PRESRC} > ${MANPAGE}
		COMMENT "Processing prebuilt manpage -> ctwm.1"
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
			COMMENT "Compressing ctwm.1.gz"
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

	if(${MANUAL_BUILD_HTML} STREQUAL "asciidoctor")
		add_custom_command(OUTPUT ${MANHTML}
			DEPENDS mk_adoc_tmpsrc
			COMMAND ${ASCIIDOCTOR} -atoc -anumbered -o ${MANHTML} ${ADOC_TMPSRC}
			COMMENT "Generating ctwm.1.html with asciidoctor."
		)
	elseif(${MANUAL_BUILD_HTML} STREQUAL "asciidoc")
		add_custom_command(OUTPUT ${MANHTML}
			DEPENDS mk_adoc_tmpsrc
			COMMAND ${ASCIIDOC} -atoc -anumbered -o ${MANHTML} ${ADOC_TMPSRC}
			COMMENT "Generating ctwm.1.html with asciidoc."
		)
	else()
		message(FATAL_ERROR "I don't know what to do with that HTML manual "
			"building type!")
	endif()
elseif(EXISTS ${HTML_PRESRC})
	# Can't build it ourselves, but we've got a prebuilt version.
	message(STATUS "Can't build HTML manual, using prebuilt version.")
	set(HAS_HTML 1)

	# As with the manpage above, we need to do the processing on the
	# generated version for build options.
	add_custom_command(OUTPUT ${MANHTML}
		DEPENDS ${HTML_PRESRC}
		COMMAND ${MANSED_CMD} < ${HTML_PRESRC} > ${MANHTML}
		COMMENT "Processing prebuilt manual -> ctwm.1.html"
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
