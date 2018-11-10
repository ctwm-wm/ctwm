# Test various rlayout-related bits
add_executable(test_layout EXCLUDE_FROM_ALL test_layout.c)
target_link_libraries(test_layout ctwmlib)
# XXX Why don't we need ${CTWMLIBS} here?
add_dependencies(test_bins test_layout)

# And a target to run it
add_test(NAME test_layout
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_layout overlap.tst test.tst
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
