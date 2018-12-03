# Test various rlayout-related bits
ctwm_simple_unit_test(test_rlayout
	BIN test_layout
	ARGS overlap.tst test.tst
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
