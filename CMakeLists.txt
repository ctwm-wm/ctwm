cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Modules we'll need
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

# Our base set of sources
set(CTWMSRC
	add_window.c
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


# Expect and try to enforce a C99 capable compiler.  There doesn't seem
# an obvious way to be sure in a fully portable way, but we probably
# don't work well in places that compile with something other than a
# program called like 'cc', and a cc that supports C99 should accept -std
# calls, so that's good enough.  Lacking it is not (yet) a fatal error,
# but is a sign that it's a compiler or platform we're moving further
# away from.
include(CheckCCompilerFlag)
set(C99_FLAG -std=c99)
check_c_compiler_flag(${C99_FLAG} COMPILER_HAS_STD_C99)
if(COMPILER_HAS_STD_C99)
	message(STATUS "Enabling ${C99_FLAG}")
	add_definitions(${C99_FLAG})
else()
	message(WARNING "Compiler doesn't support ${C99_FLAG}, "
			"building without it.")
endif(COMPILER_HAS_STD_C99)


# With -std=c99, GNU libc's includes get strict about what they export.
# Particularly, a lot of POSIX stuff doesn't get defined unless we
# explicitly ask for it.  Do our best at checking for what's there...
set(USE_GLIBC_FEATURES_H OFF)
check_include_files(features.h HAS_FEATURES_H)
if(HAS_FEATURES_H)
	# Check if including it with our args sets __USE_ISOC99; that's a
	# sign it's what we're looking for here.
	check_symbol_exists(__USE_ISOC99 features.h SETS_USE_ISOC99)
	if(SETS_USE_ISOC99)
		# OK, it does.  Assume that's a good enough test that things are
		# acting as we expect, and set the flag for our ctwm_config.h.
		set(USE_GLIBC_FEATURES_H ON)
		message(STATUS "Enabling glibc features.h settings.")
	endif()
endif(HAS_FEATURES_H)



#
# First find various libs/includes/etc we need, if enabled.
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


# Now we know where to stash this (may change this to be a little less
# coupled to X11 later).
set(TWMDIR ${X11_LIBRARY_DIR}/X11/twm)
set(CONFDIR ${TWMDIR})


# Hard to imagine xpm not being around or somebody not wanting it, but...
if(USE_XPM)
	if(NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "Couldn't find XPM libs")
	endif(NOT X11_Xpm_FOUND)

	list(APPEND CTWMLIBS ${X11_Xpm_LIB})
	set(PIXMAPDIR ${TWMDIR}/images)
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
endif(USE_SREGEX)



#
# Next find some build tools
#

# flex should be everywhere
find_package(FLEX)
if(FLEX_FOUND)
	FLEX_TARGET(ctwm_lexer lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.c)
else()
	# See if we have a pre-built one
	find_file(LEX_C lex.c
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}
		NO_DEFAULT_PATH)
	if(LEX_C)
		# Make the build process just copy it in
		message(STATUS "No flex found, using prebuilt lex.c")
		add_custom_command(OUTPUT lex.c
			DEPENDS ${LEX_C}
			COMMAND cp ${LEX_C} .
		)
	else()
		# No flex, no pre-built lex.c
		message(FATAL_ERROR "Can't find flex.")
	endif(LEX_C)
endif(FLEX_FOUND)


# This only finds bison, not yacc.  There doesn't seem to be a standard
# module for yacc, so hand-code it.
set(YFLAGS -d -b gram)
if(DO_DEBUGPARSER)
	list(APPEND YFLAGS -t -v)
	add_definitions(-DYYEBUG=1)
	message(STATUS "Enabling config parser debug.")
endif(DO_DEBUGPARSER)
find_package(BISON)
if(BISON_FOUND)
	BISON_TARGET(ctwm_parser gram.y ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
		COMPILE_FLAGS ${YFLAGS})
else()
	find_program(YACC yacc)
	if(YACC)
		# Got yacc(1), use it
		message(STATUS "Found yacc: ${YACC}")
		add_custom_command(OUTPUT gram.tab.c gram.tab.h
			DEPENDS gram.y
			COMMAND ${YACC} ${YFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/gram.y
		)
	else()
		# No bison, no yacc.  Maybe there are prebuilt files?
		find_file(GRAM_C gram.tab.c
			PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
		find_file(GRAM_H gram.tab.h
			PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
		if(GRAM_C AND GRAM_H)
			# Got prebuilt ones, use 'em
			message(STATUS "No yacc found, using prebuilt gram.tab.*")
			add_custom_command(OUTPUT gram.tab.h
				DEPENDS ${GRAM_H}
				COMMAND cp ${GRAM_H} .
			)
			add_custom_command(OUTPUT gram.tab.c
				DEPENDS ${GRAM_C}
				COMMAND cp ${GRAM_C} .
			)
		else()
			# No bison, no yacc, no prebuilt.  Boom.
			message(FATAL_ERROR "Can't find yacc.")
		endif(GRAM_C AND GRAM_H)
	endif(YACC)
endif(BISON_FOUND)


# Find asciidoc stuff to build manual, if we can.  Else, use the ones
# we'll pre-build for the tarball.  Else, uh, we're in trouble...
set(MANPAGE_ADSRC ${CMAKE_CURRENT_SOURCE_DIR}/doc/ctwm.1.adoc)
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1)
set(CAN_DO_MAN 0)
find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)
if(ASCIIDOC AND A2X)
	set(CAN_DO_MAN 1)

	# Generate .in file, processed into man format
	# We have to jump through a few hoops, because a2x gives us no
	# control whatsoever over where the output file goes or what it's
	# named.  Thanks, guys.
	message(STATUS "Found asciidoc (${A2X}) for building manpage")
	set(MANPAGE_TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/mantmp)
	file(MAKE_DIRECTORY ${MANPAGE_TMPDIR})
	set(MANPAGE_TMPIN ${MANPAGE_TMPDIR}/ctwm.1)
	set(MANPAGE_IN ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.in)
	add_custom_command(OUTPUT ${MANPAGE}.in
		DEPENDS ${MANPAGE_ADSRC}
		COMMAND cp ${MANPAGE_ADSRC} ${MANPAGE_TMPIN}.adoc
		COMMAND ${A2X} --doctype manpage --format manpage ${MANPAGE_TMPIN}.adoc
		COMMAND cp ${MANPAGE_TMPIN} ${MANPAGE_IN}
		COMMENT "Processing ${MANPAGE_ADSRC} -> ${MANPAGE}.in"
	)
else()
	# See if we have prebuilt
	find_file(MAN_PRESRC ctwm.1
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}/doc
		NO_DEFAULT_PATH)
	if(MAN_PRESRC)
		set(CAN_DO_MAN 1)
		# Just copy it in
		add_custom_command(OUTPUT ${MANPAGE}.in
			DEPENDS ${MAN_PRESRC}
			COMMAND cp ${MAN_PRESRC} ${MANPAGE}.in
			COMMENT "Copying in pre-built manpage."
		)
		message(STATUS "No asciidoc/a2x found, using prebuilt manpage.")
	else()
		message(WARNING "Can't find asciidoc/a2x, and no prebuilt manpage available.")
	endif(MAN_PRESRC)
endif(ASCIIDOC AND A2X)



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


# Include source and generated files
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})



#
# Figure out install paths for stuff
#

# Where should we stick manpages?
if(MANBASE)
	# Assume the user knows what they're doing
else()
	find_file(MANBASE man
		PATHS
			"${CMAKE_INSTALL_PREFIX}/share"
			"${CMAKE_INSTALL_PREFIX}"
		NO_DEFAULT_PATH)
endif(MANBASE)

if(MANBASE)
	set(MAN1PATH "${MANBASE}/man1")
	message(STATUS "Installing manpage to ${MAN1PATH}")
else()
	# Wacky dance to make sure it's a string rather than a list, or else
	# some later uses of it (see faked install() below) wind up with
	# embedded semicolons.  Only way to define it as a plain string with
	# set() would be as a single crazy long line, so we do a string
	# replace instead.
	string(REPLACE ";" " " manbase_err
			"Can't figure out where to install manpage: "
			"try setting MANBASE to something appropriate for your system "
			"(e.g., /usr/share/man).")
	message(WARNING ${manbase_err})
endif(MANBASE)



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

# Rewrite some build options into manpage
if(CAN_DO_MAN)
	add_custom_command(OUTPUT ${MANPAGE}
		DEPENDS ${MANPAGE}.in
		COMMAND sed -e "s,\@TWMDIR@,${TWMDIR}," < ${MANPAGE}.in > ${MANPAGE}
		COMMENT "Processing ${MANPAGE}.in -> ${MANPAGE}"
	)
else()
	# Fake to get us through the build process
	file(WRITE ${MANPAGE} "Can't build manpage.")
endif(CAN_DO_MAN)

# Compress manpage (conditionally).  We could add more magic to allow
# different automatic compression, but that's probably way more involved
# than we need to bother with.  Most systems use gzip, and for the few
# that don't, the packagers can use NOMANCOMPRESS and handle it out of
# band.
if(CAN_DO_MAN AND NOT NOMANCOMPRESS)
	find_program(GZIP_CMD gzip)
	add_custom_command(OUTPUT "${MANPAGE}.gz"
		DEPENDS ${MANPAGE}
		COMMAND ${GZIP_CMD} -nc ${MANPAGE} > ${MANPAGE}.gz
		COMMENT "Building ${MANPAGE}.gz"
	)
	add_custom_target(man ALL DEPENDS "${MANPAGE}.gz")
	set(INSTMAN "${MANPAGE}.gz")
else()
	set(INSTMAN ${MANPAGE})
endif(CAN_DO_MAN AND NOT NOMANCOMPRESS)


#
# And the install process
#

# Cram a warning in about the MANBASE if we couldn't find it.  This is a
# particularly grody way of doing things, but we can't just add it as a
# standard target that install depends on (see
# http://public.kitware.com/Bug/view.php?id=8438).  We rely on the fact
# that install() is defined to run in order, but that only happens when
# everything is in a single directory.  This will break if/when we start
# splitting things up...
if(NOT CAN_DO_MAN)
	install(CODE "message(FATAL_ERROR \"Failed to build manpage; see earlier errors.\")")
endif(NOT CAN_DO_MAN)
if(NOT MANBASE)
	install(CODE "message(FATAL_ERROR \"${manbase_err}\")")
endif(NOT MANBASE)

# Installing everything
install(TARGETS ctwm
	DESTINATION bin
)
if(USE_XPM)
	install(DIRECTORY xpm/
		DESTINATION ${PIXMAPDIR}
		FILES_MATCHING PATTERN "*.xpm"
	)
endif(USE_XPM)
install(FILES system.ctwmrc
	DESTINATION ${CONFDIR}
)
if(MANBASE)
	install(FILES ${INSTMAN}
		DESTINATION ${MAN1PATH}
	)
endif(MANBASE)
