#
# Setup some vars we use in the configure/build process
#

# The dir in which we ship pregen'd source files
set(GENSRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/gen)

# Where our doc files are (and where pregen'd docs might be)
set(SRCDOCDIR ${CMAKE_SOURCE_DIR}/doc)


# Our base set of sources
set(CTWMSRC
	# Basic files  ##STDSRC-START
	add_window.c
	animate.c
	captive.c
	clargs.c
	clicktofocus.c
	ctopts.c
	ctwm.c
	cursor.c
	decorations.c
	decorations_init.c
	drawing.c
	event_names.c
	events.c
	functions.c
	gc.c
	iconmgr.c
	icons.c
	icons_builtin.c
	image.c
	image_bitmap.c
	image_bitmap_builtin.c
	image_xwd.c
	list.c
	mask_screen.c
	menus.c
	mwmhints.c
	occupation.c
	otp.c
	parse.c
	parse_be.c
	parse_yacc.c
	resize.c
	session.c
	util.c
	vscreen.c
	windowbox.c
	workspace_config.c
	workspace_manager.c
	workspace_utils.c

	# External libs
	ext/repl_str.c
	##STDSRC-END

	# Generated files  ##GENSRC-START
	ctwm_atoms.c
	deftwmrc.c
	gram.tab.c
	lex.c
	version.c
	##GENSRC-END
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
