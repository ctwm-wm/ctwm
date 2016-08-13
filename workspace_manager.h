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

#ifndef _CTWM_WORKMGR_H
#define _CTWM_WORKMGR_H

void InitWorkSpaceManager(void);
void ConfigureWorkSpaceManager(void);
void CreateWorkSpaceManager(void);
void PaintWorkSpaceManager(VirtualScreen *vs);
void WMgrHandleExposeEvent(VirtualScreen *vs, XEvent *event);

void WMgrToggleState(VirtualScreen *vs);
void WMgrSetMapState(VirtualScreen *vs);
void WMgrSetButtonsState(VirtualScreen *vs);

void WMgrHandleKeyReleaseEvent(VirtualScreen *vs, XEvent *event);
void WMgrHandleKeyPressEvent(VirtualScreen *vs, XEvent *event);
void WMgrHandleButtonEvent(VirtualScreen *vs, XEvent *event);

void WMapMapWindow(TwmWindow *win);
void WMapSetupWindow(TwmWindow *win, int x, int y, int w, int h);
void WMapIconify(TwmWindow *win);
void WMapDeIconify(TwmWindow *win);
void WMapRaiseLower(TwmWindow *win);
void WMapLower(TwmWindow *win);
void WMapRaise(TwmWindow *win);
void WMapRestack(WorkSpace *ws);
void WMapUpdateIconName(TwmWindow *win);
void WMapRedrawName(VirtualScreen *vs, WinList *wl);

void WMapAddWindow(TwmWindow *win);
void WMapAddWindowToWorkspace(TwmWindow *win, WorkSpace *ws);
void WMapRemoveWindow(TwmWindow *win);
void WMapRemoveWindowFromWorkspace(TwmWindow *win, WorkSpace *ws);

bool WMapWindowMayBeAdded(TwmWindow *win);

#endif /* _CTWM_WORKMGR_H */
