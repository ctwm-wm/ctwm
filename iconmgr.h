/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
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
 * $XConsortium: iconmgr.h,v 1.11 89/12/10 17:47:02 jim Exp $
 *
 * Icon Manager includes
 *
 * 09-Mar-89 Tom LaStrange              File Created
 *
 ***********************************************************************/

#ifndef _CTWM_ICONMGR_H
#define _CTWM_ICONMGR_H

struct WList {
	struct WList *next;
	struct WList *prev;
	struct WList *nextv;                /* pointer to the next virtual Wlist C.L. */
	struct TwmWindow *twm;
	struct IconMgr *iconmgr;
	Window w;
	Window icon;
	int x, y, width, height;
	int row, col;
	int me;
	ColorPair cp;
	Pixel highlight;
	Pixmap iconifypm;
	unsigned top, bottom;
	short active;
	short down;
};

struct IconMgr {
	struct IconMgr *next;               /* ptr to the next icon manager */
	struct IconMgr *prev;               /* ptr to the previous icon mgr */
	struct IconMgr *lasti;              /* ptr to the last icon mgr */
	struct IconMgr *nextv;              /* ptr to the next virt icon mgr */
	struct WList *first;                /* first window in the list */
	struct WList *last;                 /* last window in the list */
	struct WList *active;               /* the active entry */
	TwmWindow *twm_win;                 /* back pointer to the new parent */
	struct ScreenInfo *scr;             /* the screen this thing is on */
	int vScreen;                        /* the virtual screen this thing is on */
	Window w;                           /* this icon manager window */
	char *geometry;                     /* geometry string */
	char *name;
	char *icon_name;
	int x, y, width, height;
	int columns, cur_rows, cur_columns;
	int count;
};

extern const int siconify_width;
extern const int siconify_height;
extern int iconmgr_textx;
extern WList *DownIconManager;

void CreateIconManagers(void);
IconMgr *AllocateIconManager(char *name, char *geom, char *icon_name,
                             int columns);
void MoveIconManager(int dir);
void MoveMappedIconManager(int dir);
void JumpIconManager(int dir);
WList *AddIconManager(TwmWindow *tmp_win);
void InsertInIconManager(IconMgr *ip, WList *tmp, TwmWindow *tmp_win);
void RemoveFromIconManager(IconMgr *ip, WList *tmp);
void RemoveIconManager(TwmWindow *tmp_win);
void CurrentIconManagerEntry(WList *current);
void ActiveIconManager(WList *active);
void NotActiveIconManager(WList *active);
void DrawIconManagerBorder(WList *tmp, int fill);
void SortIconManager(IconMgr *ip);
void PackIconManager(IconMgr *ip);
void PackIconManagers(void);
void dump_iconmanager(IconMgr *mgr, char *label);


#endif /* _CTWM_ICONMGR_H */
