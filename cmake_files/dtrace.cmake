# Try setting up some bits for dtrace.
#
# By itself, dtrace can trace things like function entry/return points
# just fine, and even pull numeric and string arguments.  More involved
# data structures, though, would either need manual definition in the D
# scripts (which is practically impossible for anything sizable), manual
# specification of offsets (even worse), or CTF info included in the
# binary (hey, we can do that!).  So, see if we can pull that stuff in...

find_program(CTFCONVERT ctfconvert)
find_program(CTFMERGE   ctfmerge)

if(CTFCONVERT AND CTFMERGE)
	message(STATUS "Found ctfconvert/ctfmerge, setting up CTF info for dtrace.")

	# ctfconvert/merge is about pulling over debug info, so make sure we
	# enable that in the objects.
	add_definitions("-g")

	# This is a horrific hack.  cmake provides no way to actually find
	# out the list of object files, or where they are, because that would
	# be too easy.  So we have to "know", and take our best shot.  Sigh.
	# Well, it's really only a dev tool anyway, so I guess some manual
	# mess isn't the end of the world.  We can't check the existence yet
	# here, since it hasn't been created at this point in the process.
	# So we just have to hope.  mk_ctf_info.sh will warn us if things
	# change...
	set(CODIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/ctwm.dir)

	add_custom_command(TARGET ctwm POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E env
			CTFCONVERT=${CTFCONVERT} CTFMERGE=${CTFMERGE}
			${TOOLS}/mk_ctf_info.sh ${CODIR}
			${CMAKE_CURRENT_BINARY_DIR}/ctwm
		COMMENT "Converting in CTF info for dtrace"
	)
endif()
