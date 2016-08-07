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
void GotoWorkSpaceByName(VirtualScreen *vs, char *wname);
void GotoWorkSpaceByNumber(VirtualScreen *vs, int workspacenum);
void GotoPrevWorkSpace(VirtualScreen *vs);
void GotoNextWorkSpace(VirtualScreen *vs);
void GotoRightWorkSpace(VirtualScreen *vs);
void GotoLeftWorkSpace(VirtualScreen *vs);
void GotoUpWorkSpace(VirtualScreen *vs);
void GotoDownWorkSpace(VirtualScreen *vs);
void GotoWorkSpace(VirtualScreen *vs, WorkSpace *ws);
void WMgrHandleExposeEvent(VirtualScreen *vs, XEvent *event);
void PaintWorkSpaceManager(VirtualScreen *vs);
void WMapToggleState(VirtualScreen *vs);
void WMapSetMapState(VirtualScreen *vs);
void WMapSetButtonsState(VirtualScreen *vs);
bool WMapWindowMayBeAdded(TwmWindow *win);
void WMapAddWindow(TwmWindow *win);
void WMapDestroyWindow(TwmWindow *win);
void WMapMapWindow(TwmWindow *win);
void WMapSetupWindow(TwmWindow *win, int x, int y, int w, int h);
void WMapIconify(TwmWindow *win);
void WMapDeIconify(TwmWindow *win);
void WMapRaiseLower(TwmWindow *win);
void WMapLower(TwmWindow *win);
void WMapRaise(TwmWindow *win);
void WMapRestack(WorkSpace *ws);
void WMapUpdateIconName(TwmWindow *win);
void WMgrHandleKeyReleaseEvent(VirtualScreen *vs, XEvent *event);
void WMgrHandleKeyPressEvent(VirtualScreen *vs, XEvent *event);
void WMgrHandleButtonEvent(VirtualScreen *vs, XEvent *event);
void WMapRedrawName(VirtualScreen *vs, WinList   wl);
void WMapAddToList(TwmWindow *win, WorkSpace *ws);
void WMapRemoveFromList(TwmWindow *win, WorkSpace *ws);
char *GetCurrentWorkSpaceName(VirtualScreen *vs);
WorkSpace *GetWorkspace(char *wname);

void ShowBackground(VirtualScreen *vs, int state);

void ws_set_useBackgroundInfo(bool newval);

#endif /* _CTWM_WORKMGR_H */
