#
# Automated tests
#

set(CTWMBIN "${CMAKE_BINARY_DIR}/ctwm")
add_custom_target(test_bins)
add_dependencies(test_bins ctwm)


# Add some infrastructure for building executables for unit tests
macro(ctwm_simple_unit_test BIN)
	# Building and linking it
	add_executable(${BIN} EXCLUDE_FROM_ALL "${BIN}.c")
	target_link_libraries(${BIN} ${CTWMLIBS} ctwmlib)

	# Add to pre-test target
	add_dependencies(test_bins ${BIN})

	# And add the test itself
	add_test(NAME ${BIN}
		COMMAND $<TARGET_FILE:${BIN}>
		)
endmacro()



# First a simple smoke test of the built binary
add_test(NAME run-info
	COMMAND ${CTWMBIN} --info
	)
set_tests_properties(run-info PROPERTIES
	PASS_REGULAR_EXPRESSION "Twm version: "
	)


# Try parsing the system.ctwmrc
add_test(NAME cfgchk
	COMMAND ${CTWMBIN} --cfgchk -f ${CMAKE_SOURCE_DIR}/system.ctwmrc)


# A first run at a unit test
add_subdirectory(util_expand)
