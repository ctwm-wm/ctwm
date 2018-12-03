# Test various rlayout-related bits
ctwm_simple_unit_test(test_rlayout_overlap
	BIN test_layout
	ARGS overlap.tst
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

ctwm_simple_unit_test(test_rlayout_test
	BIN test_layout
	ARGS test.tst
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
