/*
 * Window decoration routines -- initializtion time
 *
 * These are funcs that are called during ctwm initialization to setup
 * bits based on general X stuff and/or config file bits.
 */


#include "ctwm.h"

#include <stdio.h>

#include "add_window.h"
#include "decorations.h"
#include "image.h"
#include "parse.h"
#include "screen.h"

#include "decorations_init.h"


/*
 * Global marker used in config file loading to track "which one we're
 * currently messing with"
 */
static TitleButton *cur_tb = NULL;


/*
 * InitTitlebarButtons - Do all the necessary stuff to load in a titlebar
 * button.  If we can't find the button, then put in a question; if we can't
 * find the question mark, something is wrong and we are probably going to be
 * in trouble later on.
 */
void
InitTitlebarButtons(void)
{
	TitleButton *tb;
	int h;

	/*
	 * initialize dimensions
	 */
	Scr->TBInfo.width = (Scr->TitleHeight -
	                     2 * (Scr->FramePadding + Scr->ButtonIndent));
	if(Scr->use3Dtitles)
		Scr->TBInfo.pad = ((Scr->TitlePadding > 1)
		                   ? ((Scr->TitlePadding + 1) / 2) : 0);
	else
		Scr->TBInfo.pad = ((Scr->TitlePadding > 1)
		                   ? ((Scr->TitlePadding + 1) / 2) : 1);
	h = Scr->TBInfo.width - 2 * Scr->TBInfo.border;

	/*
	 * add in some useful buttons and bindings so that novices can still
	 * use the system.
	 */
	if(!Scr->NoDefaults) {
		/* insert extra buttons */
		if(Scr->use3Dtitles) {
			if(!CreateTitleButton(TBPM_3DDOT, F_ICONIFY, "", (MenuRoot *) NULL,
			                      False, False)) {
				fprintf(stderr, "%s:  unable to add iconify button\n", ProgramName);
			}
			if(!CreateTitleButton(TBPM_3DRESIZE, F_RESIZE, "", (MenuRoot *) NULL,
			                      True, True)) {
				fprintf(stderr, "%s:  unable to add resize button\n", ProgramName);
			}
		}
		else {
			if(!CreateTitleButton(TBPM_ICONIFY, F_ICONIFY, "", (MenuRoot *) NULL,
			                      False, False)) {
				fprintf(stderr, "%s:  unable to add iconify button\n", ProgramName);
			}
			if(!CreateTitleButton(TBPM_RESIZE, F_RESIZE, "", (MenuRoot *) NULL,
			                      True, True)) {
				fprintf(stderr, "%s:  unable to add resize button\n", ProgramName);
			}
		}
		AddDefaultFuncButtons();
	}
	ComputeCommonTitleOffsets();

	/*
	 * load in images and do appropriate centering
	 */

	for(tb = Scr->TBInfo.head; tb; tb = tb->next) {
		tb->image = GetImage(tb->name, Scr->TitleC);
		if(!tb->image) {
			tb->image = GetImage(TBPM_QUESTION, Scr->TitleC);
			if(!tb->image) {
				/*
				 * (sorta) Can't Happen.  Calls a static function that
				 * builds from static data, so could only possibly fail
				 * if XCreateBitmapFromData() failed (which should be
				 * vanishingly rare; memory allocation failures etc).
				 */
				fprintf(stderr, "%s:  unable to add titlebar button \"%s\"\n",
				        ProgramName, tb->name);
				continue;
			}
		}
		tb->width  = tb->image->width;
		tb->height = tb->image->height;
		tb->dstx = (h - tb->width + 1) / 2;
		if(tb->dstx < 0) {              /* clip to minimize copying */
			tb->srcx = -(tb->dstx);
			tb->width = h;
			tb->dstx = 0;
		}
		else {
			tb->srcx = 0;
		}
		tb->dsty = (h - tb->height + 1) / 2;
		if(tb->dsty < 0) {
			tb->srcy = -(tb->dsty);
			tb->height = h;
			tb->dsty = 0;
		}
		else {
			tb->srcy = 0;
		}
	}
}



/*
 * Sets the action for a given {mouse button,set of modifier keys} on the
 * "current" button.  This happens during initialization, in a few
 * different ways.
 *
 * CreateTitleButton() winds up creating a new button, and setting the
 * cur_tb global we rely on.  It calls us then to initialize our action
 * to what it was told (hardcoded for the !NoDefaults case in
 * InitTitlebarButtons() for fallback config, from the config file when
 * it's called via GotTitleButton() for the one-line string form of
 * *TitleButton spec).
 *
 * It's also called directly from the config parsing for the block-form
 * *TitleButton specs, when the cur_tb was previously set by
 * CreateTitleButton() at the opening of the block.
 */
void
SetCurrentTBAction(int button, int nmods, int func, char *action,
                   MenuRoot *menuroot)
{
	TitleButtonFunc *tbf;

	if(!cur_tb) {
		fprintf(stderr, "%s: can't find titlebutton\n", ProgramName);
		return;
	}
	for(tbf = cur_tb->funs; tbf; tbf = tbf->next) {
		if(tbf->num == button && tbf->mods == nmods) {
			break;
		}
	}
	if(!tbf) {
		tbf = malloc(sizeof(TitleButtonFunc));
		if(!tbf) {
			fprintf(stderr, "%s: out of memory\n", ProgramName);
			return;
		}
		tbf->next = cur_tb->funs;
		cur_tb->funs = tbf;
	}
	tbf->num = button;
	tbf->mods = nmods;
	tbf->func = func;
	tbf->action = action;
	tbf->menuroot = menuroot;
}



int
CreateTitleButton(char *name, int func, char *action, MenuRoot *menuroot,
                  Bool rightside, Bool append)
{
	int button;
	cur_tb = calloc(1, sizeof(TitleButton));

	if(!cur_tb) {
		fprintf(stderr,
		        "%s:  unable to allocate %lu bytes for title button\n",
		        ProgramName, (unsigned long) sizeof(TitleButton));
		return 0;
	}

	cur_tb->name = name;           /* note that we are not copying */
	cur_tb->rightside = rightside;
	if(rightside) {
		Scr->TBInfo.nright++;
	}
	else {
		Scr->TBInfo.nleft++;
	}

	for(button = 0; button < MAX_BUTTONS; button++) {
		SetCurrentTBAction(button + 1, 0, func, action, menuroot);
	}

	/*
	 * Cases for list:
	 *
	 *     1.  empty list, prepend left       put at head of list
	 *     2.  append left, prepend right     put in between left and right
	 *     3.  append right                   put at tail of list
	 *
	 * Do not refer to widths and heights yet since buttons not created
	 * (since fonts not loaded and heights not known).
	 */
	if((!Scr->TBInfo.head) || ((!append) && (!rightside))) {    /* 1 */
		cur_tb->next = Scr->TBInfo.head;
		Scr->TBInfo.head = cur_tb;
	}
	else if(append && rightside) {      /* 3 */
		TitleButton *t;
		for /* SUPPRESS 530 */
		(t = Scr->TBInfo.head; t->next; t = t->next);
		t->next = cur_tb;
		cur_tb->next = NULL;
	}
	else {                              /* 2 */
		TitleButton *t, *prev = NULL;
		for(t = Scr->TBInfo.head; t && !t->rightside; t = t->next) {
			prev = t;
		}
		if(prev) {
			cur_tb->next = prev->next;
			prev->next = cur_tb;
		}
		else {
			cur_tb->next = Scr->TBInfo.head;
			Scr->TBInfo.head = cur_tb;
		}
	}

	return 1;
}
