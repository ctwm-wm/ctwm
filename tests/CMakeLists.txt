#
# Automated tests
#

set(CTWMBIN "ctwm")
set(CTWMPATH "${CMAKE_BINARY_DIR}/ctwm")
add_custom_target(test_bins)
add_dependencies(test_bins ctwm)


# Add some infrastructure for building executables for unit tests
function(ctwm_simple_unit_test TNAME)
	set(_options )
	set(_arg_one BIN BINPATH WORKING_DIRECTORY)
	set(_arg_multi ARGS)
	cmake_parse_arguments(_ARGS "${_opts}" "${_arg_one}" "${_arg_multi}"
		${ARGN})

	# Maybe we're passed a specific path to run
	if(_ARGS_BINPATH)
		set(BIN_RUN "${_ARGS_BINPATH}")
	endif()

	# What's the binary name?
	if(_ARGS_BIN)
		set(BIN "${_ARGS_BIN}")
	else()
		set(BIN "${TNAME}")
	endif()

	# Derive path if we only got a name
	if(NOT BIN_RUN)
		set(BIN_RUN $<TARGET_FILE:${BIN}>)
	endif()

	# Building and linking it
	if(NOT TARGET ${BIN})
		add_executable(${BIN} EXCLUDE_FROM_ALL "${BIN}.c")
		target_link_libraries(${BIN} ctwmlib)

		# Few of our tests really need any of the X or other libs we pull in.
		# However, most of the .o's for ctwm itself are going to have some
		# ref out to them somewhere, so any tests that wind up pulling
		# functions from those are going to make the linker want to resolve
		# them when we link ${TNAME}.  So just unconditionally add them and
		# don't worry about which tests may not actually need 'em.
		target_link_libraries(${BIN} ${CTWMLIBS})

		# Add to pre-test target
		add_dependencies(test_bins ${BIN})
	endif()

	# And add the test itself
	add_test(NAME ${TNAME}
		COMMAND ${BIN_RUN} ${_ARGS_ARGS}
		WORKING_DIRECTORY ${_ARGS_WORKING_DIRECTORY}
		)
endfunction()



# First a simple smoke test of the built binary
ctwm_simple_unit_test(run-info
	BIN ${CTWMBIN}
	BINPATH ${CTWMPATH}
	ARGS --info
	)
set_tests_properties(run-info PROPERTIES
	PASS_REGULAR_EXPRESSION "Twm version: "
	)


# Try parsing the system.ctwmrc
# Have to support skipping if no $DISPLAY is set, since we require
# talking to the X server to get far enough to cfgchk.  Should really
# find a way around that, but it's a lot more involved than you'd think,
# so 'till then...
ctwm_simple_unit_test(cfgchk
	BIN ${CTWMBIN}
	BINPATH ${CTWMPATH}
	ARGS --cfgchk -f ${CMAKE_SOURCE_DIR}/system.ctwmrc
	)


# Simple test of m4 preprocessing, but we skip if built without m4.
# XXX Gotta skip if no $DISPLAY here too.  Fix that   :(
# XXX This seems like the simplest way of actually "skip"'ing a test
# based on configure options.  That's nuts.  REQUIRED_FILES sounds like
# it would, but actually causes the test to "not run" and be considered
# failed   :(
if(USE_M4)
	set(_test_m4_cmd
		"${CTWMBIN} --cfgchk -f ${CMAKE_CURRENT_SOURCE_DIR}/test_m4/ctwmrc"
		)
else()
	set(_test_m4_cmd "echo Built without m4, skipping \\; exit 99")
endif()
add_test(NAME test_m4
	COMMAND sh -c ${_test_m4_cmd}
	)
set_tests_properties(test_m4 PROPERTIES
	SKIP_RETURN_CODE 99
	)


# A first run at a unit test
add_subdirectory(util_expand)

# RLayout stuff
add_subdirectory(layout)
