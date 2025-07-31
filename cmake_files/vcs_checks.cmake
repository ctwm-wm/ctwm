#
# See what sort of version control bits we might have around.
#


# See if we're building in a git repo.  This is the presumed current
# world.
#
# Assume .git/ stays stable in format so this works...
set(GIT_INDEX_FILE ${CMAKE_SOURCE_DIR}/.git/index)
set(IS_GIT_CO 0)
if(EXISTS ${GIT_INDEX_FILE})
	set(IS_GIT_CO 1)
endif()

# See if we're building from a bzr checkout.
#
# This is fragile in the sense that it'll break if the bzr WT format
# changes, but that's staggeringly unlikely now, so...
set(BZR_DIRSTATE_FILE ${CMAKE_SOURCE_DIR}/.bzr/checkout/dirstate)
if(EXISTS ${BZR_DIRSTATE_FILE})
	set(IS_BZR_CO 1)
else()
	set(IS_BZR_CO 0)
endif()



# OK, did we get any VCSy stuff?
set(DID_VCS 0)


# Are we git?  Be gittish.
set(HAS_GIT 0)
if(NOT DID_VCS AND IS_GIT_CO)
	find_program(GIT_CMD git)
	if(GIT_CMD)
		set(HAS_GIT 1)
		set(DID_VCS 1)
		message(STATUS "Building from git repo and found git (${GIT_CMD}).")
	else()
		message(STATUS "Building from git repo, but no git found.")
	endif(GIT_CMD)
endif() # git


# Do we fallback to bzr?
set(HAS_BZR 0)
if(NOT DID_VCS AND IS_BZR_CO)
	find_program(BZR_CMD NAMES bzr brz)
	if(BZR_CMD)
		set(HAS_BZR 1)
		set(DID_VCS 1)
		message(STATUS "Building from a checkout and found bzr (${BZR_CMD}).")
	else()
		message(STATUS "Building from a checkout, but no bzr found.")
	endif(BZR_CMD)
endif() # bzr


# Flag for dev use
set(VCS_CHECKS_RUN 1)
