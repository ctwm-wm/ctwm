#
# Build env checks
#


# See if we're building from a bzr checkout.  This is fragile in the
# sense that it'll break if the bzr WT format changes, but that's
# staggeringly unlikely now, so...
set(BZR_DIRSTATE_FILE ${CMAKE_SOURCE_DIR}/.bzr/checkout/dirstate)
if(EXISTS ${BZR_DIRSTATE_FILE})
	set(IS_BZR_CO 1)
else()
	set(IS_BZR_CO 0)
endif()


# If we are, see if we can find bzr(1) installed
set(HAS_BZR 0)
if(IS_BZR_CO)
	find_program(BZR_CMD bzr)
	if(BZR_CMD)
		set(HAS_BZR 1)
		message(STATUS "Building from a checkout and found bzr.")
	else()
		message(STATUS "Building from a checkout, but no bzr found.")
	endif(BZR_CMD)
else()
	message(STATUS "You aren't building from a bzr checkout.")
endif(IS_BZR_CO)
