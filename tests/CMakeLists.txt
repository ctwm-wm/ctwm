#
# Automated tests
#

set(CTWMBIN "${CMAKE_BINARY_DIR}/ctwm")
add_custom_target(test_bins)

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
