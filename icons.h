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

/**********************************************************************
 *
 * $XConsortium: icons.h,v 1.4 89/07/18 17:16:24 jim Exp $
 *
 * Icon releated definitions
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef _CTWM_ICONS_H
#define _CTWM_ICONS_H

/* Where dit the Image for the Icon come from? */
typedef enum {
	match_none,
	match_list,                 /* shared Image: iconslist and Scr->ImageCache */
	match_icon_pixmap_hint,     /* Pixmap copied from IconPixmapHint */
	match_net_wm_icon,          /* Pixmap created from NET_WM_ICON */
	match_unknown_default,      /* shared Image: Scr->UnknownImage */
} Matchtype;

struct Icon {
	Matchtype   match;
	Window      w;              /* the icon window */
	OtpWinList *otp;            /* OnTopPriority info for the icon */
	Window      bm_w;           /* the icon bitmap window */
	Image       *image;         /* image icon structure */
	int         x;              /* icon text x coordinate */
	int         y;              /* icon text y coordiante */
	int         w_x;            /* x coor of the icon window !!untested!! */
	int         w_y;            /* y coor of the icon window !!untested!! */
	int         w_width;        /* width of the icon window */
	int         w_height;       /* height of the icon window */
	int         width;          /* width of the icon bitmap */
	int         height;         /* height of the icon bitmap */
	Pixel       border;         /* border color */
	ColorPair   iconc;
	int         border_width;
	struct IconRegion   *ir;
	bool        has_title, title_shrunk;
	bool        w_not_ours;     /* Icon.w comes from IconWindowHint */
};

struct IconRegion {
	struct IconRegion   *next;
	int                 x, y, w, h;
	RegGravity          grav1, grav2;
	int                 stepx, stepy;       // allocation granularity
	TitleJust           TitleJustification;
	IRJust              Justification;
	IRAlignement        Alignement;
	name_list           *clientlist;
	struct IconEntry    *entries;
};

struct IconEntry {
	struct IconEntry    *next;
	int                 x, y, w, h;
	TwmWindow           *twm_win;
	bool                used;
};

void IconUp(TwmWindow *tmp_win);
void IconDown(TwmWindow *tmp_win);
name_list **AddIconRegion(char *geom, RegGravity grav1, RegGravity grav2,
                          int stepx, int stepy,
                          char *ijust, char *just, char *align);
void CreateIconWindow(TwmWindow *tmp_win, int def_x, int def_y);
void ReleaseImage(Icon *icon);
void DeleteIcon(Icon *icon);
void DeleteIconsList(TwmWindow *tmp_win);
void ShrinkIconTitle(TwmWindow *tmp_win);
void ExpandIconTitle(TwmWindow *tmp_win);
int GetIconOffset(Icon *icon);

#endif /* _CTWM_ICONS_H */
