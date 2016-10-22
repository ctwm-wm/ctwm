/*
 * Functions related to moving/resizing windows.
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>

#include "colormaps.h"
#include "decorations.h"
#include "events.h"
#include "event_handlers.h"
#include "functions.h"
#include "functions_defs.h"
#include "functions_internal.h"
#include "icons.h"
#include "otp.h"
#include "parse.h"
#include "resize.h"
#include "screen.h"
#include "vscreen.h"
#include "win_ops.h"
#include "win_utils.h"
#include "workspace_manager.h"


static void movewindow(EF_FULLPROTO);

/* XXX TEMP */
bool belongs_to_twm_window(TwmWindow *t, Window w);
extern Time last_time;
extern bool func_reset_cursor;

typedef enum {
	MFD_BOTTOM,
	MFD_LEFT,
	MFD_RIGHT,
	MFD_TOP,
} MoveFillDir;
int FindConstraint(TwmWindow *tmp_win, MoveFillDir direction);


/*
 * Constrained move variables
 *
 * Used in the resize handling, but needed over in event code for
 * ButtonRelease as well.
 */
bool ConstMove = false;
CMoveDir ConstMoveDir;
int ConstMoveX;
int ConstMoveY;
int ConstMoveXL;
int ConstMoveXR;
int ConstMoveYT;
int ConstMoveYB;



/*
 * Resizing to a window's idea of its "normal" size, from WM_NORMAL_HINTS
 * property.
 */
DFHANDLER(initsize)
{
	int grav, x, y;
	unsigned int width, height, swidth, sheight;

	grav = ((tmp_win->hints.flags & PWinGravity)
	        ? tmp_win->hints.win_gravity : NorthWestGravity);

	if(!(tmp_win->hints.flags & USSize) && !(tmp_win->hints.flags & PSize)) {
		return;
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
	return;
}



/*
 * Setting a window to a specific specified geometry string.
 */
DFHANDLER(moveresize)
{
	int x, y, mask;
	unsigned int width, height;
	int px = 20, py = 30;

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
	return;
}


/*
 * Making a specified alteration to a window's size
 */
DFHANDLER(changesize)
{
	/* XXX Only use of this func; should we collapse? */
	ChangeSize(action, tmp_win);
}


/*
 * Stashing and flipping back to a geometry
 */
DFHANDLER(savegeometry)
{
	savegeometry(tmp_win);
}

DFHANDLER(restoregeometry)
{
	restoregeometry(tmp_win);
}



/*
 * Moving windows around
 */
DFHANDLER(move)
{
	movewindow(EF_ARGS);
}
DFHANDLER(forcemove)
{
	movewindow(EF_ARGS);
}
DFHANDLER(movepack)
{
	movewindow(EF_ARGS);
}
DFHANDLER(movepush)
{
	movewindow(EF_ARGS);
}


/* f.move and friends backend */
static void
movewindow(EF_FULLPROTO)
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
				func_reset_cursor = false;  // Leave cursor alone
				return;
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
			func_reset_cursor = false;  // Leave cursor alone
			return;
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

	return;
}



/*
 * f.fullzoom and its siblings
 */
DFHANDLER(zoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(horizoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(fullzoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(fullscreenzoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(leftzoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(rightzoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(topzoom)
{
	fullzoom(tmp_win, func);
}
DFHANDLER(bottomzoom)
{
	fullzoom(tmp_win, func);
}



/*
 * The main f.resize handler
 */
DFHANDLER(resize)
{
	PopDownMenu();
	if(tmp_win->squeezed) {
		XBell(dpy, 0);
		return;
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
			bool from3dborder = (eventp->xbutton.window == tmp_win->frame);
			bool fromtitlebar = !from3dborder &&
			                    belongs_to_twm_window(tmp_win, eventp->xbutton.window);

			/* Save pointer position so we can tell if it was moved or
			   not during the resize. */
			ResizeOrigX = eventp->xbutton.x_root;
			ResizeOrigY = eventp->xbutton.y_root;

			StartResize(eventp, tmp_win, fromtitlebar, from3dborder);
			func_reset_cursor = false;  // Leave special cursor alone

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
		}
	}
	return;
}


/*
 * f.pack
 *
 * XXX Collapse this down; no need for an extra level of indirection on
 * the function calling.
 */
static void packwindow(TwmWindow *tmp_win, const char *direction);
DFHANDLER(pack)
{
	if(tmp_win->squeezed) {
		XBell(dpy, 0);
		return;
	}
	packwindow(tmp_win, action);
}

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



/*
 * f.fill handling.
 *
 * XXX Similar to above, collapse away this extra level of function.
 */
static void fillwindow(TwmWindow *tmp_win, const char *direction);
DFHANDLER(fill)
{
	if(tmp_win->squeezed) {
		XBell(dpy, 0);
		return;
	}
	fillwindow(tmp_win, action);
}

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
