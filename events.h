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
 * $XConsortium: events.h,v 1.14 91/05/10 17:53:58 dave Exp $
 *
 * twm event handler include file
 *
 * 17-Nov-87 Thomas E. LaStrange                File created
 *
 ***********************************************************************/

#ifndef _CTWM_EVENTS_H
#define _CTWM_EVENTS_H

#include <stdio.h>  // For FILE

typedef void (*event_proc)(void);

void InitEvents(void);
extern Time lastTimestamp;
void SimulateMapRequest(Window w);
void AutoRaiseWindow(TwmWindow *tmp);
void SetRaiseWindow(TwmWindow *tmp);
void AutoPopupMaybe(TwmWindow *tmp);
void AutoLowerWindow(TwmWindow *tmp);
#define LastTimestamp() lastTimestamp
Window WindowOfEvent(XEvent *e);
void FixRootEvent(XEvent *e);
bool DispatchEvent(void);
bool DispatchEvent2(void);
void HandleEvents(void) __attribute__((noreturn));
void HandleExpose(void);
void HandleDestroyNotify(void);
void HandleMapRequest(void);
void HandleMapNotify(void);
void HandleUnmapNotify(void);
void HandleMotionNotify(void);
void HandleButtonRelease(void);
void HandleButtonPress(void);
void HandleEnterNotify(void);
void HandleLeaveNotify(void);
void HandleConfigureRequest(void);
void HandleClientMessage(void);
void HandlePropertyNotify(void);
void HandleKeyPress(void);
void HandleKeyRelease(void);
void HandleColormapNotify(void);
void HandleVisibilityNotify(void);
void HandleUnknown(void);
void HandleFocusIn(XFocusInEvent *event);
void HandleFocusOut(XFocusOutEvent *event);
void SynthesiseFocusOut(Window w);
void SynthesiseFocusIn(Window w);
void HandleCirculateNotify(void);
bool Transient(Window w, Window *propw);

ScreenInfo *FindScreenInfo(Window w);

void ConfigureRootWindow(XEvent *ev);

void free_cwins(TwmWindow *tmp);

extern event_proc EventHandler[];
extern Window DragWindow;
extern int origDragX;
extern int origDragY;
extern int DragX;
extern int DragY;
extern unsigned int DragWidth;
extern unsigned int DragHeight;
extern unsigned int DragBW;
extern int CurrentDragX;
extern int CurrentDragY;
extern int Context;
extern FILE *tracefile;

extern int ButtonPressed;
extern bool Cancel;

extern XEvent Event;

#endif /* _CTWM_EVENTS_H */
