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

#define MAXWORKSPACE 32

typedef enum {
	WMS_map,
	WMS_buttons,
} WMgrState;

typedef enum {
	STYLE_NORMAL,
	STYLE_STYLE1,
	STYLE_STYLE2,
	STYLE_STYLE3,
} ButtonStyle;

struct winList {
	struct WorkSpace    *wlist;
	Window              w;
	int                 x, y;
	int                 width, height;
	TwmWindow           *twm_win;
	ColorPair           cp;
	MyFont              font;
	struct winList      *next;
};

struct WorkSpaceMgr {
	struct WorkSpace       *workSpaceList;
	struct WorkSpaceWindow *workSpaceWindowList;
	struct OccupyWindow    *occupyWindow;
	MyFont          buttonFont;
	MyFont          windowFont;
	ColorPair       windowcp;
	bool            windowcpgiven;
	ColorPair       cp;
	long            count;
	char            *geometry;
	int             lines, columns;
	bool            noshowoccupyall;
	int             initialstate;
	ButtonStyle     buttonStyle;
	name_list       *windowBackgroundL;
	name_list       *windowForegroundL;
	/* The fields below have been moved from WorkSpaceWindow */
	ColorPair           curColors;
	Image               *curImage;
	unsigned long       curBorderColor;
	bool                curPaint;

	ColorPair           defColors;
	Image              *defImage;
	unsigned long       defBorderColor;
	int                 hspace, vspace;
	char               *name;
	char               *icon_name;
};

struct WorkSpace {
	int                 number;
	char                *name;
	char                *label;
	Image               *image;
	name_list           *clientlist;
	IconMgr             *iconmgr;
	ColorPair           cp;
	ColorPair           backcp;
	TwmWindow           *save_focus;  /* Used by SaveWorkspaceFocus feature */
	struct WindowRegion *FirstWindowRegion;
	struct WorkSpace *next;
};

struct MapSubwindow {
	Window  w;
	int     x, y;
	WinList wl;
};

struct ButtonSubwindow {
	Window w;
};

struct WorkSpaceWindow {                /* There is one per virtual screen */
	VirtualScreen   *vs;
	Window          w;
	TwmWindow       *twm_win;
	MapSubwindow    **mswl;               /* MapSubWindow List */
	ButtonSubwindow **bswl;               /* ButtonSubwindow List */
	WorkSpace       *currentwspc;

	int           state;

	int           width, height;
	int           bwidth, bheight;
	int           wwidth, wheight;
};


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
void AddWorkSpace(const char *name,
                  const char *background, const char *foreground,
                  const char *backback,   const char *backfore,
                  const char *backpix);
void Vanish(VirtualScreen *vs, TwmWindow *tmp_win);
void DisplayWin(VirtualScreen *vs, TwmWindow *tmp_win);
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
void WMapCreateCurrentBackGround(char *border,
                                 char *background, char *foreground,
                                 char *pixmap);
void WMapCreateDefaultBackGround(char *border,
                                 char *background, char *foreground,
                                 char *pixmap);
char *GetCurrentWorkSpaceName(VirtualScreen *vs);
WorkSpace *GetWorkspace(char *wname);
void ReparentFrameAndIcon(TwmWindow *tmp_win);

void ShowBackground(VirtualScreen *vs, int state);

#endif /* _CTWM_WORKMGR_H */
