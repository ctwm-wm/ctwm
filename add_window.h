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


/**********************************************************************
 *
 * $XConsortium: add_window.h,v 1.7 90/04/17 14:04:33 jim Exp $
 *
 * AddWindow include file
 *
 * 31-Mar-88 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef _CTWM_ADD_WINDOW_H
#define _CTWM_ADD_WINDOW_H

#include "iconmgr.h"

extern char NoName[];
extern bool resizeWhenAdd;

typedef enum {
	AWT_NORMAL,
	AWT_ICON_MANAGER,
	AWT_WINDOWBOX,
	AWT_WORKSPACE_MANAGER,
} AWType;

void GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp);
TwmWindow *AddWindow(Window w, AWType wtype, IconMgr *iconp,
                     VirtualScreen *vs);
TwmWindow *GetTwmWindow(Window w);
bool MappedNotOverride(Window w);
void GrabButtons(TwmWindow *tmp_win);
void GrabKeys(TwmWindow *tmp_win);
void GetWindowSizeHints(TwmWindow *tmp_win);
void CreateWindowRegions(void);
bool PlaceWindowInRegion(TwmWindow *tmp_win,
                         int *final_x, int *final_y);
void RemoveWindowFromRegion(TwmWindow   *tmp_win);
name_list **AddWindowRegion(char *geom, int  grav1, int grav2);
extern int AddingX;
extern int AddingY;
extern unsigned int AddingW;
extern unsigned int AddingH;

void FetchWmColormapWindows(TwmWindow *tmp);
void FetchWmProtocols(TwmWindow *tmp);

TwmColormap *CreateTwmColormap(Colormap c);
ColormapWindow *CreateColormapWindow(Window w,
                                     bool creating_parent,
                                     bool property_window);
#endif /* _CTWM_ADD_WINDOW_H */

