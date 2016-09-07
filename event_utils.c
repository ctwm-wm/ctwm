/*
 * Various util-ish functions for event handling.
 */

#include "ctwm.h"

#include <stdio.h>

#include "event_internal.h"
#include "events.h"
#include "otp.h"
#include "screen.h"
#include "win_iconify.h"
#include "workspace_manager.h"



void
AutoRaiseWindow(TwmWindow *tmp)
{
	OtpRaise(tmp, WinWin);

	if(ActiveMenu && ActiveMenu->w) {
		XRaiseWindow(dpy, ActiveMenu->w);
	}
	XSync(dpy, 0);
	enter_win = NULL;
	enter_flag = true;
	raise_win = tmp;
	WMapRaise(tmp);
}

void
SetRaiseWindow(TwmWindow *tmp)
{
	enter_flag = true;
	enter_win = NULL;
	raise_win = tmp;
	leave_win = NULL;
	leave_flag = false;
	lower_win = NULL;
	XSync(dpy, 0);
}

void
AutoPopupMaybe(TwmWindow *tmp)
{
	if(LookInList(Scr->AutoPopupL, tmp->full_name, &tmp->class)
	                || Scr->AutoPopup) {
		if(OCCUPY(tmp, Scr->currentvs->wsw->currentwspc)) {
			if(!tmp->mapped) {
				DeIconify(tmp);
				SetRaiseWindow(tmp);
			}
		}
		else {
			tmp->mapped = true;
		}
	}
}

void
AutoLowerWindow(TwmWindow *tmp)
{
	OtpLower(tmp, WinWin);

	if(ActiveMenu && ActiveMenu->w) {
		XRaiseWindow(dpy, ActiveMenu->w);
	}
	XSync(dpy, 0);
	enter_win = NULL;
	enter_flag = false;
	raise_win = NULL;
	leave_win = NULL;
	leave_flag = true;
	lower_win = tmp;
	WMapLower(tmp);
}
