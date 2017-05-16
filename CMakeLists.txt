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


#
# Most of our bits are broken out into smaller files in a local dir
#
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_files)

# Setup basic vars for where our pieces are, list of source files, etc.
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
# First off, do some checks of compiler/stdlib features and our build
# environment.
#
include(compiler_feature_checks)
include(vcs_checks)



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

# Header files are in both source and build dirs
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


#
# Check our build options and set things based on them
#
include(build_options)


#
# Some additional checks and lookups
#

# Find some build tools used for generating sources and manuals
include(setup_lex)
include(setup_yacc)
include(handle_manual)

# Do checks for library functions we need, and enable workarounds for any
# cases we decide are worth handling that way.
include(check_funcs_etc)



# Enable warnings by default
if(NOT NO_WARNS)
	add_definitions(${STD_WARNS})
	message(STATUS "Enabling standard warnings.")
endif(NOT NO_WARNS)



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
