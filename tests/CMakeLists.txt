#
# Automated tests
#

# First a few simple smoke tests of the built binary
add_test(NAME run-info
	COMMAND ${CMAKE_BINARY_DIR}/ctwm --info
	)
set_tests_properties(run-info PROPERTIES
	PASS_REGULAR_EXPRESSION "Twm version: "
	)
