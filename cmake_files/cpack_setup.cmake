#
# Setup cpack stuff for packaging
#

# Basic stuff
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A window manager for the X Window System.")
set(CPACK_PACKAGE_VENDOR "ctwm")
set(CPACK_PACKAGE_CONTACT "ctwm@ctwm.org")
set(CPACK_PACKAGE_RELOCATABLE OFF)

# Per-packaging-system stuff
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "User Interface/X")
set(CPACK_RPM_PACKAGE_REQUIRES "m4")

include(CPack)
