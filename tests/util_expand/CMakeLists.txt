# Test ExpandFilePath()

add_executable(t_efp EXCLUDE_FROM_ALL t_efp.c)
target_link_libraries(t_efp ${CTWMLIBS})
target_link_libraries(t_efp ctwmlib)
add_dependencies(test_bins t_efp)

add_test(NAME t_efp
	COMMAND $<TARGET_FILE:t_efp>
	)
