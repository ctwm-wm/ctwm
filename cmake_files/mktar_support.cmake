#
# Rules for supporting tarball builds.  Mostly pregen'ing files that
# require non-assumable dependancies.
#

# First, the lex/yacc output files are definitely on the list.
add_custom_target(mktar_genfiles
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lex.c
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.h
)
