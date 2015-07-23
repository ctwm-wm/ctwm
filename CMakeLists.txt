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


# Guard against in-tree builds
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" insrc)
if(insrc)
	message(FATAL_ERROR "Please build out of tree; don't run cmake "
			"directly in the source tree.")
endif(insrc)



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



#
# Check our build options and set things based on them
#
include(build_options)



#
# Next find some build tools
#

# Do whatever we need to do to get lex.c ready
include(setup_lex)


# And similar for gram.tab.[ch]
include(setup_yacc)


# Handle building the manual
include(handle_manual)


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
include(install_paths)



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
# And setup the installation
#
include(ctwm_install)
