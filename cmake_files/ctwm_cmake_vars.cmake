#
# Setup some vars we use in the configure/build process
#

# The dir in which we ship pregen'd source files
set(GENSRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/gen)

# Where our doc files are (and where pregen'd docs might be)
set(SRCDOCDIR ${CMAKE_SOURCE_DIR}/doc)


# Our base set of sources
set(CTWMSRC
	add_window.c
	animate.c
	clargs.c
	clicktofocus.c
	ctopts.c
	ctwm.c
	ctwm_atoms.c
	cursor.c
	decorations.c
	deftwmrc.c
	event_names.c
	events.c
	functions.c
	gc.c
	gram.tab.c
	iconmgr.c
	icons.c
	image.c
	image_bitmap.c
	image_bitmap_builtin.c
	image_xwd.c
	lex.c
	list.c
	mask_screen.c
	menus.c
	mwmhints.c
	otp.c
	parse.c
	parse_be.c
	parse_yacc.c
	resize.c
	session.c
	util.c
	version.c
	vscreen.c
	windowbox.c
	workmgr.c

	ext/repl_str.c
)


# Libs to link in (init empty list)
set(CTWMLIBS)


# Our normal set of warning flags
set(STD_WARNS
	-Wall
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef
	-Wredundant-decls -Wcast-align -Wcast-qual -Wchar-subscripts
	-Winline -Wnested-externs -Wmissing-declarations
)
