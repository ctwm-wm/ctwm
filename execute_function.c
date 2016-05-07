/*
 * Dispatched for our f.whatever functions
 */


#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <X11/Xatom.h>

#include "add_window.h"
#include "animate.h"
#include "ctopts.h"
#include "ctwm_atoms.h"
#include "cursor.h"
#include "events.h"
#include "icons.h"
#include "menus.h"
#include "otp.h"
#include "parse.h"
#include "resize.h"
#include "screen.h"
#include "types.h"
#include "util.h"
#include "version.h"
#include "windowbox.h"

#include "ext/repl_str.h"

#include "execute_function.h"


int RootFunction = 0;
int MoveFunction;  /* either F_MOVE or F_FORCEMOVE */

/* Building the f.identify window.  The events code grubs in these. */
char Info[INFO_LINES][INFO_SIZE];
int InfoLines;
unsigned int InfoWidth, InfoHeight;


/*
 * Constrained move variables
 *
 * Gets used in event handling for ButtonRelease.
 */
int ConstMove = FALSE;
int ConstMoveDir;
int ConstMoveX;
int ConstMoveY;
int ConstMoveXL;
int ConstMoveXR;
int ConstMoveYT;
int ConstMoveYB;

int WindowMoved = FALSE;

/*
 * Globals used to keep track of whether the mouse has moved during a
 * resize function.
 */
int ResizeOrigX;
int ResizeOrigY;




static void jump(TwmWindow *tmp_win, int direction, char *action);
static void ShowIconManager(void);
static void HideIconManager(void);
static int DeferExecution(int context, int func, Cursor cursor);
static void Identify(TwmWindow *t);
static bool belongs_to_twm_window(TwmWindow *t, Window w);
static void packwindow(TwmWindow *tmp_win, char *direction);
static void fillwindow(TwmWindow *tmp_win, char *direction);
static int NeedToDefer(MenuRoot *root);
static void Execute(const char *_s);
static void SendSaveYourselfMessage(TwmWindow *tmp, Time timestamp);
static void SendDeleteWindowMessage(TwmWindow *tmp, Time timestamp);
static void BumpWindowColormap(TwmWindow *tmp, int inc);
static int FindConstraint(TwmWindow *tmp_win, int direction);


/***********************************************************************
 *
 *  Procedure:
 *      ExecuteFunction - execute a twm root function
 *
 *  Inputs:
 *      func    - the function to execute
 *      action  - the menu action to execute
 *      w       - the window to execute this function on
 *      tmp_win - the twm window structure
 *      event   - the event that caused the function
 *      context - the context in which the button was pressed
 *      pulldown- flag indicating execution from pull down menu
 *
 *  Returns:
 *      TRUE if should continue with remaining actions else FALSE to abort
 *
 ***********************************************************************
 */

int
ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                XEvent *eventp, int context, int pulldown)
{
	static Time last_time = 0;
	Window rootw;
	int origX, origY;
	int do_next_action = TRUE;
	int moving_icon = FALSE;
	Bool fromtitlebar = False;
	Bool from3dborder = False;
	TwmWindow *t;

	RootFunction = 0;
	if(Cancel) {
		return TRUE;        /* XXX should this be FALSE? */
	}

	switch(func) {
		case F_UPICONMGR:
		case F_LEFTICONMGR:
		case F_RIGHTICONMGR:
		case F_DOWNICONMGR:
		case F_FORWICONMGR:
		case F_BACKICONMGR:
		case F_NEXTICONMGR:
		case F_PREVICONMGR:
		case F_NOP:
		case F_TITLE:
		case F_DELTASTOP:
		case F_RAISELOWER:
		case F_WARPTOSCREEN:
		case F_WARPTO:
		case F_WARPRING:
		case F_WARPTOICONMGR:
		case F_COLORMAP:
		case F_ALTKEYMAP:
		case F_ALTCONTEXT:
			break;

		default:
			XGrabPointer(dpy, Scr->Root, True,
			             ButtonPressMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync,
			             Scr->Root, Scr->WaitCursor, CurrentTime);
			break;
	}

	switch(func) {
#ifdef SOUNDS
		case F_TOGGLESOUND:
			toggle_sound();
			break;
		case F_REREADSOUNDS:
			reread_sounds();
			break;
#endif
		case F_NOP:
		case F_TITLE:
			break;

		case F_DELTASTOP:
			if(WindowMoved) {
				do_next_action = FALSE;
			}
			break;

		case F_RESTART: {
			DoRestart(eventp->xbutton.time);
			break;
		}
		case F_UPICONMGR:
		case F_DOWNICONMGR:
		case F_LEFTICONMGR:
		case F_RIGHTICONMGR:
		case F_FORWICONMGR:
		case F_BACKICONMGR:
			MoveIconManager(func);
			break;

		case F_FORWMAPICONMGR:
		case F_BACKMAPICONMGR:
			MoveMappedIconManager(func);
			break;

		case F_NEXTICONMGR:
		case F_PREVICONMGR:
			JumpIconManager(func);
			break;

		case F_SHOWLIST:
			if(Scr->NoIconManagers) {
				break;
			}
			ShowIconManager();
			break;

		case F_STARTANIMATION :
			StartAnimation();
			break;

		case F_STOPANIMATION :
			StopAnimation();
			break;

		case F_SPEEDUPANIMATION :
			ModifyAnimationSpeed(1);
			break;

		case F_SLOWDOWNANIMATION :
			ModifyAnimationSpeed(-1);
			break;

		case F_HIDELIST:
			if(Scr->NoIconManagers) {
				break;
			}
			HideIconManager();
			break;

		case F_SHOWWORKMGR:
			if(! Scr->workSpaceManagerActive) {
				break;
			}
			DeIconify(Scr->currentvs->wsw->twm_win);
			OtpRaise(Scr->currentvs->wsw->twm_win, WinWin);
			break;

		case F_HIDEWORKMGR:
			if(! Scr->workSpaceManagerActive) {
				break;
			}
			Iconify(Scr->currentvs->wsw->twm_win, eventp->xbutton.x_root - 5,
			        eventp->xbutton.y_root - 5);
			break;

		case F_TOGGLEWORKMGR:
			if(! Scr->workSpaceManagerActive) {
				break;
			}
			if(Scr->currentvs->wsw->twm_win->mapped)
				Iconify(Scr->currentvs->wsw->twm_win, eventp->xbutton.x_root - 5,
				        eventp->xbutton.y_root - 5);
			else {
				DeIconify(Scr->currentvs->wsw->twm_win);
				OtpRaise(Scr->currentvs->wsw->twm_win, WinWin);
			}
			break;

		case F_TOGGLESTATE :
			WMapToggleState(Scr->currentvs);
			break;

		case F_SETBUTTONSTATE :
			WMapSetButtonsState(Scr->currentvs);
			break;

		case F_SETMAPSTATE :
			WMapSetMapState(Scr->currentvs);
			break;

		case F_PIN :
			if(! ActiveMenu) {
				break;
			}
			if(ActiveMenu->pinned) {
				XUnmapWindow(dpy, ActiveMenu->w);
				ActiveMenu->mapped = UNMAPPED;
			}
			else {
				XWindowAttributes attr;
				MenuRoot *menu;

				if(ActiveMenu->pmenu == NULL) {
					menu  = (MenuRoot *) malloc(sizeof(MenuRoot));
					*menu = *ActiveMenu;
					menu->pinned = True;
					menu->mapped = NEVER_MAPPED;
					menu->width -= 10;
					if(menu->pull) {
						menu->width -= 16 + 10;
					}
					MakeMenu(menu);
					ActiveMenu->pmenu = menu;
				}
				else {
					menu = ActiveMenu->pmenu;
				}
				if(menu->mapped == MAPPED) {
					break;
				}
				XGetWindowAttributes(dpy, ActiveMenu->w, &attr);
				menu->x = attr.x;
				menu->y = attr.y;
				XMoveWindow(dpy, menu->w, menu->x, menu->y);
				XMapRaised(dpy, menu->w);
				menu->mapped = MAPPED;
			}
			PopDownMenu();
			break;

		case F_MOVEMENU:
			break;

		case F_FITTOCONTENT :
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			if(!tmp_win->iswinbox) {
				XBell(dpy, 0);
				break;
			}
			fittocontent(tmp_win);
			break;

		case F_VANISH:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			WMgrRemoveFromCurrentWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_WARPHERE:
			WMgrAddToCurrentWorkSpaceAndWarp(Scr->currentvs, action);
			break;

		case F_ADDTOWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			AddToWorkSpace(action, tmp_win);
			break;

		case F_REMOVEFROMWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			RemoveFromWorkSpace(action, tmp_win);
			break;

		case F_TOGGLEOCCUPATION:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			ToggleOccupation(action, tmp_win);
			break;

		case F_MOVETONEXTWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			MoveToNextWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_MOVETOPREVWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			MoveToPrevWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_MOVETONEXTWORKSPACEANDFOLLOW:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			MoveToNextWorkSpaceAndFollow(Scr->currentvs, tmp_win);
			break;

		case F_MOVETOPREVWORKSPACEANDFOLLOW:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			MoveToPrevWorkSpaceAndFollow(Scr->currentvs, tmp_win);
			break;

		case F_SORTICONMGR:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			{
				int save_sort;

				save_sort = Scr->SortIconMgr;
				Scr->SortIconMgr = TRUE;

				if(context == C_ICONMGR) {
					SortIconManager((IconMgr *) NULL);
				}
				else if(tmp_win->iconmgr) {
					SortIconManager(tmp_win->iconmgrp);
				}
				else {
					XBell(dpy, 0);
				}

				Scr->SortIconMgr = save_sort;
			}
			break;

		case F_ALTKEYMAP: {
			int alt, stat_;

			if(! action) {
				return TRUE;
			}
			stat_ = sscanf(action, "%d", &alt);
			if(stat_ != 1) {
				return TRUE;
			}
			if((alt < 1) || (alt > 5)) {
				return TRUE;
			}
			AlternateKeymap = Alt1Mask << (alt - 1);
			XGrabPointer(dpy, Scr->Root, True, ButtonPressMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync,
			             Scr->Root, Scr->AlterCursor, CurrentTime);
			XGrabKeyboard(dpy, Scr->Root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
			return TRUE;
		}

		case F_ALTCONTEXT: {
			AlternateContext = True;
			XGrabPointer(dpy, Scr->Root, False, ButtonPressMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync,
			             Scr->Root, Scr->AlterCursor, CurrentTime);
			XGrabKeyboard(dpy, Scr->Root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
			return TRUE;
		}
		case F_IDENTIFY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			Identify(tmp_win);
			break;

		case F_INITSIZE: {
			int grav, x, y;
			unsigned int width, height, swidth, sheight;

			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			grav = ((tmp_win->hints.flags & PWinGravity)
			        ? tmp_win->hints.win_gravity : NorthWestGravity);

			if(!(tmp_win->hints.flags & USSize) && !(tmp_win->hints.flags & PSize)) {
				break;
			}

			width  = tmp_win->hints.width  + 2 * tmp_win->frame_bw3D;
			height  = tmp_win->hints.height + 2 * tmp_win->frame_bw3D +
			          tmp_win->title_height;
			ConstrainSize(tmp_win, &width, &height);

			x  = tmp_win->frame_x;
			y  = tmp_win->frame_y;
			swidth = tmp_win->frame_width;
			sheight = tmp_win->frame_height;
			switch(grav) {
				case ForgetGravity :
				case StaticGravity :
				case NorthWestGravity :
				case NorthGravity :
				case WestGravity :
				case CenterGravity :
					break;

				case NorthEastGravity :
				case EastGravity :
					x += swidth - width;
					break;

				case SouthWestGravity :
				case SouthGravity :
					y += sheight - height;
					break;

				case SouthEastGravity :
					x += swidth - width;
					y += sheight - height;
					break;
			}
			SetupWindow(tmp_win, x, y, width, height, -1);
			break;
		}

		case F_MOVERESIZE: {
			int x, y, mask;
			unsigned int width, height;
			int px = 20, py = 30;

			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			mask = XParseGeometry(action, &x, &y, &width, &height);
			if(!(mask &  WidthValue)) {
				width = tmp_win->frame_width;
			}
			else {
				width += 2 * tmp_win->frame_bw3D;
			}
			if(!(mask & HeightValue)) {
				height = tmp_win->frame_height;
			}
			else {
				height += 2 * tmp_win->frame_bw3D + tmp_win->title_height;
			}
			ConstrainSize(tmp_win, &width, &height);
			if(mask & XValue) {
				if(mask & XNegative) {
					x += Scr->rootw  - width;
				}
			}
			else {
				x = tmp_win->frame_x;
			}
			if(mask & YValue) {
				if(mask & YNegative) {
					y += Scr->rooth - height;
				}
			}
			else {
				y = tmp_win->frame_y;
			}

			{
				int          junkX, junkY;
				unsigned int junkK;
				Window       junkW;
				XQueryPointer(dpy, Scr->Root, &junkW, &junkW, &junkX, &junkY, &px, &py, &junkK);
			}
			px -= tmp_win->frame_x;
			if(px > width) {
				px = width / 2;
			}
			py -= tmp_win->frame_y;
			if(py > height) {
				px = height / 2;
			}
			SetupWindow(tmp_win, x, y, width, height, -1);
			XWarpPointer(dpy, Scr->Root, Scr->Root, 0, 0, 0, 0, x + px, y + py);
			break;
		}

		case F_VERSION:
			Identify((TwmWindow *) NULL);
			break;

		case F_AUTORAISE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			tmp_win->auto_raise = !tmp_win->auto_raise;
			if(tmp_win->auto_raise) {
				++(Scr->NumAutoRaises);
			}
			else {
				--(Scr->NumAutoRaises);
			}
			break;

		case F_AUTOLOWER:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			tmp_win->auto_lower = !tmp_win->auto_lower;
			if(tmp_win->auto_lower) {
				++(Scr->NumAutoLowers);
			}
			else {
				--(Scr->NumAutoLowers);
			}
			break;

		case F_BEEP:
			XBell(dpy, 0);
			break;

		case F_POPUP:
			tmp_win = (TwmWindow *)action;
			if(! tmp_win) {
				break;
			}
			if(Scr->WindowFunction.func != 0) {
				ExecuteFunction(Scr->WindowFunction.func,
				                Scr->WindowFunction.item->action,
				                w, tmp_win, eventp, C_FRAME, FALSE);
			}
			else {
				DeIconify(tmp_win);
				OtpRaise(tmp_win, WinWin);
			}
			break;

		case F_WINWARP:
			tmp_win = (TwmWindow *)action;

			if(! tmp_win) {
				break;
			}
			if(Scr->WarpUnmapped || tmp_win->mapped) {
				if(!tmp_win->mapped) {
					DeIconify(tmp_win);
				}
				WarpToWindow(tmp_win, Scr->RaiseOnWarp);
			}
			break;

		case F_RESIZE:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}

			PopDownMenu();
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			EventHandler[EnterNotify] = HandleUnknown;
			EventHandler[LeaveNotify] = HandleUnknown;

			OpaqueResizeSize(tmp_win);

			if(pulldown)
				XWarpPointer(dpy, None, Scr->Root,
				             0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

			if(!tmp_win->icon || (w != tmp_win->icon->w)) {         /* can't resize icons */

				/*        fromMenu = False;  ????? */
				if((Context == C_FRAME || Context == C_WINDOW || Context == C_TITLE
				                || Context == C_ROOT)
				                && cur_fromMenu()) {
					resizeFromCenter(w, tmp_win);
				}
				else {
					/*
					 * see if this is being done from the titlebar
					 */
					from3dborder = (eventp->xbutton.window == tmp_win->frame);
					fromtitlebar = !from3dborder &&
					               belongs_to_twm_window(tmp_win, eventp->xbutton.window);

					/* Save pointer position so we can tell if it was moved or
					   not during the resize. */
					ResizeOrigX = eventp->xbutton.x_root;
					ResizeOrigY = eventp->xbutton.y_root;

					StartResize(eventp, tmp_win, fromtitlebar, from3dborder);

					do {
						XMaskEvent(dpy,
						           ButtonPressMask | ButtonReleaseMask |
						           EnterWindowMask | LeaveWindowMask |
						           ButtonMotionMask | VisibilityChangeMask | ExposureMask, &Event);

						if(fromtitlebar && Event.type == ButtonPress) {
							fromtitlebar = False;
							continue;
						}

						if(Event.type == MotionNotify) {
							/* discard any extra motion events before a release */
							while
							(XCheckMaskEvent
							                (dpy, ButtonMotionMask | ButtonReleaseMask, &Event))
								if(Event.type == ButtonRelease) {
									break;
								}
						}

						if(!DispatchEvent2()) {
							continue;
						}

					}
					while(!(Event.type == ButtonRelease || Cancel));
					return TRUE;
				}
			}
			break;


		case F_VERTZOOM:
		case F_HORIZOOM:
		case F_FULLZOOM:
		case F_FULLSCREENZOOM:
		case F_LEFTZOOM:
		case F_RIGHTZOOM:
		case F_TOPZOOM:
		case F_BOTTOMZOOM:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			fullzoom(tmp_win, func);
			break;

		case F_PACK:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			packwindow(tmp_win, action);
			break;

		case F_FILL:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			fillwindow(tmp_win, action);
			break;

		case F_JUMPLEFT:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, J_LEFT, action);
			break;
		case F_JUMPRIGHT:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, J_RIGHT, action);
			break;
		case F_JUMPDOWN:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, J_BOTTOM, action);
			break;
		case F_JUMPUP:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, J_TOP, action);
			break;

		case F_SAVEGEOMETRY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			savegeometry(tmp_win);
			break;

		case F_RESTOREGEOMETRY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			restoregeometry(tmp_win);
			break;

		case F_HYPERMOVE: {
			bool    cont = true;
			Window  root = RootWindow(dpy, Scr->screen);
			Cursor  cursor;
			CaptiveCTWM cctwm0, cctwm;

			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}

			if(tmp_win->iswinbox || tmp_win->wspmgr) {
				XBell(dpy, 0);
				break;
			}
			cctwm0 = GetCaptiveCTWMUnderPointer();
			cursor = MakeStringCursor(cctwm0.name);
			free(cctwm0.name);
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}

			XGrabPointer(dpy, root, True,
			             ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);
			while(cont) {
				XMaskEvent(dpy, ButtonPressMask | ButtonMotionMask |
				           ButtonReleaseMask, &Event);
				switch(Event.xany.type) {
					case ButtonPress :
						cont = false;
						break;

					case ButtonRelease :
						cont = false;
						cctwm = GetCaptiveCTWMUnderPointer();
						free(cctwm.name);
						if(cctwm.root == Scr->Root) {
							break;
						}
						if(cctwm.root == Scr->XineramaRoot) {
							break;
						}
						SetNoRedirect(tmp_win->w);
						XUngrabButton(dpy, AnyButton, AnyModifier, tmp_win->w);
						XReparentWindow(dpy, tmp_win->w, cctwm.root, 0, 0);
						XMapWindow(dpy, tmp_win->w);
						break;

					case MotionNotify :
						cctwm = GetCaptiveCTWMUnderPointer();
						if(cctwm.root != cctwm0.root) {
							XFreeCursor(dpy, cursor);
							cursor = MakeStringCursor(cctwm.name);
							cctwm0 = cctwm;
							XChangeActivePointerGrab(dpy,
							                         ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
							                         cursor, CurrentTime);
						}
						free(cctwm.name);
						break;
				}
			}
			ButtonPressed = -1;
			XUngrabPointer(dpy, CurrentTime);
			XFreeCursor(dpy, cursor);
			break;
		}

		case F_MOVE:
		case F_FORCEMOVE:
		case F_MOVEPACK:
		case F_MOVEPUSH: {
			Window grabwin, dragroot;

			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}

			PopDownMenu();
			if(tmp_win->OpaqueMove) {
				int sw, ss;
				float sf;

				sw = tmp_win->frame_width * tmp_win->frame_height;
				ss = Scr->rootw  * Scr->rooth;
				sf = Scr->OpaqueMoveThreshold / 100.0;
				if(sw > (ss * sf)) {
					Scr->OpaqueMove = FALSE;
				}
				else {
					Scr->OpaqueMove = TRUE;
				}
			}
			else {
				Scr->OpaqueMove = FALSE;
			}

			dragroot = Scr->XineramaRoot;

			if(tmp_win->winbox) {
				XTranslateCoordinates(dpy, dragroot, tmp_win->winbox->window,
				                      eventp->xbutton.x_root, eventp->xbutton.y_root,
				                      &(eventp->xbutton.x_root), &(eventp->xbutton.y_root), &JunkChild);
			}
			rootw = eventp->xbutton.root;
			MoveFunction = func;

			if(pulldown)
				XWarpPointer(dpy, None, Scr->Root,
				             0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

			EventHandler[EnterNotify] = HandleUnknown;
			EventHandler[LeaveNotify] = HandleUnknown;

			if(!Scr->NoGrabServer || !Scr->OpaqueMove) {
				XGrabServer(dpy);
			}

			Scr->SizeStringOffset = SIZE_HINDENT;
			XResizeWindow(dpy, Scr->SizeWindow,
			              Scr->SizeStringWidth + SIZE_HINDENT * 2,
			              Scr->SizeFont.height + SIZE_VINDENT * 2);
			XMapRaised(dpy, Scr->SizeWindow);

			grabwin = Scr->XineramaRoot;
			if(tmp_win->winbox) {
				grabwin = tmp_win->winbox->window;
			}
			XGrabPointer(dpy, grabwin, True,
			             ButtonPressMask | ButtonReleaseMask |
			             ButtonMotionMask | PointerMotionMask, /* PointerMotionHintMask */
			             GrabModeAsync, GrabModeAsync, grabwin, Scr->MoveCursor, CurrentTime);

			if(context == C_ICON && tmp_win->icon && tmp_win->icon->w) {
				w = tmp_win->icon->w;
				DragX = eventp->xbutton.x;
				DragY = eventp->xbutton.y;
				moving_icon = TRUE;
				if(tmp_win->OpaqueMove) {
					Scr->OpaqueMove = TRUE;
				}
			}

			else if(! tmp_win->icon || w != tmp_win->icon->w) {
				XTranslateCoordinates(dpy, w, tmp_win->frame,
				                      eventp->xbutton.x,
				                      eventp->xbutton.y,
				                      &DragX, &DragY, &JunkChild);

				w = tmp_win->frame;
			}

			DragWindow = None;

			/* Get x/y relative to parent window, i.e. the virtual screen, Root.
			 * XMoveWindow() moves are relative to this.
			 * MoveOutline()s however are drawn from the XineramaRoot since they
			 * may cross virtual screens.
			 */
			XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
			             &DragWidth, &DragHeight, &DragBW,
			             &JunkDepth);

			JunkBW = DragBW;
			origX = eventp->xbutton.x_root;
			origY = eventp->xbutton.y_root;
			CurrentDragX = origDragX;
			CurrentDragY = origDragY;

			/*
			 * only do the constrained move if timer is set; need to check it
			 * in case of stupid or wicked fast servers
			 */
			if(ConstrainedMoveTime &&
			                (eventp->xbutton.time - last_time) < ConstrainedMoveTime) {
				int width, height;

				ConstMove = TRUE;
				ConstMoveDir = MOVE_NONE;
				ConstMoveX = eventp->xbutton.x_root - DragX - JunkBW;
				ConstMoveY = eventp->xbutton.y_root - DragY - JunkBW;
				width = DragWidth + 2 * JunkBW;
				height = DragHeight + 2 * JunkBW;
				ConstMoveXL = ConstMoveX + width / 3;
				ConstMoveXR = ConstMoveX + 2 * (width / 3);
				ConstMoveYT = ConstMoveY + height / 3;
				ConstMoveYB = ConstMoveY + 2 * (height / 3);

				XWarpPointer(dpy, None, w,
				             0, 0, 0, 0, DragWidth / 2, DragHeight / 2);

				XQueryPointer(dpy, w, &JunkRoot, &JunkChild,
				              &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
			}
			last_time = eventp->xbutton.time;

			if(!Scr->OpaqueMove) {
				InstallRootColormap();
				if(!Scr->MoveDelta) {
					/*
					 * Draw initial outline.  This was previously done the
					 * first time though the outer loop by dropping out of
					 * the XCheckMaskEvent inner loop down to one of the
					 * MoveOutline's below.
					 */
					MoveOutline(dragroot,
					            origDragX - JunkBW + Scr->currentvs->x,
					            origDragY - JunkBW + Scr->currentvs->y,
					            DragWidth + 2 * JunkBW, DragHeight + 2 * JunkBW,
					            tmp_win->frame_bw,
					            moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);
					/*
					 * This next line causes HandleReleaseNotify to call
					 * XRaiseWindow().  This is solely to preserve the
					 * previous behaviour that raises a window being moved
					 * on button release even if you never actually moved
					 * any distance (unless you move less than MoveDelta or
					 * NoRaiseMove is set or OpaqueMove is set).
					 */
					DragWindow = w;
				}
			}

			/*
			 * see if this is being done from the titlebar
			 */
			fromtitlebar = belongs_to_twm_window(tmp_win, eventp->xbutton.window);

			if(menuFromFrameOrWindowOrTitlebar) {
				/* warp the pointer to the middle of the window */
				XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
				             origDragX + DragWidth / 2,
				             origDragY + DragHeight / 2);
				XFlush(dpy);
			}

			DisplayPosition(tmp_win, CurrentDragX, CurrentDragY);
			while(TRUE) {
				long releaseEvent = menuFromFrameOrWindowOrTitlebar ?
				                    ButtonPress : ButtonRelease;
				long movementMask = menuFromFrameOrWindowOrTitlebar ?
				                    PointerMotionMask : ButtonMotionMask;

				/* block until there is an interesting event */
				XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
				           EnterWindowMask | LeaveWindowMask |
				           ExposureMask | movementMask |
				           VisibilityChangeMask, &Event);

				/* throw away enter and leave events until release */
				if(Event.xany.type == EnterNotify ||
				                Event.xany.type == LeaveNotify) {
					continue;
				}

				if(Event.type == MotionNotify) {
					/* discard any extra motion events before a logical release */
					while(XCheckMaskEvent(dpy,
					                      movementMask | releaseEvent, &Event))
						if(Event.type == releaseEvent) {
							break;
						}
				}

				/* test to see if we have a second button press to abort move */
				if(!menuFromFrameOrWindowOrTitlebar) {
					if(Event.type == ButtonPress && DragWindow != None) {
						Cursor cur;
						if(Scr->OpaqueMove) {
							XMoveWindow(dpy, DragWindow, origDragX, origDragY);
							if(moving_icon) {
								tmp_win->icon->w_x = origDragX;
								tmp_win->icon->w_y = origDragY;
							}
						}
						else {
							MoveOutline(dragroot, 0, 0, 0, 0, 0, 0);
						}
						DragWindow = None;

						XUnmapWindow(dpy, Scr->SizeWindow);
						cur = LeftButt;
						if(Event.xbutton.button == Button2) {
							cur = MiddleButt;
						}
						else if(Event.xbutton.button >= Button3) {
							cur = RightButt;
						}

						XGrabPointer(dpy, Scr->Root, True,
						             ButtonReleaseMask | ButtonPressMask,
						             GrabModeAsync, GrabModeAsync,
						             Scr->Root, cur, CurrentTime);
						return TRUE;
					}
				}

				if(fromtitlebar && Event.type == ButtonPress) {
					fromtitlebar = False;
					CurrentDragX = origX = Event.xbutton.x_root;
					CurrentDragY = origY = Event.xbutton.y_root;
					XTranslateCoordinates(dpy, rootw, tmp_win->frame,
					                      origX, origY,
					                      &DragX, &DragY, &JunkChild);
					continue;
				}

				if(!DispatchEvent2()) {
					continue;
				}

				if(Cancel) {
					WindowMoved = FALSE;
					if(!Scr->OpaqueMove) {
						UninstallRootColormap();
					}
					return TRUE;    /* XXX should this be FALSE? */
				}
				if(Event.type == releaseEvent) {
					MoveOutline(dragroot, 0, 0, 0, 0, 0, 0);
					if(moving_icon &&
					                ((CurrentDragX != origDragX ||
					                  CurrentDragY != origDragY))) {
						tmp_win->icon_moved = TRUE;
					}
					if(!Scr->OpaqueMove && menuFromFrameOrWindowOrTitlebar) {
						int xl = Event.xbutton.x_root - (DragWidth  / 2),
						    yt = Event.xbutton.y_root - (DragHeight / 2);
						if(!moving_icon &&
						                (MoveFunction == F_MOVEPACK || MoveFunction == F_MOVEPUSH)) {
							TryToPack(tmp_win, &xl, &yt);
						}
						XMoveWindow(dpy, DragWindow, xl, yt);
					}
					if(menuFromFrameOrWindowOrTitlebar) {
						DragWindow = None;
					}
					break;
				}

				/* something left to do only if the pointer moved */
				if(Event.type != MotionNotify) {
					continue;
				}

				XQueryPointer(dpy, rootw, &(eventp->xmotion.root), &JunkChild,
				              &(eventp->xmotion.x_root), &(eventp->xmotion.y_root),
				              &JunkX, &JunkY, &JunkMask);

				FixRootEvent(eventp);
				if(tmp_win->winbox) {
					XTranslateCoordinates(dpy, dragroot, tmp_win->winbox->window,
					                      eventp->xmotion.x_root, eventp->xmotion.y_root,
					                      &(eventp->xmotion.x_root), &(eventp->xmotion.y_root), &JunkChild);
				}
				if(DragWindow == None &&
				                abs(eventp->xmotion.x_root - origX) < Scr->MoveDelta &&
				                abs(eventp->xmotion.y_root - origY) < Scr->MoveDelta) {
					continue;
				}

				DragWindow = w;

				if(!Scr->NoRaiseMove && Scr->OpaqueMove && !WindowMoved) {
					if(XFindContext(dpy, DragWindow, TwmContext, (XPointer *) &t) == XCNOENT) {
						fprintf(stderr, "ERROR: menus.c:2822\n");
					}

					if(t != tmp_win) {
						fprintf(stderr, "DragWindow isn't tmp_win!\n");
					}
					if(DragWindow == t->frame) {
						if(moving_icon) {
							fprintf(stderr, "moving_icon is TRUE incorrectly!\n");
						}
						OtpRaise(t, WinWin);
					}
					else if(t->icon && DragWindow == t->icon->w) {
						if(!moving_icon) {
							fprintf(stderr, "moving_icon is FALSE incorrectly!\n");
						}
						OtpRaise(t, IconWin);
					}
					else {
						fprintf(stderr, "ERROR: menus.c:2838\n");
					}
				}

				WindowMoved = TRUE;

				if(ConstMove) {
					switch(ConstMoveDir) {
						case MOVE_NONE:
							if(eventp->xmotion.x_root < ConstMoveXL ||
							                eventp->xmotion.x_root > ConstMoveXR) {
								ConstMoveDir = MOVE_HORIZ;
							}

							if(eventp->xmotion.y_root < ConstMoveYT ||
							                eventp->xmotion.y_root > ConstMoveYB) {
								ConstMoveDir = MOVE_VERT;
							}

							XQueryPointer(dpy, DragWindow, &JunkRoot, &JunkChild,
							              &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
							break;

						case MOVE_VERT:
							ConstMoveY = eventp->xmotion.y_root - DragY - JunkBW;
							break;

						case MOVE_HORIZ:
							ConstMoveX = eventp->xmotion.x_root - DragX - JunkBW;
							break;
					}

					if(ConstMoveDir != MOVE_NONE) {
						int xl, yt, width, height;

						xl = ConstMoveX;
						yt = ConstMoveY;
						width = DragWidth + 2 * JunkBW;
						height = DragHeight + 2 * JunkBW;

						if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
							TryToGrid(tmp_win, &xl, &yt);
						}
						if(!moving_icon && MoveFunction == F_MOVEPUSH && Scr->OpaqueMove) {
							TryToPush(tmp_win, xl, yt, 0);
						}

						if(!moving_icon &&
						                (MoveFunction == F_MOVEPACK || MoveFunction == F_MOVEPUSH)) {
							TryToPack(tmp_win, &xl, &yt);
						}

						if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
							ConstrainByBorders(tmp_win, &xl, width, &yt, height);
						}
						CurrentDragX = xl;
						CurrentDragY = yt;
						if(Scr->OpaqueMove) {
							if(MoveFunction == F_MOVEPUSH && !moving_icon) {
								SetupWindow(tmp_win, xl, yt,
								            tmp_win->frame_width, tmp_win->frame_height, -1);
							}
							else {
								XMoveWindow(dpy, DragWindow, xl, yt);
								if(moving_icon) {
									tmp_win->icon->w_x = xl;
									tmp_win->icon->w_y = yt;
								}
							}
							WMapSetupWindow(tmp_win, xl, yt, -1, -1);
						}
						else {
							MoveOutline(dragroot, xl + Scr->currentvs->x,
							            yt + Scr->currentvs->y, width, height,
							            tmp_win->frame_bw,
							            moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);
						}
					}
				}
				else if(DragWindow != None) {
					int xroot, yroot;
					int xl, yt, width, height;


					/*
					 * this is split out for virtual screens.  In that case, it's
					 * possible to drag windows from one workspace to another, and
					 * as such, these need to be adjusted to the root, rather
					 * than this virtual screen...
					 */
					xroot = eventp->xmotion.x_root;
					yroot = eventp->xmotion.y_root;

					if(!menuFromFrameOrWindowOrTitlebar) {
						xl = xroot - DragX - JunkBW;
						yt = yroot - DragY - JunkBW;
					}
					else {
						xl = xroot - (DragWidth / 2);
						yt = yroot - (DragHeight / 2);
					}
					width = DragWidth + 2 * JunkBW;
					height = DragHeight + 2 * JunkBW;

					if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
						TryToGrid(tmp_win, &xl, &yt);
					}
					if(!moving_icon && MoveFunction == F_MOVEPUSH && Scr->OpaqueMove) {
						TryToPush(tmp_win, xl, yt, 0);
					}

					if(!moving_icon &&
					                (MoveFunction == F_MOVEPACK || MoveFunction == F_MOVEPUSH)) {
						TryToPack(tmp_win, &xl, &yt);
					}

					if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
						ConstrainByBorders(tmp_win, &xl, width, &yt, height);
					}

					CurrentDragX = xl;
					CurrentDragY = yt;
					if(Scr->OpaqueMove) {
						if(MoveFunction == F_MOVEPUSH && !moving_icon) {
							SetupWindow(tmp_win, xl, yt,
							            tmp_win->frame_width, tmp_win->frame_height, -1);
						}
						else {
							XMoveWindow(dpy, DragWindow, xl, yt);
							if(moving_icon) {
								tmp_win->icon->w_x = xl;
								tmp_win->icon->w_y = yt;
							}
						}
						if(! moving_icon) {
							WMapSetupWindow(tmp_win, xl, yt, -1, -1);
						}
					}
					else {
						MoveOutline(dragroot, xl + Scr->currentvs->x,
						            yt + Scr->currentvs->y, width, height,
						            tmp_win->frame_bw,
						            moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);
					}
				}
				DisplayPosition(tmp_win, CurrentDragX, CurrentDragY);
			}
			XUnmapWindow(dpy, Scr->SizeWindow);

			if(!Scr->OpaqueMove && DragWindow == None) {
				UninstallRootColormap();
			}
			break;
		}

		case F_PRIORITYSWITCHING:
		case F_SWITCHPRIORITY:
		case F_SETPRIORITY:
		case F_CHANGEPRIORITY: {
			WinType wintype;
			int pri;
			char *endp;

			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->icon && w == tmp_win->icon->w) {
				wintype = IconWin;
			}
			else {
				wintype = WinWin;
			}
			switch(func) {
				case F_PRIORITYSWITCHING:
					OtpToggleSwitching(tmp_win, wintype);
					break;
				case F_SETPRIORITY:
					pri = (int)strtol(action, &endp, 10);
					OtpSetPriority(tmp_win, wintype, pri,
					               (*endp == '<' || *endp == 'b') ? Below : Above);
					break;
				case F_CHANGEPRIORITY:
					OtpChangePriority(tmp_win, wintype, atoi(action));
					break;
				case F_SWITCHPRIORITY:
					OtpSwitchPriority(tmp_win, wintype);
					break;
			}
#ifdef EWMH
			EwmhSet_NET_WM_STATE(tmp_win, EWMH_STATE_ABOVE);
#endif /* EWMH */
			/* Update saved priority, if any */
			if(wintype == WinWin && tmp_win->zoomed != ZOOM_NONE) {
				tmp_win->save_otpri = OtpGetPriority(tmp_win);
			}
		}
		break;

		case F_MOVETITLEBAR: {
			Window grabwin;
			int deltax = 0, newx = 0;
			int origNum;
			SqueezeInfo *si;

			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return TRUE;
			}

			PopDownMenu();
			if(tmp_win->squeezed ||
			                !tmp_win->squeeze_info ||
			                !tmp_win->title_w ||
			                context == C_ICON) {
				XBell(dpy, 0);
				break;
			}

			/* If the SqueezeInfo isn't copied yet, do it now */
			if(!tmp_win->squeeze_info_copied) {
				SqueezeInfo *s = malloc(sizeof(SqueezeInfo));
				if(!s) {
					break;
				}
				*s = *tmp_win->squeeze_info;
				tmp_win->squeeze_info = s;
				tmp_win->squeeze_info_copied = 1;
			}
			si = tmp_win->squeeze_info;

			if(si->denom != 0) {
				int target_denom = tmp_win->frame_width;
				/*
				 * If not pixel based, scale the denominator to equal the
				 * window width, so the numerator equals pixels.
				 * That way we can just modify it by pixel units, just
				 * like the other case.
				 */

				if(si->denom != target_denom) {
					float scale = (float)target_denom / si->denom;
					si->num *= scale;
					si->denom = target_denom; /* s->denom *= scale; */
				}
			}

			/* now move the mouse */
			if(tmp_win->winbox) {
				XTranslateCoordinates(dpy, Scr->Root, tmp_win->winbox->window,
				                      eventp->xbutton.x_root, eventp->xbutton.y_root,
				                      &eventp->xbutton.x_root, &eventp->xbutton.y_root, &JunkChild);
			}
			/*
			 * the event is always a button event, since key events
			 * are "weeded out" - although incompletely only
			 * F_MOVE and F_RESIZE - in HandleKeyPress().
			 */
			rootw = eventp->xbutton.root;

			EventHandler[EnterNotify] = HandleUnknown;
			EventHandler[LeaveNotify] = HandleUnknown;

			if(!Scr->NoGrabServer) {
				XGrabServer(dpy);
			}

			grabwin = Scr->Root;
			if(tmp_win->winbox) {
				grabwin = tmp_win->winbox->window;
			}
			XGrabPointer(dpy, grabwin, True,
			             ButtonPressMask | ButtonReleaseMask |
			             ButtonMotionMask | PointerMotionMask, /* PointerMotionHintMask */
			             GrabModeAsync, GrabModeAsync, grabwin, Scr->MoveCursor, CurrentTime);

#if 0   /* what's this for ? */
			if(! tmp_win->icon || w != tmp_win->icon->w) {
				XTranslateCoordinates(dpy, w, tmp_win->frame,
				                      eventp->xbutton.x,
				                      eventp->xbutton.y,
				                      &DragX, &DragY, &JunkChild);

				w = tmp_win->frame;
			}
#endif

			DragWindow = None;

			XGetGeometry(dpy, tmp_win->title_w, &JunkRoot, &origDragX, &origDragY,
			             &DragWidth, &DragHeight, &DragBW,
			             &JunkDepth);

			origX = eventp->xbutton.x_root;
			origNum = si->num;

			if(menuFromFrameOrWindowOrTitlebar) {
				/* warp the pointer to the middle of the window */
				XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
				             origDragX + DragWidth / 2,
				             origDragY + DragHeight / 2);
				XFlush(dpy);
			}

			while(TRUE) {
				long releaseEvent = menuFromFrameOrWindowOrTitlebar ?
				                    ButtonPress : ButtonRelease;
				long movementMask = menuFromFrameOrWindowOrTitlebar ?
				                    PointerMotionMask : ButtonMotionMask;

				/* block until there is an interesting event */
				XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
				           EnterWindowMask | LeaveWindowMask |
				           ExposureMask | movementMask |
				           VisibilityChangeMask, &Event);

				/* throw away enter and leave events until release */
				if(Event.xany.type == EnterNotify ||
				                Event.xany.type == LeaveNotify) {
					continue;
				}

				if(Event.type == MotionNotify) {
					/* discard any extra motion events before a logical release */
					while(XCheckMaskEvent(dpy,
					                      movementMask | releaseEvent, &Event)) {
						if(Event.type == releaseEvent) {
							break;
						}
					}
				}

				if(!DispatchEvent2()) {
					continue;
				}

				if(Event.type == releaseEvent) {
					break;
				}

				/* something left to do only if the pointer moved */
				if(Event.type != MotionNotify) {
					continue;
				}

				/* get current pointer pos, useful when there is lag */
				XQueryPointer(dpy, rootw, &eventp->xmotion.root, &JunkChild,
				              &eventp->xmotion.x_root, &eventp->xmotion.y_root,
				              &JunkX, &JunkY, &JunkMask);

				FixRootEvent(eventp);
				if(tmp_win->winbox) {
					XTranslateCoordinates(dpy, Scr->Root, tmp_win->winbox->window,
					                      eventp->xmotion.x_root, eventp->xmotion.y_root,
					                      &eventp->xmotion.x_root, &eventp->xmotion.y_root, &JunkChild);
				}

				if(!Scr->NoRaiseMove && Scr->OpaqueMove && !WindowMoved) {
					OtpRaise(tmp_win, WinWin);
				}

				deltax = eventp->xmotion.x_root - origX;
				newx = origNum + deltax;

				/*
				 * Clamp to left and right.
				 * If we're in pixel size, keep within [ 0, frame_width >.
				 * If we're proportional, don't cross the 0.
				 * Also don't let the nominator get bigger than the denominator.
				 * Keep within [ -denom, -1] or [ 0, denom >.
				 */
				{
					int wtmp = tmp_win->frame_width; /* or si->denom; if it were != 0 */
					if(origNum < 0) {
						if(newx >= 0) {
							newx = -1;
						}
						else if(newx < -wtmp) {
							newx = -wtmp;
						}
					}
					else if(origNum >= 0) {
						if(newx < 0) {
							newx = 0;
						}
						else if(newx >= wtmp) {
							newx = wtmp - 1;
						}
					}
				}

				si->num = newx;
				/* This, finally, actually moves the title bar */
				/* XXX pressing a second button should cancel and undo this */
				SetFrameShape(tmp_win);
			}
			break;
		}
		case F_FUNCTION: {
			MenuRoot *mroot;
			MenuItem *mitem;

			if((mroot = FindMenuRoot(action)) == NULL) {
				if(!action) {
					action = "undef";
				}
				fprintf(stderr, "%s: couldn't find function \"%s\"\n",
				        ProgramName, (char *)action);
				return TRUE;
			}

			if(NeedToDefer(mroot) && DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			else {
				for(mitem = mroot->first; mitem != NULL; mitem = mitem->next) {
					if(!ExecuteFunction(mitem->func, mitem->action, w,
					                    tmp_win, eventp, context, pulldown))
						/* pebl FIXME: the focus should be updated here,
						 or the function would operate on the same window */
					{
						break;
					}
				}
			}
		}
		break;

		case F_DEICONIFY:
		case F_ICONIFY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->isicon) {
				DeIconify(tmp_win);
			}
			else if(func == F_ICONIFY) {
				Iconify(tmp_win, eventp->xbutton.x_root - 5,
				        eventp->xbutton.y_root - 5);
			}
			break;

		case F_SQUEEZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			Squeeze(tmp_win);
			break;

		case F_UNSQUEEZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->squeezed) {
				Squeeze(tmp_win);
			}
			break;

		case F_SHOWBGRD:
			ShowBackground(Scr->currentvs, -1);
			break;

		case F_RAISELOWER:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(!WindowMoved) {
				if(tmp_win->icon && w == tmp_win->icon->w) {
					OtpRaiseLower(tmp_win, IconWin);
				}
				else {
					OtpRaiseLower(tmp_win, WinWin);
					WMapRaiseLower(tmp_win);
				}
			}
			break;

		case F_TINYRAISE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			/* check to make sure raise is not from the WindowFunction */
			if(tmp_win->icon && (w == tmp_win->icon->w) && Context != C_ROOT) {
				OtpTinyRaise(tmp_win, IconWin);
			}
			else {
				OtpTinyRaise(tmp_win, WinWin);
				WMapRaise(tmp_win);
			}
			break;

		case F_TINYLOWER:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			/* check to make sure raise is not from the WindowFunction */
			if(tmp_win->icon && (w == tmp_win->icon->w) && Context != C_ROOT) {
				OtpTinyLower(tmp_win, IconWin);
			}
			else {
				OtpTinyLower(tmp_win, WinWin);
				WMapRaise(tmp_win);
			}
			break;

		case F_RAISEORSQUEEZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			/* FIXME using the same double-click ConstrainedMoveTime here */
			if((eventp->xbutton.time - last_time) < ConstrainedMoveTime) {
				Squeeze(tmp_win);
				break;
			}
			last_time = eventp->xbutton.time;
		/* intentional fall-thru into F_RAISE */

		case F_RAISE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			/* check to make sure raise is not from the WindowFunction */
			if(tmp_win->icon && (w == tmp_win->icon->w) && Context != C_ROOT)  {
				OtpRaise(tmp_win, IconWin);
			}
			else {
				OtpRaise(tmp_win, WinWin);
				WMapRaise(tmp_win);
			}
			break;

		case F_LOWER:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->icon && (w == tmp_win->icon->w)) {
				OtpLower(tmp_win, IconWin);
			}
			else {
				OtpLower(tmp_win, WinWin);
				WMapLower(tmp_win);
			}
			break;

		case F_RAISEICONS:
			for(t = Scr->FirstWindow; t != NULL; t = t->next) {
				if(t->icon && t->icon->w) {
					OtpRaise(t, IconWin);
				}
			}
			break;

		case F_FOCUS:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->isicon == FALSE) {
				if(!Scr->FocusRoot && Scr->Focus == tmp_win) {
					FocusOnRoot();
				}
				else {
					InstallWindowColormaps(0, tmp_win);
					SetFocus(tmp_win, eventp->xbutton.time);
					Scr->FocusRoot = FALSE;
				}
			}
			break;

		case F_DESTROY:
			if(DeferExecution(context, func, Scr->DestroyCursor)) {
				return TRUE;
			}

			if(tmp_win->iconmgr || tmp_win->iswinbox || tmp_win->wspmgr
			                || (Scr->workSpaceMgr.occupyWindow
			                    && tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win)) {
				XBell(dpy, 0);
				break;
			}
			XKillClient(dpy, tmp_win->w);
			if(ButtonPressed != -1) {
				XEvent kev;

				XMaskEvent(dpy, ButtonReleaseMask, &kev);
				if(kev.xbutton.window == tmp_win->w) {
					kev.xbutton.window = Scr->Root;
				}
				XPutBackEvent(dpy, &kev);
			}
			break;

		case F_DELETE:
			if(DeferExecution(context, func, Scr->DestroyCursor)) {
				return TRUE;
			}

			if(tmp_win->iconmgr) {          /* don't send ourself a message */
				HideIconManager();
				break;
			}
			if(tmp_win->iswinbox || tmp_win->wspmgr
			                || (Scr->workSpaceMgr.occupyWindow
			                    && tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win)) {
				XBell(dpy, 0);
				break;
			}
			if(tmp_win->protocols & DoesWmDeleteWindow) {
				SendDeleteWindowMessage(tmp_win, LastTimestamp());
				if(ButtonPressed != -1) {
					XEvent kev;

					XMaskEvent(dpy, ButtonReleaseMask, &kev);
					if(kev.xbutton.window == tmp_win->w) {
						kev.xbutton.window = Scr->Root;
					}
					XPutBackEvent(dpy, &kev);
				}
				break;
			}
			XBell(dpy, 0);
			break;

		case F_DELETEORDESTROY:
			if(DeferExecution(context, func, Scr->DestroyCursor)) {
				return TRUE;
			}

			if(tmp_win->iconmgr) {
				HideIconManager();
				break;
			}
			if(tmp_win->iswinbox || tmp_win->wspmgr
			                || (Scr->workSpaceMgr.occupyWindow
			                    && tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win)) {
				XBell(dpy, 0);
				break;
			}
			if(tmp_win->protocols & DoesWmDeleteWindow) {
				SendDeleteWindowMessage(tmp_win, LastTimestamp());
			}
			else {
				XKillClient(dpy, tmp_win->w);
			}
			if(ButtonPressed != -1) {
				XEvent kev;

				XMaskEvent(dpy, ButtonReleaseMask, &kev);
				if(kev.xbutton.window == tmp_win->w) {
					kev.xbutton.window = Scr->Root;
				}
				XPutBackEvent(dpy, &kev);
			}
			break;

		case F_SAVEYOURSELF:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(tmp_win->protocols & DoesWmSaveYourself) {
				SendSaveYourselfMessage(tmp_win, LastTimestamp());
			}
			else {
				XBell(dpy, 0);
			}
			break;

		case F_CIRCLEUP:
			OtpCirculateSubwindows(Scr->currentvs, RaiseLowest);
			break;

		case F_CIRCLEDOWN:
			OtpCirculateSubwindows(Scr->currentvs, LowerHighest);
			break;

		case F_EXEC:
			PopDownMenu();
			if(!Scr->NoGrabServer) {
				XUngrabServer(dpy);
				XSync(dpy, 0);
			}
			XUngrabPointer(dpy, CurrentTime);
			XSync(dpy, 0);
			Execute(action);
			break;

		case F_UNFOCUS:
			FocusOnRoot();
			break;

		case F_WARPTOSCREEN: {
			if(strcmp(action, WARPSCREEN_NEXT) == 0) {
				WarpToScreen(Scr->screen + 1, 1);
			}
			else if(strcmp(action, WARPSCREEN_PREV) == 0) {
				WarpToScreen(Scr->screen - 1, -1);
			}
			else if(strcmp(action, WARPSCREEN_BACK) == 0) {
				WarpToScreen(PreviousScreen, 0);
			}
			else {
				WarpToScreen(atoi(action), 0);
			}
		}
		break;

		case F_COLORMAP: {
			if(strcmp(action, COLORMAP_NEXT) == 0) {
				BumpWindowColormap(tmp_win, 1);
			}
			else if(strcmp(action, COLORMAP_PREV) == 0) {
				BumpWindowColormap(tmp_win, -1);
			}
			else {
				BumpWindowColormap(tmp_win, 0);
			}
		}
		break;

		case F_WARPTO: {
			TwmWindow *tw;
			int len;

			len = strlen(action);

#ifdef WARPTO_FROM_ICONMGR
			if(len == 0 && tmp_win && tmp_win->iconmgr) {
				printf("curren iconmgr entry: %s", tmp_win->iconmgr->Current);
			}
#endif /* #ifdef WARPTO_FROM_ICONMGR */
			for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
				if(!strncmp(action, tw->full_name, len)) {
					break;
				}
				if(match(action, tw->full_name)) {
					break;
				}
			}
			if(!tw) {
				for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
					if(!strncmp(action, tw->class.res_name, len)) {
						break;
					}
					if(match(action, tw->class.res_name)) {
						break;
					}
				}
				if(!tw) {
					for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
						if(!strncmp(action, tw->class.res_class, len)) {
							break;
						}
						if(match(action, tw->class.res_class)) {
							break;
						}
					}
				}
			}

			if(tw) {
				if(Scr->WarpUnmapped || tw->mapped) {
					if(!tw->mapped) {
						DeIconify(tw);
					}
					WarpToWindow(tw, Scr->RaiseOnWarp);
				}
			}
			else {
				XBell(dpy, 0);
			}
		}
		break;

		case F_WARPTOICONMGR: {
			TwmWindow *tw, *raisewin = NULL;
			int len;
			Window iconwin = None;

			len = strlen(action);
			if(len == 0) {
				if(tmp_win && tmp_win->iconmanagerlist) {
					raisewin = tmp_win->iconmanagerlist->iconmgr->twm_win;
					iconwin = tmp_win->iconmanagerlist->icon;
				}
				else if(Scr->iconmgr->active) {
					raisewin = Scr->iconmgr->twm_win;
					iconwin = Scr->iconmgr->active->w;
				}
			}
			else {
				for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
					if(strncmp(action, tw->icon_name, len) == 0) {
						if(tw->iconmanagerlist &&
						                tw->iconmanagerlist->iconmgr->twm_win->mapped) {
							raisewin = tw->iconmanagerlist->iconmgr->twm_win;
							break;
						}
					}
				}
			}

			if(raisewin) {
				OtpRaise(raisewin, WinWin);
				XWarpPointer(dpy, None, iconwin, 0, 0, 0, 0, 5, 5);
			}
			else {
				XBell(dpy, 0);
			}
		}
		break;

		case F_RING:  /* Taken from vtwm version 5.3 */
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			if(tmp_win->ring.next || tmp_win->ring.prev) {
				/* It's in the ring, let's take it out. */
				TwmWindow *prev = tmp_win->ring.prev, *next = tmp_win->ring.next;

				/*
				* 1. Unlink window
				* 2. If window was only thing in ring, null out ring
				* 3. If window was ring leader, set to next (or null)
				*/
				if(prev) {
					prev->ring.next = next;
				}
				if(next) {
					next->ring.prev = prev;
				}
				if(Scr->Ring == tmp_win) {
					Scr->Ring = (next != tmp_win ? next : (TwmWindow *) NULL);
				}

				if(!Scr->Ring || Scr->RingLeader == tmp_win) {
					Scr->RingLeader = Scr->Ring;
				}
				tmp_win->ring.next = tmp_win->ring.prev = NULL;
			}
			else {
				/* Not in the ring, so put it in. */
				if(Scr->Ring) {
					tmp_win->ring.next = Scr->Ring->ring.next;
					if(Scr->Ring->ring.next->ring.prev) {
						Scr->Ring->ring.next->ring.prev = tmp_win;
					}
					Scr->Ring->ring.next = tmp_win;
					tmp_win->ring.prev = Scr->Ring;
				}
				else {
					tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
				}
			}
			/*tmp_win->ring.cursor_valid = False;*/
			break;

		case F_WARPRING:
			switch(((char *)action)[0]) {
				case 'n':
					WarpAlongRing(&eventp->xbutton, True);
					break;
				case 'p':
					WarpAlongRing(&eventp->xbutton, False);
					break;
				default:
					XBell(dpy, 0);
					break;
			}
			break;

		case F_REFRESH: {
			XSetWindowAttributes attributes;
			unsigned long valuemask;

			valuemask = (CWBackPixel | CWBackingStore | CWSaveUnder);
			attributes.background_pixel = Scr->Black;
			attributes.backing_store = NotUseful;
			attributes.save_under = False;
			w = XCreateWindow(dpy, Scr->Root, 0, 0,
			                  (unsigned int) Scr->rootw,
			                  (unsigned int) Scr->rooth,
			                  (unsigned int) 0,
			                  CopyFromParent, (unsigned int) CopyFromParent,
			                  (Visual *) CopyFromParent, valuemask,
			                  &attributes);
			XMapWindow(dpy, w);
			XDestroyWindow(dpy, w);
			XFlush(dpy);
		}
		break;

		case F_OCCUPY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			Occupy(tmp_win);
			break;

		case F_OCCUPYALL:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}
			OccupyAll(tmp_win);
			break;

		case F_GOTOWORKSPACE:
			GotoWorkSpaceByName(Scr->currentvs, action);
			break;

		case F_PREVWORKSPACE:
			GotoPrevWorkSpace(Scr->currentvs);
			break;

		case F_NEXTWORKSPACE:
			GotoNextWorkSpace(Scr->currentvs);
			break;

		case F_RIGHTWORKSPACE:
			GotoRightWorkSpace(Scr->currentvs);
			break;

		case F_LEFTWORKSPACE:
			GotoLeftWorkSpace(Scr->currentvs);
			break;

		case F_UPWORKSPACE:
			GotoUpWorkSpace(Scr->currentvs);
			break;

		case F_DOWNWORKSPACE:
			GotoDownWorkSpace(Scr->currentvs);
			break;

		case F_MENU:
			if(action && ! strncmp(action, "WGOTO : ", 8)) {
				GotoWorkSpaceByName(/* XXXXX */ Scr->currentvs,
				                                ((char *)action) + 8);
			}
			else {
				MenuItem *item;

				item = ActiveItem;
				while(item && item->sub) {
					if(!item->sub->defaultitem) {
						break;
					}
					if(item->sub->defaultitem->func != F_MENU) {
						break;
					}
					item = item->sub->defaultitem;
				}
				if(item && item->sub && item->sub->defaultitem) {
					ExecuteFunction(item->sub->defaultitem->func,
					                item->sub->defaultitem->action,
					                w, tmp_win, eventp, context, pulldown);
				}
			}
			break;

		case F_WINREFRESH:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			if(context == C_ICON && tmp_win->icon && tmp_win->icon->w)
				w = XCreateSimpleWindow(dpy, tmp_win->icon->w,
				                        0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);
			else
				w = XCreateSimpleWindow(dpy, tmp_win->frame,
				                        0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);

			XMapWindow(dpy, w);
			XDestroyWindow(dpy, w);
			XFlush(dpy);
			break;

		case F_ADOPTWINDOW:
			AdoptWindow();
			break;

		case F_TRACE:
			DebugTrace(action);
			break;

		case F_CHANGESIZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return TRUE;
			}

			ChangeSize(action, tmp_win);
			break;

		case F_QUIT:
			Done(0);
			break;

		case F_RESCUE_WINDOWS:
			RescueWindows();
			break;

	}

	if(ButtonPressed == -1) {
		XUngrabPointer(dpy, CurrentTime);
	}
	return do_next_action;
}



/*
 * Utils
 */
static void
jump(TwmWindow *tmp_win, int  direction, char *action)
{
	int                 fx, fy, px, py, step, status, cons;
	int                 fwidth, fheight;
	int                 junkX, junkY;
	unsigned int        junkK;
	Window              junkW;

	if(! action) {
		return;
	}
	status = sscanf(action, "%d", &step);
	if(status != 1) {
		return;
	}
	if(step < 1) {
		return;
	}

	fx = tmp_win->frame_x;
	fy = tmp_win->frame_y;
	XQueryPointer(dpy, Scr->Root, &junkW, &junkW, &junkX, &junkY, &px, &py, &junkK);
	px -= fx;
	py -= fy;

	fwidth  = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	fheight = tmp_win->frame_height + 2 * tmp_win->frame_bw;
	switch(direction) {
		case J_LEFT   :
			cons  = FindConstraint(tmp_win, J_LEFT);
			if(cons == -1) {
				return;
			}
			fx -= step * Scr->XMoveGrid;
			if(fx < cons) {
				fx = cons;
			}
			break;
		case J_RIGHT  :
			cons  = FindConstraint(tmp_win, J_RIGHT);
			if(cons == -1) {
				return;
			}
			fx += step * Scr->XMoveGrid;
			if(fx + fwidth > cons) {
				fx = cons - fwidth;
			}
			break;
		case J_TOP    :
			cons  = FindConstraint(tmp_win, J_TOP);
			if(cons == -1) {
				return;
			}
			fy -= step * Scr->YMoveGrid;
			if(fy < cons) {
				fy = cons;
			}
			break;
		case J_BOTTOM :
			cons  = FindConstraint(tmp_win, J_BOTTOM);
			if(cons == -1) {
				return;
			}
			fy += step * Scr->YMoveGrid;
			if(fy + fheight > cons) {
				fy = cons - fheight;
			}
			break;
	}
	/* Pebl Fixme: don't warp if jump happens through iconmgr */
	XWarpPointer(dpy, Scr->Root, Scr->Root, 0, 0, 0, 0, fx + px, fy + py);
	if(!Scr->NoRaiseMove) {
		OtpRaise(tmp_win, WinWin);
	}
	SetupWindow(tmp_win, fx, fy, tmp_win->frame_width, tmp_win->frame_height, -1);
}


static void
ShowIconManager(void)
{
	IconMgr   *i;
	WorkSpace *wl;

	if(! Scr->workSpaceManagerActive) {
		return;
	}

	if(Scr->NoIconManagers) {
		return;
	}
	for(wl = Scr->workSpaceMgr.workSpaceList; wl != NULL; wl = wl->next) {
		for(i = wl->iconmgr; i != NULL; i = i->next) {
			if(i->count == 0) {
				continue;
			}
			if(visible(i->twm_win)) {
				SetMapStateProp(i->twm_win, NormalState);
				XMapWindow(dpy, i->twm_win->w);
				OtpRaise(i->twm_win, WinWin);
				XMapWindow(dpy, i->twm_win->frame);
				if(i->twm_win->icon && i->twm_win->icon->w) {
					XUnmapWindow(dpy, i->twm_win->icon->w);
				}
			}
			i->twm_win->mapped = TRUE;
			i->twm_win->isicon = FALSE;
		}
	}
}


static void
HideIconManager(void)
{
	IconMgr   *i;
	WorkSpace *wl;

	if(Scr->NoIconManagers) {
		return;
	}
	for(wl = Scr->workSpaceMgr.workSpaceList; wl != NULL; wl = wl->next) {
		for(i = wl->iconmgr; i != NULL; i = i->next) {
			SetMapStateProp(i->twm_win, WithdrawnState);
			XUnmapWindow(dpy, i->twm_win->frame);
			if(i->twm_win->icon && i->twm_win->icon->w) {
				XUnmapWindow(dpy, i->twm_win->icon->w);
			}
			i->twm_win->mapped = FALSE;
			i->twm_win->isicon = TRUE;
		}
	}
}


/***********************************************************************
 *
 *  Procedure:
 *      DeferExecution - defer the execution of a function to the
 *          next button press if the context is C_ROOT
 *
 *  Inputs:
 *      context - the context in which the mouse button was pressed
 *      func    - the function to defer
 *      cursor  - the cursor to display while waiting
 *
 ***********************************************************************
 */

static int
DeferExecution(int context, int func, Cursor cursor)
{
	if((context == C_ROOT) || (context == C_ALTERNATE)) {
		SetLastCursor(cursor);
		XGrabPointer(dpy,
		             Scr->Root,
		             True,
		             ButtonPressMask | ButtonReleaseMask,
		             GrabModeAsync,
		             GrabModeAsync,
		             (func == F_ADOPTWINDOW) ? None : Scr->Root,
		             cursor,
		             CurrentTime);
		RootFunction = func;

		return (TRUE);
	}

	return (FALSE);
}



/*
 * Backend for f.identify and f.version: Fills in the Info array with the
 * appropriate bits for ctwm and the window specified (if any), and
 * sizes/pops up the InfoWindow.
 *
 * Notably, the bits of Info aren't written into the window during this
 * process; that happens later as a result of the expose event.
 */
static void
Identify(TwmWindow *t)
{
	int i, n, twidth, width, height;
	int x, y;
	unsigned int wwidth, wheight, bw, depth;
	Window junk;
	int px, py, dummy;
	unsigned udummy;
	unsigned char *prop;
	unsigned long nitems, bytesafter;
	Atom actual_type;
	int actual_format;
	XRectangle inc_rect;
	XRectangle logical_rect;
	char *ctopts;

	/*
	 * Include some checking we don't blow out _LINES.  We use snprintf()
	 * exclusively to avoid blowing out _SIZE.
	 *
	 * In an ideal world, we'd probably fix this to be more dynamically
	 * allocated, but this will do for now.
	 */
	n = 0;
#define CHKN do { \
                if(n > (INFO_LINES - 3)) { \
                        fprintf(stderr, "Overflowing Info[] on line %d\n", n); \
                        sprintf(Info[n++], "(overflow)"); \
                        goto info_dismiss; \
                } \
        } while(0)

	snprintf(Info[n++], INFO_SIZE, "Twm version:  %s", TwmVersion);
	CHKN;
	if(VCSRevision) {
		snprintf(Info[n++], INFO_SIZE, "VCS Revision:  %s", VCSRevision);
		CHKN;
	}

	ctopts = ctopts_string(", ");
	snprintf(Info[n++], INFO_SIZE, "Compile time options : %s", ctopts);
	free(ctopts);
	CHKN;

	Info[n++][0] = '\0';
	CHKN;

	if(t) {
		XGetGeometry(dpy, t->w, &JunkRoot, &JunkX, &JunkY,
		             &wwidth, &wheight, &bw, &depth);
		XTranslateCoordinates(dpy, t->w, Scr->Root, 0, 0,
		                      &x, &y, &junk);
		snprintf(Info[n++], INFO_SIZE, "Name               = \"%s\"",
		         t->full_name);
		CHKN;
		snprintf(Info[n++], INFO_SIZE, "Class.res_name     = \"%s\"",
		         t->class.res_name);
		CHKN;
		snprintf(Info[n++], INFO_SIZE, "Class.res_class    = \"%s\"",
		         t->class.res_class);
		CHKN;
		Info[n++][0] = '\0';
		CHKN;
		snprintf(Info[n++], INFO_SIZE,
		         "Geometry/root (UL) = %dx%d+%d+%d (Inner: %dx%d+%d+%d)",
		         wwidth + 2 * (bw + t->frame_bw3D),
		         wheight + 2 * (bw + t->frame_bw3D) + t->title_height,
		         x - (bw + t->frame_bw3D),
		         y - (bw + t->frame_bw3D + t->title_height),
		         wwidth, wheight, x, y);
		CHKN;
		snprintf(Info[n++], INFO_SIZE,
		         "Geometry/root (LR) = %dx%d-%d-%d (Inner: %dx%d-%d-%d)",
		         wwidth + 2 * (bw + t->frame_bw3D),
		         wheight + 2 * (bw + t->frame_bw3D) + t->title_height,
		         Scr->rootw - (x + wwidth + bw + t->frame_bw3D),
		         Scr->rooth - (y + wheight + bw + t->frame_bw3D),
		         wwidth, wheight,
		         Scr->rootw - (x + wwidth), Scr->rooth - (y + wheight));
		CHKN;
		snprintf(Info[n++], INFO_SIZE, "Border width       = %d", bw);
		CHKN;
		snprintf(Info[n++], INFO_SIZE, "3D border width    = %d", t->frame_bw3D);
		CHKN;
		snprintf(Info[n++], INFO_SIZE, "Depth              = %d", depth);
		CHKN;
		if(t->vs &&
		                t->vs->wsw &&
		                t->vs->wsw->currentwspc) {
			snprintf(Info[n++], INFO_SIZE, "Virtual Workspace  = %s",
			         t->vs->wsw->currentwspc->name);
			CHKN;
		}
		snprintf(Info[n++], INFO_SIZE, "OnTopPriority      = %d",
		         OtpGetPriority(t));
		CHKN;

		if(t->icon != NULL) {
			XGetGeometry(dpy, t->icon->w, &JunkRoot, &JunkX, &JunkY,
			             &wwidth, &wheight, &bw, &depth);
			Info[n++][0] = '\0';
			CHKN;
			snprintf(Info[n++], INFO_SIZE, "IconGeom/root     = %dx%d+%d+%d",
			         wwidth, wheight, JunkX, JunkY);
			CHKN;
			snprintf(Info[n++], INFO_SIZE, "IconGeom/intern   = %dx%d+%d+%d",
			         t->icon->w_width, t->icon->w_height,
			         t->icon->w_x, t->icon->w_y);
			CHKN;
			snprintf(Info[n++], INFO_SIZE, "IconBorder width  = %d", bw);
			CHKN;
			snprintf(Info[n++], INFO_SIZE, "IconDepth         = %d", depth);
			CHKN;
		}

		if(XGetWindowProperty(dpy, t->w, XA_WM_CLIENT_MACHINE, 0L, 64, False,
		                      XA_STRING, &actual_type, &actual_format, &nitems,
		                      &bytesafter, &prop) == Success) {
			if(nitems && prop) {
				snprintf(Info[n++], INFO_SIZE, "Client machine     = %s",
				         (char *)prop);
				XFree(prop);
				CHKN;
			}
		}
		Info[n++][0] = '\0';
		CHKN;
	}

#undef CHKN
info_dismiss:
	snprintf(Info[n++], INFO_SIZE, "Click to dismiss....");


	/*
	 * OK, it's all built now.
	 */

	/* figure out the width and height of the info window */
	height = n * (Scr->DefaultFont.height + 2);
	width = 1;
	for(i = 0; i < n; i++) {
		XmbTextExtents(Scr->DefaultFont.font_set, Info[i],
		               strlen(Info[i]), &inc_rect, &logical_rect);

		twidth = logical_rect.width;
		if(twidth > width) {
			width = twidth;
		}
	}

	/*
	 * If InfoLines is set, that means it was mapped, so hide it away.
	 * Sorta odd way of flagging it, but...
	 */
	if(InfoLines) {
		XUnmapWindow(dpy, Scr->InfoWindow);
	}

	/* Stash the new number of lines */
	InfoLines = n;

	width += 10;                /* some padding */
	height += 10;               /* some padding */
	if(XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
	                 &dummy, &dummy, &px, &py, &udummy)) {
		px -= (width / 2);
		py -= (height / 3);
		if(px + width + BW2 >= Scr->rootw) {
			px = Scr->rootw - width - BW2;
		}
		if(py + height + BW2 >= Scr->rooth) {
			py = Scr->rooth - height - BW2;
		}
		if(px < 0) {
			px = 0;
		}
		if(py < 0) {
			py = 0;
		}
	}
	else {
		px = py = 0;
	}
	XMoveResizeWindow(dpy, Scr->InfoWindow, px, py, width, height);
	XMapRaised(dpy, Scr->InfoWindow);
	InfoWidth  = width;
	InfoHeight = height;
}


static bool
belongs_to_twm_window(TwmWindow *t, Window w)
{
	if(!t) {
		return false;
	}

	if(w == t->frame || w == t->title_w || w == t->hilite_wl || w == t->hilite_wr ||
	                (t->icon && (w == t->icon->w || w == t->icon->bm_w))) {
		return true;
	}

	if(t && t->titlebuttons) {
		TBWindow *tbw;
		int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
		for(tbw = t->titlebuttons; nb > 0; tbw++, nb--) {
			if(tbw->window == w) {
				return true;
			}
		}
	}
	return false;
}


static void
packwindow(TwmWindow *tmp_win, char *direction)
{
	int                 cons, newx, newy;
	int                 x, y, px, py, junkX, junkY;
	unsigned int        junkK;
	Window              junkW;

	if(!strcmp(direction,   "left")) {
		cons  = FindConstraint(tmp_win, J_LEFT);
		if(cons == -1) {
			return;
		}
		newx  = cons;
		newy  = tmp_win->frame_y;
	}
	else if(!strcmp(direction,  "right")) {
		cons  = FindConstraint(tmp_win, J_RIGHT);
		if(cons == -1) {
			return;
		}
		newx  = cons;
		newx -= tmp_win->frame_width  + 2 * tmp_win->frame_bw;
		newy  = tmp_win->frame_y;
	}
	else if(!strcmp(direction,    "top")) {
		cons  = FindConstraint(tmp_win, J_TOP);
		if(cons == -1) {
			return;
		}
		newx  = tmp_win->frame_x;
		newy  = cons;
	}
	else if(!strcmp(direction, "bottom")) {
		cons  = FindConstraint(tmp_win, J_BOTTOM);
		if(cons == -1) {
			return;
		}
		newx  = tmp_win->frame_x;
		newy  = cons;
		newy -= tmp_win->frame_height  + 2 * tmp_win->frame_bw;
	}
	else {
		return;
	}

	XQueryPointer(dpy, Scr->Root, &junkW, &junkW, &junkX, &junkY, &x, &y, &junkK);
	px = x - tmp_win->frame_x + newx;
	py = y - tmp_win->frame_y + newy;
	XWarpPointer(dpy, Scr->Root, Scr->Root, 0, 0, 0, 0, px, py);
	OtpRaise(tmp_win, WinWin);
	XMoveWindow(dpy, tmp_win->frame, newx, newy);
	SetupWindow(tmp_win, newx, newy, tmp_win->frame_width, tmp_win->frame_height,
	            -1);
}


static void
fillwindow(TwmWindow *tmp_win, char *direction)
{
	int cons, newx, newy, save;
	unsigned int neww, newh;
	int i;
	int winx = tmp_win->frame_x;
	int winy = tmp_win->frame_y;
	int winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	int winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;

	if(!strcmp(direction, "left")) {
		cons = FindConstraint(tmp_win, J_LEFT);
		if(cons == -1) {
			return;
		}
		newx = cons;
		newy = tmp_win->frame_y;
		neww = winw + winx - newx;
		newh = winh;
		neww -= 2 * tmp_win->frame_bw;
		newh -= 2 * tmp_win->frame_bw;
		ConstrainSize(tmp_win, &neww, &newh);
	}
	else if(!strcmp(direction, "right")) {
		for(i = 0; i < 2; i++) {
			cons = FindConstraint(tmp_win, J_RIGHT);
			if(cons == -1) {
				return;
			}
			newx = tmp_win->frame_x;
			newy = tmp_win->frame_y;
			neww = cons - winx;
			newh = winh;
			save = neww;
			neww -= 2 * tmp_win->frame_bw;
			newh -= 2 * tmp_win->frame_bw;
			ConstrainSize(tmp_win, &neww, &newh);
			if((neww != winw) || (newh != winh) ||
			                (cons == Scr->rootw - Scr->BorderRight)) {
				break;
			}
			neww = save;
			SetupWindow(tmp_win, newx, newy, neww, newh, -1);
		}
	}
	else if(!strcmp(direction, "top")) {
		cons = FindConstraint(tmp_win, J_TOP);
		if(cons == -1) {
			return;
		}
		newx = tmp_win->frame_x;
		newy = cons;
		neww = winw;
		newh = winh + winy - newy;
		neww -= 2 * tmp_win->frame_bw;
		newh -= 2 * tmp_win->frame_bw;
		ConstrainSize(tmp_win, &neww, &newh);
	}
	else if(!strcmp(direction, "bottom")) {
		for(i = 0; i < 2; i++) {
			cons = FindConstraint(tmp_win, J_BOTTOM);
			if(cons == -1) {
				return;
			}
			newx = tmp_win->frame_x;
			newy = tmp_win->frame_y;
			neww = winw;
			newh = cons - winy;
			save = newh;
			neww -= 2 * tmp_win->frame_bw;
			newh -= 2 * tmp_win->frame_bw;
			ConstrainSize(tmp_win, &neww, &newh);
			if((neww != winw) || (newh != winh) ||
			                (cons == Scr->rooth - Scr->BorderBottom)) {
				break;
			}
			newh = save;
			SetupWindow(tmp_win, newx, newy, neww, newh, -1);
		}
	}
	else if(!strcmp(direction, "vertical")) {
		if(tmp_win->zoomed == ZOOM_NONE) {
			tmp_win->save_frame_height = tmp_win->frame_height;
			tmp_win->save_frame_width = tmp_win->frame_width;
			tmp_win->save_frame_y = tmp_win->frame_y;
			tmp_win->save_frame_x = tmp_win->frame_x;
			tmp_win->save_otpri = OtpGetPriority(tmp_win);

			tmp_win->frame_y++;
			newy = FindConstraint(tmp_win, J_TOP);
			tmp_win->frame_y--;
			newh = FindConstraint(tmp_win, J_BOTTOM) - newy;
			newh -= 2 * tmp_win->frame_bw;

			newx = tmp_win->frame_x;
			neww = tmp_win->frame_width;

			ConstrainSize(tmp_win, &neww, &newh);

			/* if the bottom of the window has moved up
			 * it will be pushed down */
			if(newy + newh <
			                tmp_win->save_frame_y + tmp_win->save_frame_height)
				newy = tmp_win->save_frame_y +
				       tmp_win->save_frame_height - newh;
			tmp_win->zoomed = F_VERTZOOM;
			SetupWindow(tmp_win, newx, newy, neww, newh, -1);
		}
		else {
			fullzoom(tmp_win, tmp_win->zoomed);
		}
		return;
	}
	else {
		return;
	}
	SetupWindow(tmp_win, newx, newy, neww, newh, -1);
}


/***********************************************************************
 *
 *  Procedure:
 *      NeedToDefer - checks each function in the list to see if it
 *              is one that needs to be defered.
 *
 *  Inputs:
 *      root    - the menu root to check
 *
 ***********************************************************************
 */
static int
NeedToDefer(MenuRoot *root)
{
	MenuItem *mitem;

	for(mitem = root->first; mitem != NULL; mitem = mitem->next) {
		switch(mitem->func) {
			case F_IDENTIFY:
			case F_RESIZE:
			case F_MOVE:
			case F_FORCEMOVE:
			case F_DEICONIFY:
			case F_ICONIFY:
			case F_RAISELOWER:
			case F_RAISE:
			case F_LOWER:
			case F_FOCUS:
			case F_DESTROY:
			case F_WINREFRESH:
			case F_VERTZOOM:
			case F_FULLZOOM:
			case F_FULLSCREENZOOM:
			case F_HORIZOOM:
			case F_RIGHTZOOM:
			case F_LEFTZOOM:
			case F_TOPZOOM:
			case F_BOTTOMZOOM:
			case F_SQUEEZE:
			case F_AUTORAISE:
			case F_AUTOLOWER:
			case F_CHANGESIZE:
				return TRUE;
		}
	}
	return FALSE;
}



/***********************************************************************
 *
 *  Procedure:
 *      Execute - execute the string by /bin/sh
 *
 *  Inputs:
 *      s       - the string containing the command
 *
 ***********************************************************************
 */
static void
Execute(const char *_s)
{
	char *s;
	char *_ds;
	char *orig_display;
	int restorevar = 0;
	char *subs;

	/* Seatbelt */
	if(!_s) {
		return;
	}

	/* Work on a local copy since we're mutating it */
	s = strdup(_s);
	if(!s) {
		return;
	}

	/* Stash up current $DISPLAY value for resetting */
	orig_display = getenv("DISPLAY");


	/*
	 * Build a display string using the current screen number, so that
	 * X programs which get fired up from a menu come up on the screen
	 * that they were invoked from, unless specifically overridden on
	 * their command line.
	 *
	 * Which is to say, given that we're on display "foo.bar:1.2", we
	 * want to translate that into "foo.bar:1.{Scr->screen}".
	 *
	 * We strdup() because DisplayString() is a macro returning into the
	 * dpy structure, and we're going to mutate the value we get from it.
	 */
	_ds = DisplayString(dpy);
	if(_ds) {
		char *ds;
		char *colon;

		ds = strdup(_ds);
		if(!ds) {
			goto end_execute;
		}

		/* If it's not host:dpy, we don't have anything to do here */
		colon = strrchr(ds, ':');
		if(colon) {
			char *dot, *new_display;

			/* Find the . in display.screen and chop it off */
			dot = strchr(colon, '.');
			if(dot) {
				*dot = '\0';
			}

			/* Build a new string with our correct screen info */
			asprintf(&new_display, "%s.%d", ds, Scr->screen);
			if(!new_display) {
				free(ds);
				goto end_execute;
			}

			/* And set */
			setenv("DISPLAY", new_display, 1);
			free(new_display);
			restorevar = 1;
		}
		free(ds);
	}


	/*
	 * We replace a couple placeholders in the string.  $currentworkspace
	 * is documented in the manual; $redirect is not.
	 */
	subs = strstr(s, "$currentworkspace");
	if(subs) {
		char *tmp;
		char *wsname;

		wsname = GetCurrentWorkSpaceName(Scr->currentvs);
		if(!wsname) {
			wsname = "";
		}

		tmp = replace_substr(s, "$currentworkspace", wsname);
		if(!tmp) {
			goto end_execute;
		}
		free(s);
		s = tmp;
	}

	subs = strstr(s, "$redirect");
	if(subs) {
		char *tmp;
		char *redir;

		if(CLarg.is_captive) {
			asprintf(&redir, "-xrm 'ctwm.redirect:%s'", Scr->captivename);
			if(!redir) {
				goto end_execute;
			}
		}
		else {
			redir = malloc(1);
			*redir = '\0';
		}

		tmp = replace_substr(s, "$redirect", redir);
		free(s);
		s = tmp;

		free(redir);
	}


	/*
	 * Call it.  Return value doesn't really matter, since whatever
	 * happened we're done.  Maybe someday if we develop a "show user
	 * message" generalized func, we can tell the user if executing
	 * failed somehow.
	 */
	system(s);


	/*
	 * Restore $DISPLAY if we changed it.  It's probably only necessary
	 * in edge cases (it might be used by ctwm restarting itself, for
	 * instance) and it's not quite clear whether the DisplayString()
	 * result would even be wrong for that, but what the heck, setenv()
	 * is cheap.
	 */
	if(restorevar) {
		if(orig_display) {
			setenv("DISPLAY", orig_display, 1);
		}
		else {
			unsetenv("DISPLAY");
		}
	}


	/* Clean up */
end_execute:
	free(s);
	return;
}


static void
SendDeleteWindowMessage(TwmWindow *tmp, Time timestamp)
{
	send_clientmessage(tmp->w, XA_WM_DELETE_WINDOW, timestamp);
}

static void
SendSaveYourselfMessage(TwmWindow *tmp, Time timestamp)
{
	send_clientmessage(tmp->w, XA_WM_SAVE_YOURSELF, timestamp);
}


/*
 * BumpWindowColormap - rotate our internal copy of WM_COLORMAP_WINDOWS
 */
static void
BumpWindowColormap(TwmWindow *tmp, int inc)
{
	int i, j, previously_installed;
	ColormapWindow **cwins;

	if(!tmp) {
		return;
	}

	if(inc && tmp->cmaps.number_cwins > 0) {
		cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *)*
		                                   tmp->cmaps.number_cwins);
		if(cwins) {
			if((previously_installed =
			                        /* SUPPRESS 560 */(Scr->cmapInfo.cmaps == &tmp->cmaps &&
			                                        tmp->cmaps.number_cwins))) {
				for(i = tmp->cmaps.number_cwins; i-- > 0;) {
					tmp->cmaps.cwins[i]->colormap->state = 0;
				}
			}

			for(i = 0; i < tmp->cmaps.number_cwins; i++) {
				j = i - inc;
				if(j >= tmp->cmaps.number_cwins) {
					j -= tmp->cmaps.number_cwins;
				}
				else if(j < 0) {
					j += tmp->cmaps.number_cwins;
				}
				cwins[j] = tmp->cmaps.cwins[i];
			}

			free((char *) tmp->cmaps.cwins);

			tmp->cmaps.cwins = cwins;

			if(tmp->cmaps.number_cwins > 1)
				memset(tmp->cmaps.scoreboard, 0,
				       ColormapsScoreboardLength(&tmp->cmaps));

			if(previously_installed) {
				InstallColormaps(PropertyNotify, NULL);
			}
		}
	}
	else {
		FetchWmColormapWindows(tmp);
	}
	return;
}


static int
FindConstraint(TwmWindow *tmp_win, int direction)
{
	TwmWindow   *t;
	int         w, h;
	int         winx = tmp_win->frame_x;
	int         winy = tmp_win->frame_y;
	int         winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	int         winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;
	int         ret;

	switch(direction) {
		case J_LEFT   :
			if(winx < Scr->BorderLeft) {
				return -1;
			}
			ret = Scr->BorderLeft;
			break;
		case J_RIGHT  :
			if(winx + winw > Scr->rootw - Scr->BorderRight) {
				return -1;
			}
			ret = Scr->rootw - Scr->BorderRight;
			break;
		case J_TOP    :
			if(winy < Scr->BorderTop) {
				return -1;
			}
			ret = Scr->BorderTop;
			break;
		case J_BOTTOM :
			if(winy + winh > Scr->rooth - Scr->BorderBottom) {
				return -1;
			}
			ret = Scr->rooth - Scr->BorderBottom;
			break;
		default       :
			return -1;
	}
	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		if(t == tmp_win) {
			continue;
		}
		if(!visible(t)) {
			continue;
		}
		if(!t->mapped) {
			continue;
		}
		w = t->frame_width  + 2 * t->frame_bw;
		h = t->frame_height + 2 * t->frame_bw;

		switch(direction) {
			case J_LEFT :
				if(winx        <= t->frame_x + w) {
					continue;
				}
				if(winy        >= t->frame_y + h) {
					continue;
				}
				if(winy + winh <= t->frame_y) {
					continue;
				}
				ret = MAX(ret, t->frame_x + w);
				break;
			case J_RIGHT :
				if(winx + winw >= t->frame_x) {
					continue;
				}
				if(winy        >= t->frame_y + h) {
					continue;
				}
				if(winy + winh <= t->frame_y) {
					continue;
				}
				ret = MIN(ret, t->frame_x);
				break;
			case J_TOP :
				if(winy        <= t->frame_y + h) {
					continue;
				}
				if(winx        >= t->frame_x + w) {
					continue;
				}
				if(winx + winw <= t->frame_x) {
					continue;
				}
				ret = MAX(ret, t->frame_y + h);
				break;
			case J_BOTTOM :
				if(winy + winh >= t->frame_y) {
					continue;
				}
				if(winx        >= t->frame_x + w) {
					continue;
				}
				if(winx + winw <= t->frame_x) {
					continue;
				}
				ret = MIN(ret, t->frame_y);
				break;
		}
	}
	return ret;
}
