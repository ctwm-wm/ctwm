#
# Find and setup asciidoc[tor] bits
#


# First see if we can find the programs
find_program(ASCIIDOCTOR asciidoctor)
find_program(ASCIIDOC asciidoc)
find_program(A2X a2x)


# If we have asciidoctor, we need to figure out the version, as manpage
# output is relatively new (unreleased, at the time of this writing).
if(ASCIIDOCTOR)
	execute_process(
		COMMAND ${ASCIIDOCTOR} --version
		RESULT_VARIABLE _adoctor_result
		OUTPUT_VARIABLE _adoctor_verout
		ERROR_QUIET
	)
	if(NOT ${_adoctor_result} EQUAL "0")
		# Err...
		message(WARNING "Unexpected result trying asciidoctor --version.")
		set(_adoctor_verout "Asciidoctor 0.0.0 FAKE")
	endif()
	unset(_adoctor_result)

	# Break out the version.
	set(_adoctor_veregex "Asciidoctor ([0-9]+\\.[0-9]+\\.[0-9]+).*")
	string(REGEX REPLACE ${_adoctor_veregex} "\\1"
		ASCIIDOCTOR_VERSION ${_adoctor_verout})
	unset(_adoctor_verout)
	unset(_adoctor_veregex)
	message(STATUS "Found asciidoctor (${ASCIIDOCTOR}) version ${ASCIIDOCTOR_VERSION}")

	# 1.5.3 is the first release that can write manpages natively.  This
	# means 1.5.3 dev versions after a certain point can as well; assume
	# anybody running a 1.5.3 dev is keeping up well enough that it can
	# DTRT too.  We assume any version can do HTML.
	set(ASCIIDOCTOR_CAN_MAN  0)
	set(ASCIIDOCTOR_CAN_HTML 1)
	if(${ASCIIDOCTOR_VERSION} VERSION_GREATER "1.5.2")
		set(ASCIIDOCTOR_CAN_MAN 1)
	elseif(${ASCIIDOCTOR_VERSION} VERSION_LESS "0.0.1")
		set(ASCIIDOCTOR_CAN_HTML 0)
	endif()
endif(ASCIIDOCTOR)


# For asciidoc, it doesn't really matter, but look up the version for
# cosmetics anyway
if(ASCIIDOC)
	execute_process(
		COMMAND ${ASCIIDOC} --version
		RESULT_VARIABLE _adoc_result
		OUTPUT_VARIABLE _adoc_verout
		ERROR_QUIET
	)
	if(NOT ${_adoc_result} EQUAL "0")
		# Err...
		message(WARNING "Unexpected result trying asciidoc --version.")
		set(_adoc_verout "asciidoc 0.0.0")
	endif()
	unset(_adoc_result)

	# Break out the version.
	set(_adoc_veregex "asciidoc ([0-9]+\\.[0-9]+\\.[0-9]+).*")
	string(REGEX REPLACE ${_adoc_veregex} "\\1"
		ASCIIDOC_VERSION ${_adoc_verout})
	unset(_adoc_verout)
	unset(_adoc_veregex)
	message(STATUS "Found asciidoc (${ASCIIDOC}) version ${ASCIIDOC_VERSION}")

	# Can always do both, unless horked
	if(${ASCIIDOC_VERSION} VERSION_GREATER "0.0.0")
		set(ASCIIDOC_CAN_MAN  1)
		set(ASCIIDOC_CAN_HTML 1)
	endif()

	# This is an example of 'horked'...
	if(NOT A2X)
		set(ASCIIDOC_CAN_MAN 0)
	endif()
endif(ASCIIDOC)
