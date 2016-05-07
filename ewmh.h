/*
 *  [ ctwm ]
 *
 *  Copyright 2014 Olaf Seibert
 *
 * Permission to use, copy, modify and distribute this software [ctwm]
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Olaf Seibert not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. Olaf Seibert
 * makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied
 * warranty.
 *
 * Olaf Seibert DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL Olaf Seibert BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Olaf Seibert [ rhialto@falu.nl ][ May 2014 ]
 */

#ifndef _CTWM_EWMH_H
#define _CTWM_EWMH_H

#ifdef EWMH

#include "types.h"

/*
 * Switch for experimental code to treat a Desktop window as a Root
 * window for the purposes of key and button bindings.
 * It doesn't work as nicely as I hoped though; maybe I'll get some
 * better idea.
 */
/* #define EWMH_DESKTOP_ROOT */

typedef enum EwmhWindowType {
	wt_Normal,
	wt_Desktop,
	wt_Dock,
} EwmhWindowType;

/*
 * The window is to reserve space at the edge of the screen.
 */
typedef struct EwmhStrut {
	struct EwmhStrut *next;
	struct TwmWindow *win;

	int left;
	int right;
	int top;
	int bottom;
} EwmhStrut;

#define EWMH_HAS_STRUT                  0x0001

#define EWMH_STATE_MAXIMIZED_VERT       0x0010  /* for _NET_WM_STATE */
#define EWMH_STATE_MAXIMIZED_HORZ       0x0020  /* for _NET_WM_STATE */
#define EWMH_STATE_FULLSCREEN           0x0040  /* for _NET_WM_STATE */
#define EWMH_STATE_SHADED               0x0080  /* for _NET_WM_STATE */
#define EWMH_STATE_ABOVE                0x0100  /* for _NET_WM_STATE */
#define EWMH_STATE_BELOW                0x0200  /* for _NET_WM_STATE */
#define EWMH_STATE_ALL                  0xFFF0

/*
 * Priorities of the window types we recognize
 */

#define EWMH_PRI_DESKTOP                -8
#define EWMH_PRI_DOCK                    4
#define EWMH_PRI_FULLSCREEN              6
#define EWMH_PRI_NORMAL                  0
#define EWMH_PRI_ABOVE                   2

extern void EwmhInit(void);
extern int EwmhInitScreenEarly(ScreenInfo *scr);
extern void EwmhInitScreenLate(ScreenInfo *scr);
extern void EwmhInitVirtualRoots(ScreenInfo *scr);
extern void EwmhTerminate(void);
extern void EwhmSelectionClear(XSelectionClearEvent *sev);
extern int EwmhClientMessage(XClientMessageEvent *msg);
extern Image *EwhmGetIcon(ScreenInfo *scr, TwmWindow *twm_win);
extern int EwmhHandlePropertyNotify(XPropertyEvent *event, TwmWindow *twm_win);
extern void EwmhSet_NET_WM_DESKTOP(TwmWindow *twm_win);
extern void EwmhSet_NET_WM_DESKTOP_ws(TwmWindow *twm_win, WorkSpace *ws);
extern int EwmhGetOccupation(TwmWindow *twm_win);
extern void EwmhUnmapNotify(TwmWindow *twm_win);
extern void EwmhAddClientWindow(TwmWindow *new_win);
extern void EwmhDeleteClientWindow(TwmWindow *old_win);
extern void EwmhSet_NET_CLIENT_LIST_STACKING(void);
extern void EwmhSet_NET_ACTIVE_WINDOW(Window w);
extern void EwmhGetProperties(TwmWindow *twm_win);
extern int EwmhGetPriority(TwmWindow *twm_win);
extern Bool EwmhHasBorder(TwmWindow *twm_win);
extern Bool EwmhHasTitle(TwmWindow *twm_win);
extern Bool EwmhOnWindowRing(TwmWindow *twm_win);
extern void EwmhSet_NET_SHOWING_DESKTOP(int state);
extern void EwmhSet_NET_WM_STATE(TwmWindow *twm_win, int changes);

#endif /* EWMH */
#endif /* _CTWM_EWMH_H */
