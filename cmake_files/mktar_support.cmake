#
# Rules for supporting tarball builds.  Mostly pregen'ing files that
# require non-assumable dependancies.
#

set(MKTAR_GENFILES "${CMAKE_CURRENT_BINARY_DIR}/MKTAR_GENFILES")

# First, the lex/yacc output files are definitely on the list.
add_custom_target(mktar_genfiles
	COMMENT "Building generated files for tarball."
	DEPENDS ${MKTAR_GENFILES}
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/lex.c
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.h
)

add_custom_command(OUTPUT ${MKTAR_GENFILES}
	COMMENT "touch MKTAR_GENFILES"
	COMMAND touch ${MKTAR_GENFILES}
)
