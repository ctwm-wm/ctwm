cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Modules we'll need
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

# Break out a bunch of bits into our own include dir
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_files)

# Set a few predef'd vars for our use
include(ctwm_cmake_vars)


# Our base set of sources
set(CTWMSRC
	add_window.c
	clargs.c
	clicktofocus.c
	ctwm.c
	ctwm_atoms.c
	cursor.c
	events.c
	gc.c
	gram.tab.c
	iconmgr.c
	icons.c
	lex.c
	list.c
	menus.c
	mwmhints.c
	otp.c
	parse.c
	resize.c
	session.c
	util.c
	version.c
	vscreen.c
	windowbox.c
	workmgr.c
)

# Libs to link in (init empty list)
set(CTWMLIBS)


# Guard against in-tree builds
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" insrc)
if(insrc)
	message(FATAL_ERROR "Please build out of tree; don't run cmake "
			"directly in the source tree.")
endif(insrc)


#
# Define options
# These can be set at the command line; e.g., "cmake -DUSE_JPEG=OFF"
#
option(USE_XPM    "Enable XPM support"                 ON )
option(USE_JPEG   "Enable libjpeg support"             ON )
option(USE_M4     "Enable m4 support"                  ON )
option(USE_SOUND  "Enable librplay sound support"      OFF)
option(USE_SREGEX "Use regex from libc"                ON )
option(USE_EWMH   "Support some Extended Window Manager Hints"  ON )
option(USE_GNOME  "Enable GNOME1 support"              !USE_EWMH )


#
# First off, do some checks of compiler/stdlib features
#
include(compiler_feature_checks)



#
# Next, find various libs/includes/etc we need, if enabled.
#

# First things first.  If we don't have X, we're going nowhere.
find_package(X11)
if(NOT X11_FOUND)
	# This just isn't feasible...
	message(FATAL_ERROR "Can't find X libs.")
endif(NOT X11_FOUND)

include_directories(${X11_INCLUDE_DIR})
list(APPEND CTWMLIBS ${X11_LIBRARIES})
list(APPEND CTWMLIBS ${X11_Xmu_LIB})
list(APPEND CTWMLIBS ${X11_Xt_LIB})


#
# Setup some search paths
#
set(INCSEARCH
	"${CMAKE_INSTALL_PREFIX}/include"
	${X11_INCLUDE_DIR}
	"/usr/local/include"
	"/usr/include"
)
set(LIBSEARCH
	"${CMAKE_INSTALL_PREFIX}/lib"
	${X11_LIBRARY_DIR}
	"/usr/local/lib"
	"/usr/lib"
	"/lib"
)



# Hard to imagine xpm not being around or somebody not wanting it, but...
if(USE_XPM)
	if(NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "Couldn't find XPM libs")
	endif(NOT X11_Xpm_FOUND)

	list(APPEND CTWMLIBS ${X11_Xpm_LIB})
	message(STATUS "Enabling XPM support: ${X11_Xpm_LIB}.")
endif(USE_XPM)


# libjpeg is pretty common
if(USE_JPEG)
	find_package(JPEG)
	if(NOT JPEG_FOUND)
		message(FATAL_ERROR "Couldn't find libjpeg")
	endif()

	include_directories(${JPEG_INCLUDE_DIR})
	list(APPEND CTWMLIBS ${JPEG_LIBRARIES})
	message(STATUS "Enabling libjpeg support.")
endif(USE_JPEG)


# m4 is on by default too
if(USE_M4)
	if(NOT M4_CMD)
		find_program(M4_CMD m4 gm4)
	endif(NOT M4_CMD)
	if(NOT M4_CMD)
		message(FATAL_ERROR "Can't find m4 program: try setting M4_CMD.")
	endif(NOT M4_CMD)
	list(APPEND CTWMSRC parse_m4.c)
	message(STATUS "Enabling m4 support (${M4_CMD}).")
endif(USE_M4)


# rplay off by default
if(USE_SOUND)
	find_library(LIBRPLAY NAMES rplay PATHS ${LIBSEARCH})
	if(NOT LIBRPLAY)
		message(FATAL_ERROR "Can't find librplay lib.")
	endif(NOT LIBRPLAY)
	find_path(LIBRPLAY_INCLUDE_DIR NAME rplay.h PATHS ${INCSEARCH})
	if(NOT LIBRPLAY_INCLUDE_DIR)
		message(FATAL_ERROR "Can't find rplay.h.")
	endif(NOT LIBRPLAY_INCLUDE_DIR)

	list(APPEND CTWMSRC sound.c)
	list(APPEND CTWMLIBS ${LIBRPLAY})
	include_directories(${LIBRPLAY_INCLUDE_DIR})
	message(STATUS "Enabling librplay sound support.")
endif(USE_SOUND)


# Check if the user wants EWMH support built in.
if(USE_EWMH)
    # Hand-build ewmh_atoms.[ch]
	set(ewmh_atoms ewmh_atoms.h ewmh_atoms.c)
	add_custom_command(OUTPUT ${ewmh_atoms}
		DEPENDS ewmh_atoms.in
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_atoms.sh ${CMAKE_CURRENT_SOURCE_DIR}/ewmh_atoms.in ewmh_atoms EWMH
    )

	list(APPEND CTWMSRC ewmh.c ewmh_atoms.c)
	message(STATUS "Enabling Extended Window Manager Hints support.")
else()
	message(STATUS "Disabling Extended Window Manager Hints support.")
endif(USE_EWMH)


# Do build Gnome support bits by default
if(USE_GNOME)
	list(APPEND CTWMSRC gnome.c)
	message(STATUS "Enabling GNOME1 support.")
else()
	message(STATUS "Disabling GNOME1 support.")
endif(USE_GNOME)


# System provides regex stuff in libc?
if(USE_SREGEX)
	check_include_files(regex.h HAS_REGEX_H)
	check_function_exists(regexec HAS_REGEXEC)

	if(NOT HAS_REGEX_H)
		message(FATAL_ERROR "Can't find regex.h")
	endif(NOT HAS_REGEX_H)
	if(NOT HAS_REGEXEC)
		message(FATAL_ERROR "Can't find regexec()")
	endif(NOT HAS_REGEXEC)

	message(STATUS "Enabling libc regex usage.")
else()
	message(FATAL_ERROR "USE_SREGEX=OFF no longer supported.")
endif(USE_SREGEX)



#
# Next find some build tools
#

# Do whatever we need to do to get lex.c ready
include(setup_lex)


# And similar for gram.tab.[ch]
include(setup_yacc)


# Find asciidoc stuff to build manual, if we can.  Else, use the ones
# we'll pre-build for the tarball.  Else, uh, we're in trouble...
set(MANPAGE_TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/mantmp)
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1)
set(MANHTML ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.html)
set(ADOC_SRC ${CMAKE_CURRENT_SOURCE_DIR}/doc/ctwm.1.adoc)
set(ADOC_TMPSRC ${MANPAGE_TMPDIR}/ctwm.1.adoc)
set(HAS_MAN 0)
set(HAS_HTML 0)
set(CAN_BUILD_MAN 0)

find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)
if(ASCIIDOC AND A2X)
	set(HAS_MAN 1)
	set(CAN_BUILD_MAN 1)

	# Generate .in file, processed into man format
	# We have to jump through a few hoops, because a2x gives us no
	# control whatsoever over where the output file goes or what it's
	# named.  Thanks, guys.
	message(STATUS "Found asciidoc (${A2X}) for building manpage")

	# Setup a temp dir under the build for our processing
	file(MAKE_DIRECTORY ${MANPAGE_TMPDIR})

	# We hop through a temporary file to process in definitions for e.g.
	# $ETCDIR.  Can't do that here though since the directories aren't
	# defined yet, so we write the target for ADOC_TMPSRC lower down.

	set(MANPAGE_TMP ${MANPAGE_TMPDIR}/ctwm.1)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${ADOC_TMPSRC}
		COMMAND ${A2X} --doctype manpage --format manpage ${ADOC_TMPSRC}
		COMMAND cp ${MANPAGE_TMP} ${MANPAGE}
		COMMENT "Processing ${ADOC_TMPSRC} -> ${MANPAGE}"
	)

	# Should do HTML at some point, but asciidoc is so slow (~5 secs) I'm
	# not bothering.  When we switch to asciidoctor (~.2 secs), I'll
	# revisit.
else()
	# See if we have prebuilt
	find_file(MAN_PRESRC ctwm.1
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)
	if(MAN_PRESRC)
		set(HAS_MAN 1)
		message(STATUS "No asciidoc/a2x found, using prebuilt manpage.")
		# Target below does rewrites
	else()
		message(WARNING "Can't find asciidoc/a2x, and no prebuilt manpage available.")
	endif(MAN_PRESRC)

	# Install prebuilt HTML if we have it generated too.
	find_file(HTML_PRESRC ctwm.1.html
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)
	if(HTML_PRESRC)
		set(HAS_HTML 1)
		message(STATUS "Installing prebuilt HTML manual.")
	endif(HTML_PRESRC)
endif(ASCIIDOC AND A2X)


# getopt_long(3) is in getopt.h everywhere I can find it.  Until we find
# a system where it's somewhere else, or systems it's nowhere and we have
# to provide it, we'll either find it there or error out and blow up.  If
# we don't find problems that require additional workaround in the next
# couple releases, we should retire the check to avoid wasting time/space
# checking things we know will work.
check_include_files(getopt.h HAS_GETOPT_H)
check_function_exists(getopt_long HAS_GETOPT_LONG)
if(NOT HAS_GETOPT_H)
	message(FATAL_ERROR "Can't find getopt.h (needed for getopt_long()).")
endif(NOT HAS_GETOPT_H)
if(NOT HAS_GETOPT_LONG)
	message(FATAL_ERROR "You don't seem to have getopt_long().")
endif(NOT HAS_GETOPT_LONG)



#
# Figure out install paths for stuff
#

# CMAKE_INSTALL_PREFIX is the user's responsibility to set right.  We
# just assume it is.

# Where ctwm looks for fallback config files
if(NOT ETCDIR)
	set(ETCDIR ${CMAKE_INSTALL_PREFIX}/etc)
endif(NOT ETCDIR)
message(STATUS "System-wide config in      ${ETCDIR}")

# Where we stick the binary
if(NOT BINDIR)
	set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
endif(NOT BINDIR)
message(STATUS "Installing ctwm in         ${BINDIR}")

# Where runtime data stuff (e.g., images) is found
if(NOT DATADIR)
	set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/ctwm)
endif(NOT DATADIR)
message(STATUS "Installing runtime data in ${DATADIR}")

# Where we install non-manpage docs to
if(NOT DOCDIR)
	set(DOCDIR ${CMAKE_INSTALL_PREFIX}/share/doc/ctwm)
endif(NOT DOCDIR)
message(STATUS "Installing docs in         ${DOCDIR}")

# Where we put example configs etc.
if(NOT EXAMPLEDIR)
	set(EXAMPLEDIR ${CMAKE_INSTALL_PREFIX}/share/examples/ctwm)
endif(NOT EXAMPLEDIR)
message(STATUS "Installing examples in     ${EXAMPLEDIR}")


# Where should we stick manpages?
# User can set MANDIR to override, if they don't, check a few likely
# places...
if(NOT MANDIR)
	find_file(MANDIR man
		PATHS
			"${CMAKE_INSTALL_PREFIX}/share"
			"${CMAKE_INSTALL_PREFIX}"
		NO_DEFAULT_PATH)
endif(NOT MANDIR)

# If we can't find something, hardcode a guess and yell about it
if(NOT MANDIR)
	set(MANDIR "${CMAKE_INSTALL_PREFIX}/man")
	message(WARNING "Can't find install base for manpages, "
			"assuming ${MANDIR}")
endif(NOT MANDIR)

set(MAN1PATH "${MANDIR}/man1")
message(STATUS "Installing manpage to      ${MAN1PATH}")



#
# Now other build flags and the like
#

# Standard defs
set(STD_WARNS
	-Wall
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef
	-Wredundant-decls -Wcast-align -Wcast-qual -Wchar-subscripts
	-Winline -Wnested-externs -Wmissing-declarations
)
if(NOT NO_WARNS)
	add_definitions(${STD_WARNS})
endif(NOT NO_WARNS)

# Set images dir.  Can't do this in the XPM-finding block above, since we
# haven't figured the path yet.
if(USE_XPM)
	set(PIXMAPDIR ${DATADIR}/images)
endif(USE_XPM)


# Include source and generated files
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})



#
# And the build targets
#

# Hand-build deftwmrc.h
set(defh ${CMAKE_CURRENT_BINARY_DIR}/deftwmrc.h)
set(mkdefh ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_deftwmrc.sh)
add_custom_command(OUTPUT ${defh}
	DEPENDS system.ctwmrc ${mkdefh}
	COMMAND ${mkdefh} ${CMAKE_CURRENT_SOURCE_DIR}/system.ctwmrc > ${defh}
)
# Need to do this explicitly for cmake to figure it out
set_source_files_properties(parse.c OBJECT_DEPENDS ${defh})

# Hand-build ctwm_atoms.[ch]
set(ctwm_atoms ctwm_atoms.h ctwm_atoms.c)
add_custom_command(OUTPUT ${ctwm_atoms}
	DEPENDS ctwm_atoms.in
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_atoms.sh ${CMAKE_CURRENT_SOURCE_DIR}/ctwm_atoms.in ctwm_atoms CTWM
)

# Setup config file
configure_file(ctwm_config.h.in ctwm_config.h ESCAPE_QUOTES)

# We're building ctwm
add_executable(ctwm ${CTWMSRC})
target_link_libraries(ctwm ${CTWMLIBS})


# This doesn't really serve much purpose at the moment, so it's not even
# documented, but the code exists.  So make it buildable.
if(DO_CLIENT)
	include_directories(client)
	add_subdirectory(client)
endif(DO_CLIENT)

# Build the event-forwarding program.
add_executable(forward forward.c)
target_link_libraries(forward ${X11_LIBRARIES})

# Rewrite some build options into docs, either into the adoc source
# before processing, or into prebuilt versions if we can't generate them
# ourselves.
set(MANSED_CMD sed -e "s,\@ETCDIR@,${ETCDIR},")
if(CAN_BUILD_MAN)
	add_custom_command(OUTPUT ${ADOC_TMPSRC}
		DEPENDS ${ADOC_SRC}
		COMMAND ${MANSED_CMD} < ${ADOC_SRC} > ${ADOC_TMPSRC}
		COMMENT "Processing ${ADOC_SRC} -> ${ADOC_TMPSRC}"
	)
endif(CAN_BUILD_MAN)
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

if(HAS_HTML)
	add_custom_target(man-html ALL DEPENDS ${MANHTML})
	set(INSTHTML ${MANHTML})
endif(HAS_HTML)


#
# And the install process
#

# Some bits we always install; the binary, the default config, and our
# CHANGES file.
install(TARGETS ctwm
	DESTINATION ${BINDIR}
)
install(FILES system.ctwmrc
	DESTINATION ${EXAMPLEDIR}
)
install(FILES README.md CHANGES.md
	DESTINATION ${DOCDIR}
)

# If we's using XPM (really, when are we not?), install the pixmaps.
if(USE_XPM)
	install(DIRECTORY xpm/
		DESTINATION ${PIXMAPDIR}
		FILES_MATCHING PATTERN "*.xpm"
	)
endif(USE_XPM)

# Install manual bits, assuming we have them.
if(NOT HAS_MAN)
	message(WARNING "No manpage to install: recheck config if "
			"this is unexpected.")
else()
	install(FILES ${INSTMAN}
		DESTINATION ${MAN1PATH}
	)
endif(NOT HAS_MAN)
if(INSTHTML)
	install(FILES ${INSTHTML}
		DESTINATION ${DOCDIR}
	)
endif(INSTHTML)

# If we're building with sound, install its doc too.
if(USE_SOUND)
	install(FILES sound.txt
		DESTINATION ${DOCDIR}
	)
endif(USE_SOUND)

# If building with GNOME1 bits, install its doc too.
if(USE_GNOME)
	install(FILES README.gnome
		DESTINATION ${DOCDIR}
	)
endif(USE_GNOME)
