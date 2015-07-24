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


# Fill in version info
set(version_c_in ${CMAKE_CURRENT_SOURCE_DIR}/version.c.in)
set(version_c    ${CMAKE_CURRENT_BINARY_DIR}/version.c)

# If we've got a bzr checkout we can figure the revid from, fill it in.
# Else just copy.
if(IS_BZR_CO AND HAS_BZR)
	add_custom_command(OUTPUT ${version_c}
		DEPENDS ${version_c_in} ${BZR_DIRSTATE_FILE}
		COMMAND ${CMAKE_SOURCE_DIR}/tools/rewrite_version_bzr.sh < ${version_c_in} > ${version_c}
	)
else()
	add_custom_command(OUTPUT ${version_c}
		DEPENDS ${version_c_in}
		COMMAND sed -e 's/%%REVISION%%/NULL/' < ${version_c_in} > ${version_c}
	)
endif(IS_BZR_CO AND HAS_BZR)
