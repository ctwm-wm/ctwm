#include <stdio.h>
#include <stdlib.h>

#include "parse_menu.h"
#include "functions_defs.h"
#include "gram.tab.h"
#include "parse_yacc.h"

#ifndef YYEOF
# define YYEOF 0
#endif

extern int (*twmInputFunc)(void);
extern int yylex(void);
extern int yylex_destroy(void);
extern char *yytext;

static bool
ParseMenuEntry(MenuRoot *menu)
{
	/* grammar:
	     action:       FKEYWORD
	                 | FSKEYWORD STRING
	     menu_entry:   STRING action
	                 | STRING LP STRING COLON STRING RP action
	*/
#define STEP_START        0
#define STEP_LABEL_OK     1 // label OK
#define STEP_LP_OK        2 // "(" OK
#define STEP_FORE_OK      3 // foreground color OK
#define STEP_COLON_OK     4 // ":" OK
#define STEP_BACK_OK      5 // background color OK
#define STEP_RP_OK        6 // ")" OK
#define STEP_FSKEYWORD_OK 7 // action FSKEYWORD OK
#define STEP_DONE         8 // menu entry completed
#define STEP_ERROR        9 // an error occurred
	int step = STEP_START;

	char *str, *label = NULL, *fore = NULL, *back = NULL, *action = NULL;
	MenuRoot *pullm = NULL;
	int func;

	while(step != STEP_DONE && step != STEP_ERROR) {
		int token = yylex();
		switch(token) {
			case YYEOF:
				if(step == STEP_START) {
					return 0;
				}
				fprintf(stderr, "Unterminated menu entry (step %d)\n", step);
				step = STEP_ERROR;
				break;

			case STRING:
				str = strdup(yylval.ptr);
				RemoveDQuote(str);
				switch(step) {
					case STEP_START:
						label = str;
						step = STEP_LABEL_OK;
						break;
					case STEP_LP_OK:
						fore = str;
						step = STEP_FORE_OK;
						break;
					case STEP_COLON_OK:
						back = str;
						step = STEP_BACK_OK;
						break;
					case STEP_FSKEYWORD_OK:
						action = str;
						step = STEP_DONE;
						switch(func) {
							case F_MENU:
							case F_DYNMENU:
								pullm = GetRoot(action, NULL, NULL, func == F_DYNMENU);
								pullm->prev = menu;
								// If the menu has just been created (so its name ==
								// 1st GetRoot param), dissociate its name from
								// action, as action will be freed once the current
								// menu will be destroyed
								if(pullm->name == action) {
									pullm->name = strdup(action);
								}
								break;
							case F_WARPRING:
								if(!CheckWarpRingArg(action)) {
									fprintf(stderr, "ignoring invalid f.warptoring argument \"%s\"\n", action);
									free(action);
									action = NULL;
									func = F_NOP;
								}
								break;
							case F_WARPTOSCREEN:
								if(!CheckWarpScreenArg(action)) {
									fprintf(stderr, "ignoring invalid f.warptoscreen argument \"%s\"\n", action);
									free(action);
									action = NULL;
									func = F_NOP;
								}
								break;
							case F_COLORMAP:
								if(!CheckColormapArg(action)) {
									fprintf(stderr, "ignoring invalid f.colormap argument \"%s\"\n", action);
									free(action);
									action = NULL;
									func = F_NOP;
								}
								break;
						}
						break;
					default:
						free(str);
						fprintf(stderr, "Unexpected menuentry string \"%s\" (step %d)\n", yylval.ptr,
						        step);
						step = STEP_ERROR;
						break;
				}
				break;

			case FKEYWORD:
				switch(step) {
					case STEP_LABEL_OK:
					case STEP_RP_OK:
						func = yylval.num;
						step = STEP_DONE;
						break;
					default:
						fprintf(stderr, "Unexpected menuentry keyword %s (step %d)\n", yytext, step);
						step = STEP_ERROR;
						break;
				}
				break;

			case FSKEYWORD:
				switch(step) {
					case STEP_LABEL_OK:
					case STEP_RP_OK:
						func = yylval.num;
						step = STEP_FSKEYWORD_OK;
						break;
					default:
						fprintf(stderr, "Unexpected menuentry keyword %s (step %d)\n", yytext, step);
						step = STEP_ERROR;
						break;
				}
				break;

			case LP:
				if(step == STEP_LABEL_OK) {
					step = STEP_LP_OK;
					break;
				}
				fprintf(stderr, "Unexpected menuentry '(' (step %d)\n", step);
				step = STEP_ERROR;
				break;

			case COLON:
				if(step == STEP_FORE_OK) {
					step = STEP_COLON_OK;
					break;
				}
				fprintf(stderr, "Unexpected menuentry ':' (step %d)\n", step);
				step = STEP_ERROR;
				break;

			case RP:
				if(step == STEP_BACK_OK) {
					step = STEP_RP_OK;
					break;
				}
				fprintf(stderr, "Unexpected menuentry ')' (step %d)\n", step);
				step = STEP_ERROR;
				break;

			default:
				fprintf(stderr, "Unexpected menu entry token: \"%s\" / %d (step %d)\n", yytext,
				        token, step);
				step = STEP_ERROR;
				break;
		}
	}

	if(step == STEP_ERROR) {
		if(label != NULL) {
			free(label);
		}
		if(action != NULL) {
			free(action);
		}
		if(fore != NULL) {
			free(fore);
		}
		if(back != NULL) {
			free(back);
		}
		return 0;
	}

	if(func == F_SEPARATOR) {
		if(menu->last != NULL) {
			menu->last->separated = true;
		}
	}
	else {
		AddToMenu(menu, label, action, pullm, func, fore, back);
	}
	if(fore != NULL) {
		free(fore);
		free(back);
	}
	return 1;
}


static char *currentString;
static int stringInput(void)
{
	if(currentString) {
		return (unsigned int) * currentString++;
	}
	return 0;
}


void
ParseMenu(MenuRoot *menu, char *src)
{
	currentString = src;
	twmInputFunc = stringInput;

	while(ParseMenuEntry(menu))
		;

	yylex_destroy();
	currentString = NULL;
}
