cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Figure our version
file(READ "VERSION" vf_str)
string(STRIP ${vf_str} vf_str)
# Not currently using the split out variants
if(0)
	set(version_file_re "([0-9]+)\\.([0-9]+)\\.([0-9]+)(.*)")
	string(REGEX REPLACE ${version_file_re} "\\1" ctwm_version_major ${vf_str})
	string(REGEX REPLACE ${version_file_re} "\\2" ctwm_version_minor ${vf_str})
	string(REGEX REPLACE ${version_file_re} "\\3" ctwm_version_patch ${vf_str})
	string(REGEX REPLACE ${version_file_re} "\\4" ctwm_version_addl  ${vf_str})
	set(ctwm_version_str "${ctwm_version_major}.${ctwm_version_minor}.${ctwm_version_patch}${ctwm_version_addl}")
else()
	set(ctwm_version_str ${vf_str})
endif()

# Modules we'll need
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

# Break out a bunch of bits into our own include dir
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_files)

# Set a few predef'd vars for our use
include(basic_vars)

# Setup paths for where we install stuff
include(install_paths)


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

# And some build env checks
include(build_env_checks)



#
# Next, find various libs/includes/etc we need, if enabled.
#

# First things first.  If we don't have X, we're going nowhere.
find_package(X11)
if(NOT X11_FOUND)
	# This just isn't feasible...
	message(FATAL_ERROR "Can't find X libs.")
endif(NOT X11_FOUND)
if(NOT X11_INCLUDE_DIR)
	message(FATAL_ERROR "Can't find X includes.")
endif(NOT X11_INCLUDE_DIR)

include_directories(${X11_INCLUDE_DIR})

# Need to link in at least these; double check that we found 'em before
# blindly applying them.  We can seemingly get through the above just
# fine, even if it didn't find all the bits...
foreach(VEXT LIBRARIES Xmu_LIB Xt_LIB)
	if(NOT X11_${VEXT})
		message(FATAL_ERROR "Can't find X11_${VEXT}; missing lib or "
			"-devel package?")
	endif()
	list(APPEND CTWMLIBS ${X11_${VEXT}})
endforeach()


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



#
# Do checks for library functions we need, and enable workarounds for any
# cases we decide are worth handling that way.
#
include(check_funcs_etc)



#
# Now other build flags and the like
#

# Enable warnings by default
if(NOT NO_WARNS)
	add_definitions(${STD_WARNS})
	message(STATUS "Enabling standard warnings.")
endif(NOT NO_WARNS)


# Header files are in both source and build dirs
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})



#
# And the build targets
#

# Generated files
include(gen_source_files)


# We're building ctwm
add_executable(ctwm ${CTWMSRC})
target_link_libraries(ctwm ${CTWMLIBS})


# This doesn't really serve much purpose at the moment, so it's not even
# documented, but the code exists.  So make it buildable.
if(DO_CLIENT)
	add_subdirectory(client)
endif(DO_CLIENT)



#
# And setup the installation
#
include(do_install)


#
# Pull in some CPack config for auto-building packages (like .deb and
# .rpm)
#
include(cpack_setup)


#
# Some targets to support release management stuff; building generated
# files for the tarballs
#
include(mktar_support)


#
# Output various information about what we've figured and what we're
# doing for the builder's edification
#
include(show_build_info)
