/*
 * Colormap handling
 */

#include "ctwm.h"

#include <stdlib.h>

#include "colormaps.h"
#include "screen.h"


/*
 * From events.c; imported manually since I'm not listing it in events.h
 * because nowhere but here needs it.
 */
extern bool ColortableThrashing;

static Bool UninstallRootColormapQScanner(Display *display, XEvent *ev,
                char *args);


/***********************************************************************
 *
 *  Procedure:
 *      InstallWindowColormaps - install the colormaps for one twm window
 *
 *  Inputs:
 *      type    - type of event that caused the installation
 *      tmp     - for a subset of event types, the address of the
 *                window structure, whose colormaps are to be installed.
 *
 ***********************************************************************
 */
int
InstallWindowColormaps(int type, TwmWindow *tmp)
{
	if(tmp) {
		return InstallColormaps(type, &tmp->cmaps);
	}
	else {
		return InstallColormaps(type, NULL);
	}
}


int
InstallColormaps(int type, Colormaps *cmaps)
{
	int i, j, n, number_cwins, state;
	ColormapWindow **cwins, *cwin, **maxcwin = NULL;
	TwmColormap *cmap;
	char *row, *scoreboard;

	switch(type) {
		case EnterNotify:
		case LeaveNotify:
		case DestroyNotify:
		default:
			/* Save the colormap to be loaded for when force loading of
			 * root colormap(s) ends.
			 */
			Scr->cmapInfo.pushed_cmaps = cmaps;
			/* Don't load any new colormap if root colormap(s) has been
			 * force loaded.
			 */
			if(Scr->cmapInfo.root_pushes) {
				return (0);
			}
			/* Don't reload the current window colormap list.
			if (Scr->cmapInfo.cmaps == cmaps)
			    return (0);
			 */
			if(Scr->cmapInfo.cmaps) {
				for(i = Scr->cmapInfo.cmaps->number_cwins,
				                cwins = Scr->cmapInfo.cmaps->cwins; i-- > 0; cwins++) {
					(*cwins)->colormap->state &= ~CM_INSTALLABLE;
				}
			}
			Scr->cmapInfo.cmaps = cmaps;
			break;

		case PropertyNotify:
		case VisibilityNotify:
		case ColormapNotify:
			break;
	}

	number_cwins = Scr->cmapInfo.cmaps->number_cwins;
	cwins = Scr->cmapInfo.cmaps->cwins;
	scoreboard = Scr->cmapInfo.cmaps->scoreboard;

	ColortableThrashing = false; /* in case installation aborted */

	state = CM_INSTALLED;

	for(i = n = 0; i < number_cwins; i++) {
		cwins[i]->colormap->state &= ~CM_INSTALL;
	}
	for(i = n = 0; i < number_cwins && n < Scr->cmapInfo.maxCmaps; i++) {
		cwin = cwins[i];
		cmap = cwin->colormap;
		if(cmap->state & CM_INSTALL) {
			continue;
		}
		cmap->state |= CM_INSTALLABLE;
		cmap->w = cwin->w;
		if(cwin->visibility != VisibilityFullyObscured) {
			row = scoreboard + (i * (i - 1) / 2);
			for(j = 0; j < i; j++)
				if(row[j] && (cwins[j]->colormap->state & CM_INSTALL)) {
					break;
				}
			if(j != i) {
				continue;
			}
			n++;
			maxcwin = &cwins[i];
			state &= (cmap->state & CM_INSTALLED);
			cmap->state |= CM_INSTALL;
		}
	}
	Scr->cmapInfo.first_req = NextRequest(dpy);

	for(; n > 0 && maxcwin >= &cwins[0]; maxcwin--) {
		cmap = (*maxcwin)->colormap;
		if(cmap->state & CM_INSTALL) {
			cmap->state &= ~CM_INSTALL;
			if(!(state & CM_INSTALLED)) {
				cmap->install_req = NextRequest(dpy);
				/* printf ("XInstallColormap : %x, %x\n", cmap, cmap->c); */
				XInstallColormap(dpy, cmap->c);
			}
			cmap->state |= CM_INSTALLED;
			n--;
		}
	}
	return (1);
}



/***********************************************************************
 *
 *  Procedures:
 *      <Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 *         These matching routines provide a mechanism to insure that
 *         the root colormap(s) is installed during operations like
 *         rubber banding or menu display that require colors from
 *         that colormap.  Calls may be nested arbitrarily deeply,
 *         as long as there is one UninstallRootColormap call per
 *         InstallRootColormap call.
 *
 *         The final UninstallRootColormap will cause the colormap list
 *         which would otherwise have be loaded to be loaded, unless
 *         Enter or Leave Notify events are queued, indicating some
 *         other colormap list would potentially be loaded anyway.
 ***********************************************************************
 */
void
InstallRootColormap(void)
{
	Colormaps *tmp;
	if(Scr->cmapInfo.root_pushes == 0) {
		/*
		 * The saving and restoring of cmapInfo.pushed_window here
		 * is a slimy way to remember the actual pushed list and
		 * not that of the root window.
		 */
		tmp = Scr->cmapInfo.pushed_cmaps;
		InstallColormaps(0, &Scr->RootColormaps);
		Scr->cmapInfo.pushed_cmaps = tmp;
	}
	Scr->cmapInfo.root_pushes++;
}


/* ARGSUSED*/
static Bool
UninstallRootColormapQScanner(Display *display, XEvent *ev,
                              char *args)
{
	if(!*args) {
		if(ev->type == EnterNotify) {
			if(ev->xcrossing.mode != NotifyGrab) {
				*args = 1;
			}
		}
		else if(ev->type == LeaveNotify) {
			if(ev->xcrossing.mode == NotifyNormal) {
				*args = 1;
			}
		}
	}

	return (False);
}


void
UninstallRootColormap(void)
{
	char args;
	XEvent dummy;

	if(Scr->cmapInfo.root_pushes) {
		Scr->cmapInfo.root_pushes--;
	}

	if(!Scr->cmapInfo.root_pushes) {
		/*
		 * If we have subsequent Enter or Leave Notify events,
		 * we can skip the reload of pushed colormaps.
		 */
		XSync(dpy, 0);
		args = 0;
		(void) XCheckIfEvent(dpy, &dummy, UninstallRootColormapQScanner, &args);

		if(!args) {
			InstallColormaps(0, Scr->cmapInfo.pushed_cmaps);
		}
	}
}
