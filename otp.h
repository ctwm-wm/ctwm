/*
 *  [ ctwm ]
 *
 *  Copyright 1992, 2005, 2007 Stefan Monnier.
 *
 * Permission to use, copy, modify  and distribute this software  [ctwm] and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above  copyright notice appear  in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of  Stefan Monnier not be used in adverti-
 * sing or  publicity  pertaining to  distribution of  the software  without
 * specific, written prior permission. Stefan Monnier make no represen-
 * tations  about the suitability  of this software  for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Stefan Monnier DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL  Stefan Monnier  BE LIABLE FOR ANY SPECIAL,  INDIRECT OR
 * CONSEQUENTIAL  DAMAGES OR ANY  DAMAGES WHATSOEVER  RESULTING FROM LOSS OF
 * USE, DATA  OR PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR
 * OTHER  TORTIOUS ACTION,  ARISING OUT OF OR IN  CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Stefan Monnier [ monnier@lia.di.epfl.ch ]
 *
 * $Id: otp.h,v 1.7 2005/04/08 16:59:17 monnier Exp $
 *
 * handles all the OnTopPriority-related issues.
 *
 */

#ifndef _CTWM_OTP_H
#define _CTWM_OTP_H

#include "types.h"

/* kind of window */
typedef enum WinType { WinWin, IconWin } WinType;

/* Wrapper functions to maintain the internal list uptodate.  */
extern int ReparentWindow(Display *display, TwmWindow *twm_win,
                          WinType wintype, Window parent, int x, int y);
extern void ReparentWindowAndIcon(Display *display, TwmWindow *twm_win,
                                  Window parent, int win_x, int win_y,
                                  int icon_x, int icon_y);

/* misc functions that are not specific to OTP */
extern Bool isTransientOf(TwmWindow *, TwmWindow *);
extern Bool isSmallTransientOf(TwmWindow *, TwmWindow *);
extern Bool isGroupLeaderOf(TwmWindow *, TwmWindow *);
extern Bool isGroupLeader(TwmWindow *);

/* functions to "move" windows */
extern void OtpRaise(TwmWindow *, WinType);
extern void OtpLower(TwmWindow *, WinType);
extern void OtpRaiseLower(TwmWindow *, WinType);
extern void OtpTinyRaise(TwmWindow *, WinType);
extern void OtpTinyLower(TwmWindow *, WinType);
extern void OtpCirculateSubwindows(VirtualScreen *vs, int direction);
extern void OtpHandleCirculateNotify(VirtualScreen *vs, TwmWindow *twm_win,
                                     WinType wintype, int place);

/* functions to change a window's OTP value */
extern void OtpSetPriority(TwmWindow *, WinType, int, int);
extern void OtpChangePriority(TwmWindow *, WinType, int);
extern void OtpSwitchPriority(TwmWindow *, WinType);
extern void OtpToggleSwitching(TwmWindow *, WinType);
extern void OtpRecomputeValues(TwmWindow *);
extern void OtpForcePlacement(TwmWindow *, int, TwmWindow *);

extern void OtpReassignIcon(TwmWindow *twm_win, Icon *old_icon);
extern void OtpFreeIcon(TwmWindow *twm_win);

/* functions to manage the preferences. The second arg specifies icon prefs */
extern void OtpScrInitData(ScreenInfo *);
extern name_list **OtpScrSwitchingL(ScreenInfo *, WinType);
extern name_list **OtpScrPriorityL(ScreenInfo *, WinType, int);
extern void OtpScrSetSwitching(ScreenInfo *, WinType, Bool);
extern void OtpScrSetZero(ScreenInfo *, WinType, int);

/* functions to inform OTP-manager of window creation/destruction */
extern void OtpAdd(TwmWindow *, WinType);
extern void OtpRemove(TwmWindow *, WinType);

/* Iterators.  */
extern TwmWindow *OtpBottomWin(void);
extern TwmWindow *OtpTopWin(void);
extern TwmWindow *OtpNextWinUp(TwmWindow *);
extern TwmWindow *OtpNextWinDown(TwmWindow *);

/* Other access functions */
extern int OtpGetPriority(TwmWindow *twm_win);

/* Other debugging functions */
extern Bool OtpCheckConsistency(void);

#endif /* _CTWM_OTP_H */
