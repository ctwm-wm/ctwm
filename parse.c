/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/*
 *  [ ctwm ]
 *
 *  Copyright 1992 Claude Lecommandeur.
 *
 * Permission to use, copy, modify  and distribute this software  [ctwm] and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above  copyright notice appear  in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of  Claude Lecommandeur not be used in adverti-
 * sing or  publicity  pertaining to  distribution of  the software  without
 * specific, written prior permission. Claude Lecommandeur make no represen-
 * tations  about the suitability  of this software  for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Claude Lecommandeur DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL  Claude Lecommandeur  BE LIABLE FOR ANY SPECIAL,  INDIRECT OR
 * CONSEQUENTIAL  DAMAGES OR ANY  DAMAGES WHATSOEVER  RESULTING FROM LOSS OF
 * USE, DATA  OR PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR
 * OTHER  TORTIOUS ACTION,  ARISING OUT OF OR IN  CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Claude Lecommandeur [ lecom@sic.epfl.ch ][ April 1992 ]
 */


/***********************************************************************
 *
 * $XConsortium: parse.c,v 1.52 91/07/12 09:59:37 dave Exp $
 *
 * parse the .twmrc file
 *
 * 17-Nov-87 Thomas E. LaStrange       File created
 * 10-Oct-90 David M. Sternlicht       Storing saved colors on root
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 ***********************************************************************/

#include "ctwm.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/Xatom.h>

#include "ctwm_atoms.h"
#include "screen.h"
#include "menus.h"
#include "util.h"
#include "parse.h"
#include "parse_int.h"
#include "deftwmrc.h"
#ifdef SOUNDS
#  include "sound.h"
#endif

#ifndef SYSTEM_INIT_FILE
#define SYSTEM_INIT_FILE "/usr/lib/X11/twm/system.twmrc"
#endif
#define BUF_LEN 300

static int ParseStringList(const char **sl);

/*
 * With current bison, this is defined in the gram.tab.h, so this causes
 * a warning for redundant declaration.  With older bisons and byacc,
 * it's not, so taking it out causes a warning for implicit declaration.
 * A little looking around doesn't show any handy #define's we could use
 * to be sure of the difference.  This should quiet it down on gcc/clang
 * anyway...
 */
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wredundant-decls"
extern int yyparse(void);
# pragma GCC diagnostic pop
#else
extern int yyparse(void);
#endif

static FILE *twmrc;
static int ptr = 0;
static int len = 0;
static char buff[BUF_LEN + 1];
static const char **stringListSource, *currentString;

/*
 * While there are (were) referenced in a number of places through the
 * file, overflowlen is initialized to 0, only possibly changed in
 * twmUnput(), and unless it's non-zero, neither is otherwise touched.
 * So this is purely a twmUnput()-related var, and with flex, never used
 * for anything.
 */
#ifdef NON_FLEX_LEX
static char overflowbuff[20];           /* really only need one */
static int overflowlen;
#endif

int ConstrainedMoveTime = 400;          /* milliseconds, event times */
int ParseError;                         /* error parsing the .twmrc file */
int RaiseDelay = 0;                     /* msec, for AutoRaise */
int twmrc_lineno;
int (*twmInputFunc)(void);              /* used in lexer */

static int twmStringListInput(void);
#ifndef USEM4
static int twmFileInput(void);
#else
static int m4twmFileInput(void);
#endif

#if defined(YYDEBUG) && YYDEBUG
int yydebug = 1;
#endif


/*
 * Backend func that takes an input-providing func and hooks it up to the
 * lex/yacc parser to do the work
 */
static int doparse(int (*ifunc)(void), const char *srctypename,
                   const char *srcname)
{
	ptr = 0;
	len = 0;
	twmrc_lineno = 0;
	ParseError = FALSE;
	twmInputFunc = ifunc;
#ifdef NON_FLEX_LEX
	overflowlen = 0;
#endif

	yyparse();

	if(ParseError) {
		fprintf(stderr, "%s:  errors found in twm %s",
		        ProgramName, srctypename);
		if(srcname) {
			fprintf(stderr, " \"%s\"", srcname);
		}
		fprintf(stderr, "\n");
	}
	return (ParseError ? 0 : 1);
}


/*
 * Principle entry point from top-level code to parse the config file
 */
int ParseTwmrc(char *filename)
{
	int i;
	char *home = NULL;
	int homelen = 0;
	char *cp = NULL;
	char tmpfilename[257];

	/*
	 * Check for the twmrc file in the following order:
	 *   0.  -f filename.#
	 *   1.  -f filename
	 *   2.  .ctwmrc.#
	 *   3.  .ctwmrc
	 *   4.  .twmrc.#
	 *   5.  .twmrc
	 *   6.  system.ctwmrc
	 */
	for(twmrc = NULL, i = 0; !twmrc && i < 7; i++) {
		switch(i) {
			case 0:                       /* -f filename.# */
				if(filename) {
					cp = tmpfilename;
					(void) sprintf(tmpfilename, "%s.%d", filename, Scr->screen);
				}
				else {
					cp = filename;
				}
				break;

			case 1:                       /* -f filename */
				cp = filename;
				break;

			case 2:                       /* ~/.ctwmrc.screennum */
				if(!filename) {
					home = getenv("HOME");
					if(home) {
						homelen = strlen(home);
						cp = tmpfilename;
						(void) sprintf(tmpfilename, "%s/.ctwmrc.%d",
						               home, Scr->screen);
						break;
					}
				}
				continue;

			case 3:                       /* ~/.ctwmrc */
				if(home) {
					tmpfilename[homelen + 8] = '\0';
				}
				break;

			case 4:                       /* ~/.twmrc.screennum */
				if(!filename) {
					home = getenv("HOME");
					if(home) {
						homelen = strlen(home);
						cp = tmpfilename;
						(void) sprintf(tmpfilename, "%s/.twmrc.%d",
						               home, Scr->screen);
						break;
					}
				}
				continue;

			case 5:                       /* ~/.twmrc */
				if(home) {
					tmpfilename[homelen + 7] = '\0'; /* C.L. */
				}
				break;

			case 6:                       /* system.twmrc */
				cp = SYSTEM_INIT_FILE;
				break;
		}

		if(cp) {
			twmrc = fopen(cp, "r");
		}
	}

	if(twmrc) {
		int status;
#ifdef USEM4
		FILE *raw = NULL;
#endif

		/*
		 * If we wound up opening a config file that wasn't the filename
		 * we were passed, make sure the user knows about it.
		 */
		if(filename && strncmp(cp, filename, strlen(filename)) != 0) {
			fprintf(stderr,
			        "%s:  unable to open twmrc file %s, using %s instead\n",
			        ProgramName, filename, cp);
		}


		/*
		 * Kick off the parsing, however we do it.
		 */
#ifdef USEM4
		if(CLarg.GoThroughM4) {
			/*
			 * Hold onto raw filehandle so we can fclose() it below, and
			 * swap twmrc over to the output from m4
			 */
			raw = twmrc;
			twmrc = start_m4(raw);
		}
		status = doparse(m4twmFileInput, "file", cp);
		wait(0);
		fclose(twmrc);
		if(raw) {
			fclose(raw);
		}
#else
		status = doparse(twmFileInput, "file", cp);
		fclose(twmrc);
#endif

		/* And we're done */
		return status;
	}
	else {
		/*
		 * Couldn't find anything to open, fall back to our builtin
		 * config.
		 */
		if(filename) {
			fprintf(stderr,
			        "%s:  unable to open twmrc file %s, using built-in defaults instead\n",
			        ProgramName, filename);
		}
		return ParseStringList(defTwmrc);
	}

	/* NOTREACHED */
}

static int ParseStringList(const char **sl)
{
	stringListSource = sl;
	currentString = *sl;
	return doparse(twmStringListInput, "string list", (char *)NULL);
}


/*
 * Various input routines for the lexer for the various sources of
 * config.
 */

#ifndef USEM4
#include <ctype.h>

/* This has Tom's include() funtionality.  This is utterly useless if you
 * can use m4 for the same thing.               Chris P. Ross */

#define MAX_INCLUDES 10

static struct incl {
	FILE *fp;
	char *name;
	int lineno;
} rc_includes[MAX_INCLUDES];
static int include_file = 0;


static int twmFileInput(void)
{
#ifdef NON_FLEX_LEX
	if(overflowlen) {
		return (int) overflowbuff[--overflowlen];
	}
#endif

	while(ptr == len) {
		while(include_file) {
			if(fgets(buff, BUF_LEN, rc_includes[include_file].fp) == NULL) {
				free(rc_includes[include_file].name);
				fclose(rc_includes[include_file].fp);
				twmrc_lineno = rc_includes[include_file--].lineno;
			}
			else {
				break;
			}
		}

		if(!include_file)
			if(fgets(buff, BUF_LEN, twmrc) == NULL) {
				return 0;
			}
		twmrc_lineno++;

		if(strncmp(buff, "include", 7) == 0) {
			/* Whoops, an include file! */
			char *p = buff + 7, *q;
			FILE *fp;

			while(isspace(*p)) {
				p++;
			}
			for(q = p; *q && !isspace(*q); q++) {
				continue;
			}
			*q = 0;

			if((fp = fopen(p, "r")) == NULL) {
				fprintf(stderr, "%s: Unable to open included init file %s\n",
				        ProgramName, p);
				continue;
			}
			if(++include_file >= MAX_INCLUDES) {
				fprintf(stderr, "%s: init file includes nested too deep\n",
				        ProgramName);
				continue;
			}
			rc_includes[include_file].fp = fp;
			rc_includes[include_file].lineno = twmrc_lineno;
			twmrc_lineno = 0;
			rc_includes[include_file].name = malloc(strlen(p) + 1);
			strcpy(rc_includes[include_file].name, p);
			continue;
		}
		ptr = 0;
		len = strlen(buff);
	}
	return ((int)buff[ptr++]);
}
#else /* USEM4 */
/* If you're going to use m4, use this version instead.  Much simpler.
 * m4 ism's credit to Josh Osborne (stripes) */

static int m4twmFileInput(void)
{
	int line;
	static FILE *cp = NULL;

	if(cp == NULL && CLarg.keepM4_filename) {
		cp = fopen(CLarg.keepM4_filename, "w");
		if(cp == NULL) {
			fprintf(stderr,
			        "%s:  unable to create m4 output %s, ignoring\n",
			        ProgramName, CLarg.keepM4_filename);
			CLarg.keepM4_filename = NULL;
		}
	}

#ifdef NON_FLEX_LEX
	if(overflowlen) {
		return((int) overflowbuff[--overflowlen]);
	}
#endif

	while(ptr == len) {
nextline:
		if(fgets(buff, BUF_LEN, twmrc) == NULL) {
			if(cp) {
				fclose(cp);
			}
			return(0);
		}
		if(cp) {
			fputs(buff, cp);
		}

		if(sscanf(buff, "#line %d", &line)) {
			twmrc_lineno = line - 1;
			goto nextline;
		}
		else {
			twmrc_lineno++;
		}

		ptr = 0;
		len = strlen(buff);
	}
	return ((int)buff[ptr++]);
}
#endif /* USEM4 */


static int twmStringListInput(void)
{
#ifdef NON_FLEX_LEX
	if(overflowlen) {
		return (int) overflowbuff[--overflowlen];
	}
#endif

	/*
	 * return the character currently pointed to
	 */
	if(currentString) {
		unsigned int c = (unsigned int) * currentString++;

		if(c) {
			return c;        /* if non-nul char */
		}
		currentString = *++stringListSource;  /* advance to next bol */
		return '\n';                    /* but say that we hit last eol */
	}
	return 0;                           /* eof */
}


#ifdef NON_FLEX_LEX

/***********************************************************************
 *
 *  Procedure:
 *      twmUnput - redefinition of the lex unput routine
 *
 *  Inputs:
 *      c       - the character to push back onto the input stream
 *
 ***********************************************************************
 */

/* Only used with AT&T lex */
void twmUnput(int c)
{
	if(overflowlen < sizeof overflowbuff) {
		overflowbuff[overflowlen++] = (char) c;
	}
	else {
		twmrc_error_prefix();
		fprintf(stderr, "unable to unput character (%c)\n",
		        c);
	}
}


/***********************************************************************
 *
 *  Procedure:
 *      TwmOutput - redefinition of the lex output routine
 *
 *  Inputs:
 *      c       - the character to print
 *
 ***********************************************************************
 */

/* Only used with AT&T lex */
void TwmOutput(int c)
{
	putchar(c);
}

#endif /* NON_FLEX_LEX */
