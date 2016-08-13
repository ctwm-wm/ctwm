%{
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
 * $XConsortium: lex.l,v 1.62 89/12/10 17:46:33 jim Exp $
 *
 * .twmrc lex file
 *
 * 12-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#include "ctwm.h"
#include "parse.h"
#include "parse_be.h"

#include "gram.tab.h"

/*
 * flex uses a YY_INPUT macro internally rather than input.  It doesn't
 * need unput since it maintains state internally.
 */
#ifdef FLEX_SCANNER
#  undef YY_INPUT
#  define YY_INPUT(buf,result,max_size) {buf[0]=twmInputFunc();result=(buf[0] != 0);}
#endif

/*
 * YY_FLEX_{MAJOR,MINOR}_VERSION was added in 2.5.1 (28Mar95); just in
 * case we need to do version checks of stuff.
 */

/*
 * flex listens to these to avoid declaring these functions, which we
 * don't use, so suppress the warning.
 */
#define YY_NO_UNPUT
#define YY_NO_INPUT

/* From gram.y, for debugging and error reporting */
extern const char *lex_str;

%}

string				\"([^"]|\\.)*\"
number				[0-9]+
%%
"{"				{ return (LB); }
"}"				{ return (RB); }
"("				{ return (LP); }
")"				{ return (RP); }
"="				{ return (EQUALS); }
":"				{ return (COLON); }
"+"				{ return PLUS; }
"-"				{ return MINUS; }
"|"				{ return OR; }

[a-zA-Z\.]+			{ int token = parse_keyword (yytext, 
							     &yylval.num);
				  if (token == ERRORTOKEN) {
				      twmrc_error_prefix();
				      fprintf (stderr,
				       "ignoring unknown keyword:  %s\n", 
					       yytext);
				      ParseError = true;
				  } else 
				    lex_str = yytext;
				    return token;
				}

"!"				{ yylval.num = F_EXEC; return FSKEYWORD; }

{string}			{ yylval.ptr = yytext; return STRING; }
{number}			{ (void)sscanf(yytext, "%d", &yylval.num);
				  lex_str = yytext; return NUMBER; }
\#[^\n]*\n			{;}
[\n\t ]				{;}
.				{
				  twmrc_error_prefix();
				  fprintf (stderr, 
					   "ignoring character \"%s\"\n",
					   yytext);
				  ParseError = true;
				}
%%

/*
 * In flex versions before 2.4.1 (30Nov93), yywrap was a macro, not a
 * function.  There's no way we really support versions that old, but
 * what the heck...
 */
#ifndef yywrap
int yywrap(void) { return(1);}
#endif

/* AT&T lex uses the input/unput macros */
#ifndef FLEX_SCANNER
/*
 * I believe Solaris at least recently recently (and maybe currently)
 * ships with an AT&T lex, but also with flex beside it.  Plan
 * 9 might ship only A&T lex?
 *
 * However, our current build toolchain doesn't look for any lex other
 * than flex.  So #error out if we get here; adventurous users might take
 * this out, and it might work; let us know!
 */
#error Not supported on non-flex; remove this line at your own risk
#ifdef NON_FLEX_LEX
#undef unput
#undef input
#undef output
#undef feof
#define unput(c)	twmUnput(c)
#define input()		(*twmInputFunc)()
#define output(c)	TwmOutput(c)
#define feof()		(1)
#endif /* NON_FLEX_LEX */
#endif /* !FLEX_SCANNER */

