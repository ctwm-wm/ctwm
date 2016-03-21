#
# Rules for generated source files
#


# Hand-build deftwmrc.c
set(defc ${CMAKE_CURRENT_BINARY_DIR}/deftwmrc.c)
set(mkdefc ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_deftwmrc.sh)
add_custom_command(OUTPUT ${defc}
	DEPENDS system.ctwmrc ${mkdefc}
	COMMAND ${mkdefc} ${CMAKE_CURRENT_SOURCE_DIR}/system.ctwmrc > ${defc}
)


# Hand-build ctwm_atoms.[ch]
set(ctwm_atoms ctwm_atoms.h ctwm_atoms.c)
add_custom_command(OUTPUT ${ctwm_atoms}
	DEPENDS ctwm_atoms.in
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_atoms.sh ${CMAKE_CURRENT_SOURCE_DIR}/ctwm_atoms.in ctwm_atoms CTWM
)


# Generate up the event names lookup source
set(en_list ${CMAKE_CURRENT_SOURCE_DIR}/event_names.list)
set(en_in   ${CMAKE_CURRENT_SOURCE_DIR}/event_names.c.in)
set(en_out  ${CMAKE_CURRENT_BINARY_DIR}/event_names.c)
set(en_mk   ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_event_names.sh)
add_custom_command(OUTPUT ${en_out}
	DEPENDS ${en_list} ${en_in} ${en_mk}
	COMMAND ${en_mk} ${en_list} ${en_in} > ${en_out}
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
		COMMENT "Generating version.c from current WT state."
	)
else()
	# Is there a prebuilt one to use?
	if(EXISTS ${GENSRCDIR}/version.c)
		# Yep, just use it
		add_custom_command(OUTPUT ${version_c}
			DEPENDS ${GENSRCDIR}/version.c
			COMMAND cp ${GENSRCDIR}/version.c ${version_c}
			COMMENT "Using pregenerated version.c."
		)
	else()
		# Nope
		add_custom_command(OUTPUT ${version_c}
			DEPENDS ${version_c_in}
			COMMAND sed -e 's/%%REVISION%%/NULL/' < ${version_c_in} > ${version_c}
			COMMENT "Using null version.c."
		)
	endif(EXISTS ${GENSRCDIR}/version.c)
endif(IS_BZR_CO AND HAS_BZR)
