#
# Rules for generated source files
#


# Hand-build deftwmrc.h
set(defh ${CMAKE_CURRENT_BINARY_DIR}/deftwmrc.h)
set(mkdefh ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_deftwmrc.sh)
add_custom_command(OUTPUT ${defh}
	DEPENDS system.ctwmrc ${mkdefh}
	COMMAND ${mkdefh} ${CMAKE_CURRENT_SOURCE_DIR}/system.ctwmrc > ${defh}
)
# Need to do this explicitly for cmake to figure it out
set_source_files_properties(parse.c OBJECT_DEPENDS ${defh})


# Hand-build ctwm_atoms.[ch]
set(ctwm_atoms ctwm_atoms.h ctwm_atoms.c)
add_custom_command(OUTPUT ${ctwm_atoms}
	DEPENDS ctwm_atoms.in
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_atoms.sh ${CMAKE_CURRENT_SOURCE_DIR}/ctwm_atoms.in ctwm_atoms CTWM
)


# Setup config header file
configure_file(ctwm_config.h.in ctwm_config.h ESCAPE_QUOTES)
