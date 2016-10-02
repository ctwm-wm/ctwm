/*
 * Dispatcher for our f.whatever functions
 */


#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>

#include "add_window.h"
#include "animate.h"
#include "colormaps.h"
#include "ctopts.h"
#include "ctwm_atoms.h"
#include "cursor.h"
#include "decorations.h"
#include "events.h"
#include "event_handlers.h"
#include "iconmgr.h"
#include "icons.h"
#include "list.h" // for match()
#include "otp.h"
#include "parse.h"
#include "resize.h"
#include "screen.h"
#include "util.h"
#include "drawing.h"
#include "occupation.h"
#include "version.h"
#include "win_iconify.h"
#include "win_ops.h"
#include "win_utils.h"
#include "windowbox.h"
#include "captive.h"
#include "vscreen.h"
#include "workspace_manager.h"
#include "workspace_utils.h"

#include "ext/repl_str.h"

#include "functions.h"


/*
 * Various functions can be executed "from the root" (which generally
 * means "from a menu"), but apply to a specific window (e.g., f.move,
 * f.identify, etc).  You obviously can't be selecting it from a menu and
 * pointing at the window to target at the same time, so we have to
 * 2-step those calls.  This happens via the DeferExecution() call in the
 * implementations of those functions, which stashes the "in progress"
 * function in RootFunction.  The HandleButtonPress() event handler will
 * later notice that and loop events back around into ExecuteFunction()
 * again to pick up where it left off.
 *
 * (a more descriptive name might be in order)
 */
int RootFunction = 0;

/*
 * Which move-ish function is in progress.  This is _almost_ really a
 * local var in the movewindow() function, but we also reference it in
 * the HandleButtonRelease() event handler because that has to know
 * which move variant we're doing to figure out whether it has to
 * constrain the final coordinates in various ways.
 */
int MoveFunction;

/* Building the f.identify window.  The events code grubs in these. */
#define INFO_LINES 30
#define INFO_SIZE 200
char Info[INFO_LINES][INFO_SIZE];


/*
 * Constrained move variables
 *
 * Gets used in event handling for ButtonRelease.
 */
bool ConstMove = false;
CMoveDir ConstMoveDir;
int ConstMoveX;
int ConstMoveY;
int ConstMoveXL;
int ConstMoveXR;
int ConstMoveYT;
int ConstMoveYB;

bool WindowMoved = false;

/*
 * Globals used to keep track of whether the mouse has moved during a
 * resize function.
 */
int ResizeOrigX;
int ResizeOrigY;


/* Time of various actions */
static Time last_time = 0;


/*
 * MoveFillDir-ectional specifiers, used in jump/pack/fill
 */
typedef enum {
	MFD_BOTTOM,
	MFD_LEFT,
	MFD_RIGHT,
	MFD_TOP,
} MoveFillDir;




static void jump(TwmWindow *tmp_win, MoveFillDir direction, const char *action);
static void ShowIconManager(void);
static void HideIconManager(void);
static bool DeferExecution(int context, int func, Cursor cursor);
static void Identify(TwmWindow *t);
static bool belongs_to_twm_window(TwmWindow *t, Window w);
static void packwindow(TwmWindow *tmp_win, const char *direction);
static void fillwindow(TwmWindow *tmp_win, const char *direction);
static bool movewindow(int func, Window w, TwmWindow *tmp_win,
                       XEvent *eventp, int context, bool pulldown);
static bool should_defer(int func);
static Cursor defer_cursor(int func);
static Cursor NeedToDefer(MenuRoot *root);
static void Execute(const char *_s);
static void SendSaveYourselfMessage(TwmWindow *tmp, Time timestamp);
static void SendDeleteWindowMessage(TwmWindow *tmp, Time timestamp);
static int FindConstraint(TwmWindow *tmp_win, MoveFillDir direction);


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
 *      true if should continue with remaining actions else false to abort
 *
 ***********************************************************************
 */

bool
ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                XEvent *eventp, int context, bool pulldown)
{
	Window rootw;
	bool do_next_action = true;
	bool fromtitlebar = false;
	bool from3dborder = false;
	TwmWindow *t;

	/* This should always start out clear when we come in here */
	RootFunction = 0;

	/* Early escape for cutting out of things */
	if(Cancel) {
		return true;        /* XXX should this be false? */
	}


	/*
	 * For most functions with a few exceptions, grab the pointer.
	 *
	 * XXX big XXX.  I have no idea why.  Apart from adding 1 or 2
	 * functions to the exclusion list, this code comes verbatim from
	 * twm, which has no history or documentation as to why it's
	 * happening.
	 *
	 * My best guess is that this is being done solely to set the cursor?
	 * X-ref the comment on the Ungrab() at the end of the function.
	 */
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


	/*
	 * Do the meat of running whatever func is being called.
	 */
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
				do_next_action = false;
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

		case F_SHOWICONMGR:
			if(Scr->NoIconManagers) {
				break;
			}
			ShowIconManager();
			break;

		case F_STARTANIMATION:
			StartAnimation();
			break;

		case F_STOPANIMATION:
			StopAnimation();
			break;

		case F_SPEEDUPANIMATION:
			ModifyAnimationSpeed(1);
			break;

		case F_SLOWDOWNANIMATION:
			ModifyAnimationSpeed(-1);
			break;

		case F_HIDEICONMGR:
			if(Scr->NoIconManagers) {
				break;
			}
			HideIconManager();
			break;

		case F_SHOWWORKSPACEMGR:
			if(! Scr->workSpaceManagerActive) {
				break;
			}
			DeIconify(Scr->currentvs->wsw->twm_win);
			OtpRaise(Scr->currentvs->wsw->twm_win, WinWin);
			break;

		case F_HIDEWORKSPACEMGR:
			if(! Scr->workSpaceManagerActive) {
				break;
			}
			Iconify(Scr->currentvs->wsw->twm_win, eventp->xbutton.x_root - 5,
			        eventp->xbutton.y_root - 5);
			break;

		case F_TOGGLEWORKSPACEMGR:
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

		case F_TOGGLESTATE:
			WMgrToggleState(Scr->currentvs);
			break;

		case F_SETBUTTONSTATE:
			WMgrSetButtonsState(Scr->currentvs);
			break;

		case F_SETMAPSTATE:
			WMgrSetMapState(Scr->currentvs);
			break;

		case F_PIN:
			if(! ActiveMenu) {
				break;
			}
			if(ActiveMenu->pinned) {
				XUnmapWindow(dpy, ActiveMenu->w);
				ActiveMenu->mapped = MRM_UNMAPPED;
			}
			else {
				XWindowAttributes attr;
				MenuRoot *menu;

				if(ActiveMenu->pmenu == NULL) {
					menu  = malloc(sizeof(MenuRoot));
					*menu = *ActiveMenu;
					menu->pinned = true;
					menu->mapped = MRM_NEVER;
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
				if(menu->mapped == MRM_MAPPED) {
					break;
				}
				XGetWindowAttributes(dpy, ActiveMenu->w, &attr);
				menu->x = attr.x;
				menu->y = attr.y;
				XMoveWindow(dpy, menu->w, menu->x, menu->y);
				XMapRaised(dpy, menu->w);
				menu->mapped = MRM_MAPPED;
			}
			PopDownMenu();
			break;

		case F_MOVEMENU:
			break;

		case F_FITTOCONTENT:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			if(!tmp_win->iswinbox) {
				XBell(dpy, 0);
				break;
			}
			fittocontent(tmp_win);
			break;

		case F_VANISH:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}

			WMgrRemoveFromCurrentWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_WARPHERE:
			WMgrAddToCurrentWorkSpaceAndWarp(Scr->currentvs, action);
			break;

		case F_ADDTOWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			AddToWorkSpace(action, tmp_win);
			break;

		case F_REMOVEFROMWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			RemoveFromWorkSpace(action, tmp_win);
			break;

		case F_TOGGLEOCCUPATION:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			ToggleOccupation(action, tmp_win);
			break;

		case F_MOVETONEXTWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			MoveToNextWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_MOVETOPREVWORKSPACE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			MoveToPrevWorkSpace(Scr->currentvs, tmp_win);
			break;

		case F_MOVETONEXTWORKSPACEANDFOLLOW:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			MoveToNextWorkSpaceAndFollow(Scr->currentvs, tmp_win);
			break;

		case F_MOVETOPREVWORKSPACEANDFOLLOW:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			MoveToPrevWorkSpaceAndFollow(Scr->currentvs, tmp_win);
			break;

		case F_SORTICONMGR:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}

			{
				int save_sort;

				save_sort = Scr->SortIconMgr;
				Scr->SortIconMgr = true;

				if(context == C_ICONMGR) {
					SortIconManager(NULL);
				}
				else if(tmp_win->isiconmgr) {
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
				return true;
			}
			stat_ = sscanf(action, "%d", &alt);
			if(stat_ != 1) {
				return true;
			}
			if((alt < 1) || (alt > 5)) {
				return true;
			}
			AlternateKeymap = Alt1Mask << (alt - 1);
			XGrabPointer(dpy, Scr->Root, True, ButtonPressMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync,
			             Scr->Root, Scr->AlterCursor, CurrentTime);
			XGrabKeyboard(dpy, Scr->Root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
			return true;
		}

		case F_ALTCONTEXT: {
			AlternateContext = true;
			XGrabPointer(dpy, Scr->Root, False, ButtonPressMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync,
			             Scr->Root, Scr->AlterCursor, CurrentTime);
			XGrabKeyboard(dpy, Scr->Root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
			return true;
		}
		case F_IDENTIFY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}

			Identify(tmp_win);
			break;

		case F_INITSIZE: {
			int grav, x, y;
			unsigned int width, height, swidth, sheight;

			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				case ForgetGravity:
				case StaticGravity:
				case NorthWestGravity:
				case NorthGravity:
				case WestGravity:
				case CenterGravity:
					break;

				case NorthEastGravity:
				case EastGravity:
					x += swidth - width;
					break;

				case SouthWestGravity:
				case SouthGravity:
					y += sheight - height;
					break;

				case SouthEastGravity:
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
				return true;
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
			Identify(NULL);
			break;

		case F_AUTORAISE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				return true;
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
			/*
			 * This is a synthetic function; it exists only to be called
			 * internally from the various magic menus like TwmWindows
			 * etc.
			 */
			tmp_win = (TwmWindow *)action;
			if(! tmp_win) {
				break;
			}
			if(Scr->WindowFunction.func != 0) {
				ExecuteFunction(Scr->WindowFunction.func,
				                Scr->WindowFunction.item->action,
				                w, tmp_win, eventp, C_FRAME, false);
			}
			else {
				DeIconify(tmp_win);
				OtpRaise(tmp_win, WinWin);
			}
			break;

		case F_WINWARP:
			/* Synthetic function; x-ref comment on F_POPUP */
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
				return true;
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
							fromtitlebar = false;
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
					return true;
				}
			}
			break;


		case F_ZOOM:
		case F_HORIZOOM:
		case F_FULLZOOM:
		case F_FULLSCREENZOOM:
		case F_LEFTZOOM:
		case F_RIGHTZOOM:
		case F_TOPZOOM:
		case F_BOTTOMZOOM:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			fullzoom(tmp_win, func);
			break;

		case F_PACK:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			packwindow(tmp_win, action);
			break;

		case F_FILL:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			fillwindow(tmp_win, action);
			break;

		case F_JUMPLEFT:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, MFD_LEFT, action);
			break;
		case F_JUMPRIGHT:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, MFD_RIGHT, action);
			break;
		case F_JUMPDOWN:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, MFD_BOTTOM, action);
			break;
		case F_JUMPUP:
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}
			if(tmp_win->squeezed) {
				XBell(dpy, 0);
				break;
			}
			jump(tmp_win, MFD_TOP, action);
			break;

		case F_SAVEGEOMETRY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			savegeometry(tmp_win);
			break;

		case F_RESTOREGEOMETRY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			restoregeometry(tmp_win);
			break;

		case F_HYPERMOVE: {
			bool    cont = true;
			Window  root = RootWindow(dpy, Scr->screen);
			Cursor  cursor;
			Window captive_root;

			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}

			if(tmp_win->iswinbox || tmp_win->iswspmgr) {
				XBell(dpy, 0);
				break;
			}

			{
				CaptiveCTWM cctwm = GetCaptiveCTWMUnderPointer();
				cursor = MakeStringCursor(cctwm.name);
				free(cctwm.name);
				captive_root = cctwm.root;
			}

			XGrabPointer(dpy, root, True,
			             ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
			             GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);
			while(cont) {
				XMaskEvent(dpy, ButtonPressMask | ButtonMotionMask |
				           ButtonReleaseMask, &Event);
				switch(Event.xany.type) {
					case ButtonPress:
						cont = false;
						break;

					case ButtonRelease: {
						CaptiveCTWM cctwm = GetCaptiveCTWMUnderPointer();
						cont = false;
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
					}

					case MotionNotify: {
						CaptiveCTWM cctwm = GetCaptiveCTWMUnderPointer();
						if(cctwm.root != captive_root) {
							unsigned int chmask;

							XFreeCursor(dpy, cursor);
							cursor = MakeStringCursor(cctwm.name);
							captive_root = cctwm.root;

							chmask = (ButtonPressMask | ButtonMotionMask
							          | ButtonReleaseMask);
							XChangeActivePointerGrab(dpy, chmask,
							                         cursor, CurrentTime);
						}
						free(cctwm.name);
						break;
					}
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
			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
			}

			/* All in external func */
			if(movewindow(func, w, tmp_win, eventp, context, pulldown)) {
				return true;
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
				return true;
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

			break;
		}

		case F_MOVETITLEBAR: {
			Window grabwin;
			int deltax = 0, newx = 0;
			int origX;
			int origNum;
			SqueezeInfo *si;

			if(DeferExecution(context, func, Scr->MoveCursor)) {
				return true;
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

			while(1) {
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
			Cursor curs;

			if((mroot = FindMenuRoot(action)) == NULL) {
				if(!action) {
					action = "undef";
				}
				fprintf(stderr, "%s: couldn't find function \"%s\"\n",
				        ProgramName, (char *)action);
				return true;
			}

			if((curs = NeedToDefer(mroot)) != None
			                && DeferExecution(context, func, curs)) {
				return true;
			}
			else {
				for(mitem = mroot->first; mitem != NULL; mitem = mitem->next) {
					if(!ExecuteFunction(mitem->func, mitem->action, w,
					                    tmp_win, eventp, context, pulldown)) {
						/* pebl FIXME: the focus should be updated here,
						 or the function would operate on the same window */
						break;
					}
				}
			}

			break;
		}

		case F_DEICONIFY:
		case F_ICONIFY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				return true;
			}

			Squeeze(tmp_win);
			break;

		case F_UNSQUEEZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}

			if(tmp_win->squeezed) {
				Squeeze(tmp_win);
			}
			break;

		case F_SHOWBACKGROUND:
			ShowBackground(Scr->currentvs, -1);
			break;

		case F_RAISELOWER:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				return true;
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
				return true;
			}

			/* check to make sure raise is not from the WindowFunction */
			if(tmp_win->icon && (w == tmp_win->icon->w) && Context != C_ROOT) {
				OtpTinyLower(tmp_win, IconWin);
			}
			else {
				OtpTinyLower(tmp_win, WinWin);
				WMapLower(tmp_win);
			}
			break;

		case F_RAISEORSQUEEZE:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				return true;
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
				return true;
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
				return true;
			}

			if(!tmp_win->isicon) {
				if(!Scr->FocusRoot && Scr->Focus == tmp_win) {
					FocusOnRoot();
				}
				else {
					InstallWindowColormaps(0, tmp_win);
					SetFocus(tmp_win, eventp->xbutton.time);
					Scr->FocusRoot = false;
				}
			}
			break;

		case F_DESTROY:
			if(DeferExecution(context, func, Scr->DestroyCursor)) {
				return true;
			}

			if(tmp_win->isiconmgr || tmp_win->iswinbox || tmp_win->iswspmgr
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
				return true;
			}

			if(tmp_win->isiconmgr) {     /* don't send ourself a message */
				HideIconManager();
				break;
			}
			if(tmp_win->iswinbox || tmp_win->iswspmgr
			                || (Scr->workSpaceMgr.occupyWindow
			                    && tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win)) {
				XBell(dpy, 0);
				break;
			}
			if(tmp_win->protocols & DoesWmDeleteWindow) {
				SendDeleteWindowMessage(tmp_win, EventTime);
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
				return true;
			}

			if(tmp_win->isiconmgr) {
				HideIconManager();
				break;
			}
			if(tmp_win->iswinbox || tmp_win->iswspmgr
			                || (Scr->workSpaceMgr.occupyWindow
			                    && tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win)) {
				XBell(dpy, 0);
				break;
			}
			if(tmp_win->protocols & DoesWmDeleteWindow) {
				SendDeleteWindowMessage(tmp_win, EventTime);
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
				return true;
			}

			if(tmp_win->protocols & DoesWmSaveYourself) {
				SendSaveYourselfMessage(tmp_win, EventTime);
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

			break;
		}

		case F_COLORMAP: {
			/* XXX Window targetting; should this be on the Defer list? */
			if(strcmp(action, COLORMAP_NEXT) == 0) {
				BumpWindowColormap(tmp_win, 1);
			}
			else if(strcmp(action, COLORMAP_PREV) == 0) {
				BumpWindowColormap(tmp_win, -1);
			}
			else {
				BumpWindowColormap(tmp_win, 0);
			}

			break;
		}

		case F_WARPTO: {
			TwmWindow *tw;
			int len;

			len = strlen(action);

#ifdef WARPTO_FROM_ICONMGR
			/* XXX should be iconmgrp? */
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

			break;
		}

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

			break;
		}

		case F_RING:  /* Taken from vtwm version 5.3 */
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
					Scr->Ring = (next != tmp_win ? next : NULL);
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
			/*tmp_win->ring.cursor_valid = false;*/
			break;

		case F_WARPRING:
			switch(((char *)action)[0]) {
				case 'n':
					WarpAlongRing(&eventp->xbutton, true);
					break;
				case 'p':
					WarpAlongRing(&eventp->xbutton, false);
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
			                  Scr->rootw,
			                  Scr->rooth,
			                  0,
			                  CopyFromParent, CopyFromParent,
			                  CopyFromParent, valuemask,
			                  &attributes);
			XMapWindow(dpy, w);
			XDestroyWindow(dpy, w);
			XFlush(dpy);

			break;
		}

		case F_OCCUPY:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
			}
			Occupy(tmp_win);
			break;

		case F_OCCUPYALL:
			if(DeferExecution(context, func, Scr->SelectCursor)) {
				return true;
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
				return true;
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
				return true;
			}

			ChangeSize(action, tmp_win);
			break;

		case F_QUIT:
			Done(0);
			break;

		case F_RESCUEWINDOWS:
			RescueWindows();
			break;

	}


	/*
	 * Ungrab the pointer.  Sometimes.  This condition apparently means
	 * we got to the end of the execution (didn't return early due to
	 * e.g. a Defer), and didn't come in as a result of pressing a mouse
	 * button.  Note that this is _not_ strictly dual to the
	 * XGrabPointer() conditionally called in the switch() early on;
	 * there will be plenty of cases where one executes without the
	 * other.
	 *
	 * XXX It isn't clear that this really belong here...
	 */
	if(ButtonPressed == -1) {
		XUngrabPointer(dpy, CurrentTime);
	}

	return do_next_action;
}



/*
 * Utils
 */
/* f.jump{down,left,right,up} */
static void
jump(TwmWindow *tmp_win, MoveFillDir direction, const char *action)
{
	int          fx, fy, px, py, step, status, cons;
	int          fwidth, fheight;
	int          junkX, junkY;
	unsigned int junkK;
	Window       junkW;

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
		case MFD_LEFT:
			cons  = FindConstraint(tmp_win, MFD_LEFT);
			if(cons == -1) {
				return;
			}
			fx -= step * Scr->XMoveGrid;
			if(fx < cons) {
				fx = cons;
			}
			break;
		case MFD_RIGHT:
			cons  = FindConstraint(tmp_win, MFD_RIGHT);
			if(cons == -1) {
				return;
			}
			fx += step * Scr->XMoveGrid;
			if(fx + fwidth > cons) {
				fx = cons - fwidth;
			}
			break;
		case MFD_TOP:
			cons  = FindConstraint(tmp_win, MFD_TOP);
			if(cons == -1) {
				return;
			}
			fy -= step * Scr->YMoveGrid;
			if(fy < cons) {
				fy = cons;
			}
			break;
		case MFD_BOTTOM:
			cons  = FindConstraint(tmp_win, MFD_BOTTOM);
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


/*
 * f.showiconmanager
 */
static void
ShowIconManager(void)
{
	IconMgr   *i;
	WorkSpace *wl;

	/*
	 * XXX I don't think this is right; there can still be icon managers
	 * to show even if we've never set any Workspaces {}.  And
	 * HideIconManager() doesn't have this extra condition either...
	 */
	if(! Scr->workSpaceManagerActive) {
		return;
	}

	if(Scr->NoIconManagers) {
		return;
	}

	for(wl = Scr->workSpaceMgr.workSpaceList; wl != NULL; wl = wl->next) {
		for(i = wl->iconmgr; i != NULL; i = i->next) {
			/* Don't show iconmgr's with nothing in 'em */
			if(i->count == 0) {
				continue;
			}

			/* If it oughta be in a vscreen, show it */
			if(visible(i->twm_win)) {
				/* IM window */
				SetMapStateProp(i->twm_win, NormalState);
				XMapWindow(dpy, i->twm_win->w);
				OtpRaise(i->twm_win, WinWin);
				XMapWindow(dpy, i->twm_win->frame);

				/* Hide icon */
				if(i->twm_win->icon && i->twm_win->icon->w) {
					XUnmapWindow(dpy, i->twm_win->icon->w);
				}
			}

			/* Mark as shown */
			i->twm_win->mapped = true;
			i->twm_win->isicon = false;
		}
	}
}


/*
 * f.hideiconmanager.  Also called when you f.delete an icon manager.
 *
 * This hides all the icon managers in all the workspaces, and it doesn't
 * leave icons behind, so it's _not_ the same as just iconifying, and
 * thus not implemented by just calling Iconify(), but by doing the
 * hiding manually.
 */
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
			/* Hide the IM window */
			SetMapStateProp(i->twm_win, WithdrawnState);
			XUnmapWindow(dpy, i->twm_win->frame);

			/* Hide its icon */
			if(i->twm_win->icon && i->twm_win->icon->w) {
				XUnmapWindow(dpy, i->twm_win->icon->w);
			}

			/* Mark as pretend-iconified, even though the icon is hidden */
			i->twm_win->mapped = false;
			i->twm_win->isicon = true;
		}
	}
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
			int iwx, iwy;

			XGetGeometry(dpy, t->icon->w, &JunkRoot, &iwx, &iwy,
			             &wwidth, &wheight, &bw, &depth);
			Info[n++][0] = '\0';
			CHKN;
			snprintf(Info[n++], INFO_SIZE, "IconGeom/root     = %dx%d+%d+%d",
			         wwidth, wheight, iwx, iwy);
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

	/* Unmap if it's currently up, while we muck with it */
	if(Scr->InfoWindow.mapped) {
		XUnmapWindow(dpy, Scr->InfoWindow.win);
		/* Don't really need to bother since we're about to reset, but...  */
		Scr->InfoWindow.mapped = false;
	}

	/* Stash the new number of lines */
	Scr->InfoWindow.lines = n;

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
	XMoveResizeWindow(dpy, Scr->InfoWindow.win, px, py, width, height);
	XMapRaised(dpy, Scr->InfoWindow.win);
	Scr->InfoWindow.mapped = true;
	Scr->InfoWindow.width  = width;
	Scr->InfoWindow.height = height;
}


/*
 * And the routine to actually write the text into the InfoWindow.  This
 * gets called from events.c as a result of Expose events on the window.
 */
void
draw_info_window(void)
{
	int i;
	const int height = Scr->DefaultFont.height + 2;

	Draw3DBorder(Scr->InfoWindow.win, 0, 0,
	             Scr->InfoWindow.width, Scr->InfoWindow.height,
	             2, Scr->DefaultC, off, true, false);

	FB(Scr->DefaultC.fore, Scr->DefaultC.back);

	for(i = 0; i < Scr->InfoWindow.lines ; i++) {
		XmbDrawString(dpy, Scr->InfoWindow.win, Scr->DefaultFont.font_set,
		              Scr->NormalGC, 5,
		              (i * height) + Scr->DefaultFont.y + 5,
		              Info[i], strlen(Info[i]));
	}
}


/*
 * Is Window w part of the conglomerate of metawindows we put around the
 * real window for TwmWindow t?  Note that this does _not_ check if w is
 * the actual window we built the TwmWindow t around.
 */
static bool
belongs_to_twm_window(TwmWindow *t, Window w)
{
	/* Safety */
	if(!t) {
		return false;
	}

	/* Part of the framing we put around the window? */
	if(w == t->frame || w == t->title_w
	                || w == t->hilite_wl || w == t->hilite_wr) {
		return true;
	}

	/* Part of the icon bits? */
	if(t->icon && (w == t->icon->w || w == t->icon->bm_w)) {
		return true;
	}

	/* One of the title button windows? */
	if(t->titlebuttons) {
		TBWindow *tbw;
		int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
		for(tbw = t->titlebuttons ; nb > 0 ; tbw++, nb--) {
			if(tbw->window == w) {
				return true;
			}
		}
	}

	/* Then no */
	return false;
}


/* f.pack */
static void
packwindow(TwmWindow *tmp_win, const char *direction)
{
	int          cons, newx, newy;
	int          x, y, px, py, junkX, junkY;
	unsigned int junkK;
	Window       junkW;

	if(!strcmp(direction,   "left")) {
		cons  = FindConstraint(tmp_win, MFD_LEFT);
		if(cons == -1) {
			return;
		}
		newx  = cons;
		newy  = tmp_win->frame_y;
	}
	else if(!strcmp(direction,  "right")) {
		cons  = FindConstraint(tmp_win, MFD_RIGHT);
		if(cons == -1) {
			return;
		}
		newx  = cons;
		newx -= tmp_win->frame_width + 2 * tmp_win->frame_bw;
		newy  = tmp_win->frame_y;
	}
	else if(!strcmp(direction,    "top")) {
		cons  = FindConstraint(tmp_win, MFD_TOP);
		if(cons == -1) {
			return;
		}
		newx  = tmp_win->frame_x;
		newy  = cons;
	}
	else if(!strcmp(direction, "bottom")) {
		cons  = FindConstraint(tmp_win, MFD_BOTTOM);
		if(cons == -1) {
			return;
		}
		newx  = tmp_win->frame_x;
		newy  = cons;
		newy -= tmp_win->frame_height + 2 * tmp_win->frame_bw;
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
	SetupWindow(tmp_win, newx, newy, tmp_win->frame_width,
	            tmp_win->frame_height, -1);
}


/* f.fill */
static void
fillwindow(TwmWindow *tmp_win, const char *direction)
{
	int cons, newx, newy, save;
	unsigned int neww, newh;
	int i;
	const int winx = tmp_win->frame_x;
	const int winy = tmp_win->frame_y;
	const int winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	const int winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;

	if(!strcmp(direction, "left")) {
		cons = FindConstraint(tmp_win, MFD_LEFT);
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
			cons = FindConstraint(tmp_win, MFD_RIGHT);
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
		cons = FindConstraint(tmp_win, MFD_TOP);
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
			cons = FindConstraint(tmp_win, MFD_BOTTOM);
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
			newy = FindConstraint(tmp_win, MFD_TOP);
			tmp_win->frame_y--;
			newh = FindConstraint(tmp_win, MFD_BOTTOM) - newy;
			newh -= 2 * tmp_win->frame_bw;

			newx = tmp_win->frame_x;
			neww = tmp_win->frame_width;

			ConstrainSize(tmp_win, &neww, &newh);

			/* if the bottom of the window has moved up
			 * it will be pushed down */
			if(newy + newh < tmp_win->save_frame_y + tmp_win->save_frame_height) {
				newy = tmp_win->save_frame_y +
				       tmp_win->save_frame_height - newh;
			}
			tmp_win->zoomed = F_ZOOM;
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


/*
 * f.move and friends
 *
 * This code previously sometimes returned early out of the midst of the
 * switch() in ExecuteFunction(), but only ever true (whether
 * meaningfully or not).  So we take that and turn it into a bool
 * function that, when it returns true, the above also return's true
 * right from ExecuteFunction(); otherwise breaks out and falls through
 * like previously.  If we ever revisit the meaning of the return, we'll
 * have to get smarter about that too.
 */
static bool
movewindow(int func, /* not void *action */ Window w, TwmWindow *tmp_win,
           XEvent *eventp, int context, bool pulldown)
{
	Window grabwin, dragroot;
	Window rootw;
	unsigned int brdw;
	int origX, origY;
	bool moving_icon = false;
	bool fromtitlebar = false;
	TwmWindow *t;

	PopDownMenu();
	if(tmp_win->OpaqueMove) {
		int sw, ss;
		float sf;

		sw = tmp_win->frame_width * tmp_win->frame_height;
		ss = Scr->rootw  * Scr->rooth;
		sf = Scr->OpaqueMoveThreshold / 100.0;
		if(sw > (ss * sf)) {
			Scr->OpaqueMove = false;
		}
		else {
			Scr->OpaqueMove = true;
		}
	}
	else {
		Scr->OpaqueMove = false;
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
		moving_icon = true;
		if(tmp_win->OpaqueMove) {
			Scr->OpaqueMove = true;
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

	brdw = DragBW;
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

		ConstMove = true;
		ConstMoveDir = MOVE_NONE;
		ConstMoveX = eventp->xbutton.x_root - DragX - brdw;
		ConstMoveY = eventp->xbutton.y_root - DragY - brdw;
		width = DragWidth + 2 * brdw;
		height = DragHeight + 2 * brdw;
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
			            origDragX - brdw + Scr->currentvs->x,
			            origDragY - brdw + Scr->currentvs->y,
			            DragWidth + 2 * brdw, DragHeight + 2 * brdw,
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
	while(1) {
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
				return true;
			}
		}

		if(fromtitlebar && Event.type == ButtonPress) {
			fromtitlebar = false;
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
			WindowMoved = false;
			if(!Scr->OpaqueMove) {
				UninstallRootColormap();
			}
			return true;    /* XXX should this be false? */
		}
		if(Event.type == releaseEvent) {
			MoveOutline(dragroot, 0, 0, 0, 0, 0, 0);
			if(moving_icon &&
			                ((CurrentDragX != origDragX ||
			                  CurrentDragY != origDragY))) {
				tmp_win->icon_moved = true;
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
					fprintf(stderr, "moving_icon is true incorrectly!\n");
				}
				OtpRaise(t, WinWin);
			}
			else if(t->icon && DragWindow == t->icon->w) {
				if(!moving_icon) {
					fprintf(stderr, "moving_icon is false incorrectly!\n");
				}
				OtpRaise(t, IconWin);
			}
			else {
				fprintf(stderr, "ERROR: menus.c:2838\n");
			}
		}

		WindowMoved = true;

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
					ConstMoveY = eventp->xmotion.y_root - DragY - brdw;
					break;

				case MOVE_HORIZ:
					ConstMoveX = eventp->xmotion.x_root - DragX - brdw;
					break;
			}

			if(ConstMoveDir != MOVE_NONE) {
				int xl, yt, width, height;

				xl = ConstMoveX;
				yt = ConstMoveY;
				width = DragWidth + 2 * brdw;
				height = DragHeight + 2 * brdw;

				if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
					TryToGrid(tmp_win, &xl, &yt);
				}
				if(!moving_icon && MoveFunction == F_MOVEPUSH && Scr->OpaqueMove) {
					TryToPush(tmp_win, xl, yt);
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
				xl = xroot - DragX - brdw;
				yt = yroot - DragY - brdw;
			}
			else {
				xl = xroot - (DragWidth / 2);
				yt = yroot - (DragHeight / 2);
			}
			width = DragWidth + 2 * brdw;
			height = DragHeight + 2 * brdw;

			if(Scr->DontMoveOff && MoveFunction != F_FORCEMOVE) {
				TryToGrid(tmp_win, &xl, &yt);
			}
			if(!moving_icon && MoveFunction == F_MOVEPUSH && Scr->OpaqueMove) {
				TryToPush(tmp_win, xl, yt);
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

	return false;
}


/*
 * Check to see if a function (implicitly, a window-targetting function)
 * is happening in a context away from an actual window, and if so stash
 * up info about what's in progress and return true to tell the caller to
 * end processing the function (for now).  X-ref comment on RootFunction
 * variable definition for details.
 *
 *  Inputs:
 *      context - the context in which the mouse button was pressed
 *      func    - the function to defer
 *      cursor  - the cursor to display while waiting
 */
static bool
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

		return true;
	}

	return false;
}


/*
 * Various determinates of whether a function should be deferred if its
 * called in a general (rather than win-specific) context, and what
 * cursor should be used in the meantime.
 *
 * We define a big lookup array to do it.  We have to indirect through an
 * intermediate enum value instead of just the cursor since it isn't
 * available at compile time, and we can't just make it a pointer into
 * Scr since there are [potentially] multiple Scr's anyway.  And we need
 * an explicit unused DC_NONE value so our real values are all non-zero;
 * the ones we don't explicitly set get initialized to 0, which we can
 * then take as a flag saying "we don't defer this func".
 *
 * XXX And if so, we should use this more directly to defer things as
 * needed instead of hardcoding.
 */
typedef enum {
	DC_NONE = 0,
	DC_SELECT,
	DC_MOVE,
	DC_DESTROY,
} _dfcs_cursor;
static _dfcs_cursor dfcs[F_maxfunc + 1] = {
	/* Windowbox related */
	[F_FITTOCONTENT] = DC_SELECT,

	/* Icon manager related */
	[F_SORTICONMGR] = DC_SELECT,

	/* Messing with window occupation */
	[F_ADDTOWORKSPACE]      = DC_SELECT,
	[F_REMOVEFROMWORKSPACE] = DC_SELECT,
	[F_MOVETONEXTWORKSPACE] = DC_SELECT,
	[F_MOVETOPREVWORKSPACE] = DC_SELECT,
	[F_MOVETONEXTWORKSPACEANDFOLLOW] = DC_SELECT,
	[F_MOVETOPREVWORKSPACEANDFOLLOW] = DC_SELECT,
	[F_TOGGLEOCCUPATION] = DC_SELECT,
	[F_VANISH]    = DC_SELECT,
	[F_OCCUPY]    = DC_SELECT,
	[F_OCCUPYALL] = DC_SELECT,

	/* Messing with position */
	[F_MOVE]      = DC_MOVE,
	[F_FORCEMOVE] = DC_MOVE,
	[F_PACK]      = DC_SELECT,
	[F_FILL]      = DC_SELECT,
	[F_MOVEPACK]  = DC_MOVE,
	[F_MOVEPUSH]  = DC_MOVE,
	[F_JUMPLEFT]  = DC_MOVE,
	[F_JUMPRIGHT] = DC_MOVE,
	[F_JUMPDOWN]  = DC_MOVE,
	[F_JUMPUP]    = DC_MOVE,

	/* Messing with size */
	[F_INITSIZE]   = DC_SELECT,
	[F_RESIZE]     = DC_MOVE,
	[F_CHANGESIZE] = DC_SELECT,
	[F_ZOOM]       = DC_SELECT,
	[F_HORIZOOM]   = DC_SELECT,
	[F_RIGHTZOOM]  = DC_SELECT,
	[F_LEFTZOOM]   = DC_SELECT,
	[F_TOPZOOM]    = DC_SELECT,
	[F_BOTTOMZOOM] = DC_SELECT,
	[F_FULLZOOM]   = DC_SELECT,
	[F_FULLSCREENZOOM] = DC_SELECT,

	/* Messing with all sorts of geometry */
	[F_MOVERESIZE]   = DC_SELECT,
	[F_SAVEGEOMETRY] = DC_SELECT,
	[F_RESTOREGEOMETRY] = DC_SELECT,

	/* Special moves */
	[F_HYPERMOVE] = DC_MOVE,

	/* Window and titlebar squeeze-related */
	[F_SQUEEZE]   = DC_SELECT,
	[F_UNSQUEEZE] = DC_SELECT,
	[F_MOVETITLEBAR] = DC_MOVE,

	/* Stacking */
	[F_RAISE]     = DC_SELECT,
	[F_LOWER]     = DC_SELECT,
	[F_TINYRAISE] = DC_SELECT,
	[F_TINYLOWER] = DC_SELECT,
	[F_AUTORAISE] = DC_SELECT,
	[F_AUTOLOWER] = DC_SELECT,
	[F_RAISELOWER] = DC_SELECT,
	[F_SETPRIORITY] = DC_SELECT,
	[F_CHANGEPRIORITY] = DC_SELECT,
	[F_SWITCHPRIORITY] = DC_SELECT,
	[F_PRIORITYSWITCHING] = DC_SELECT,

	/* Combo and misc ops */
	[F_RAISEORSQUEEZE] = DC_SELECT,
	[F_IDENTIFY]   = DC_SELECT,
	[F_DEICONIFY]  = DC_SELECT,
	[F_ICONIFY]    = DC_SELECT,
	[F_FOCUS]      = DC_SELECT,
	[F_RING]       = DC_SELECT,
	[F_WINREFRESH] = DC_SELECT,
	/* x-ref comment questioning if F_COLORMAP should be here */

	/* Window deletion related */
	[F_DELETE]  = DC_DESTROY,
	[F_DESTROY] = DC_DESTROY,
	[F_SAVEYOURSELF] = DC_SELECT,
	[F_DELETEORDESTROY] = DC_DESTROY,
};

static bool
should_defer(int func)
{
	/* Shouldn't ever happen, so "no" is the best response */
	if(func < 0 || func > F_maxfunc) {
		return false;
	}

	if(dfcs[func] != DC_NONE) {
		return true;
	}
	return false;
}

static Cursor
defer_cursor(int func)
{
	/* Shouldn't ever happen, but be safe */
	if(func < 0 || func > F_maxfunc) {
		return None;
	}

	switch(dfcs[func]) {
		case DC_SELECT:
			return Scr->SelectCursor;
		case DC_MOVE:
			return Scr->MoveCursor;
		case DC_DESTROY:
			return Scr->DestroyCursor;

		default:
			/* Is there a better choice? */
			return None;
	}

	/* NOTREACHED */
	return None;
}


/*
 * Checks each function in a user-defined Function list called via
 * f.function to see any of them need to be defered.  The Function config
 * action creates pseudo-menus to store the items in that call, so we
 * loop through the "items" in that "menu".  Try not to think about that
 * too much.
 *
 * This previously used a hardcoded list of functions to defer, which was
 * substantially smaller than the list it's currently checking.  It now
 * checks all the same functions that are themselves checked
 * individually, which is almost certainly how it should have always
 * worked anyway.
 */
static Cursor
NeedToDefer(MenuRoot *root)
{
	MenuItem *mitem;

	for(mitem = root->first; mitem != NULL; mitem = mitem->next) {
		if(should_defer(mitem->func)) {
			Cursor dc = defer_cursor(mitem->func);
			if(dc == None) {
				return Scr->SelectCursor;
			}
			return dc;
		}
	}
	return None;
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


static int
FindConstraint(TwmWindow *tmp_win, MoveFillDir direction)
{
	TwmWindow  *t;
	int ret;
	const int winx = tmp_win->frame_x;
	const int winy = tmp_win->frame_y;
	const int winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	const int winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;

	switch(direction) {
		case MFD_LEFT:
			if(winx < Scr->BorderLeft) {
				return -1;
			}
			ret = Scr->BorderLeft;
			break;
		case MFD_RIGHT:
			if(winx + winw > Scr->rootw - Scr->BorderRight) {
				return -1;
			}
			ret = Scr->rootw - Scr->BorderRight;
			break;
		case MFD_TOP:
			if(winy < Scr->BorderTop) {
				return -1;
			}
			ret = Scr->BorderTop;
			break;
		case MFD_BOTTOM:
			if(winy + winh > Scr->rooth - Scr->BorderBottom) {
				return -1;
			}
			ret = Scr->rooth - Scr->BorderBottom;
			break;
		default:
			return -1;
	}
	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		const int w = t->frame_width  + 2 * t->frame_bw;
		const int h = t->frame_height + 2 * t->frame_bw;

		if(t == tmp_win) {
			continue;
		}
		if(!visible(t)) {
			continue;
		}
		if(!t->mapped) {
			continue;
		}

		switch(direction) {
			case MFD_LEFT:
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
			case MFD_RIGHT:
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
			case MFD_TOP:
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
			case MFD_BOTTOM:
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
