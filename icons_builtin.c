/*
 * Built-in icon building
 */

#include "ctwm.h"

#include <stdlib.h>

#include "util.h"
#include "screen.h"
#include "iconmgr.h"

#include "icons_builtin.h"


struct Colori {
	Pixel color;
	Pixmap pix;
	struct Colori *next;
};

Pixmap
Create3DMenuIcon(unsigned int height,
                 unsigned int *widthp, unsigned int *heightp,
                 ColorPair cp)
{
	unsigned int h, w;
	int         i;
	struct Colori *col;
	static struct Colori *colori = NULL;

	h = height;
	w = h * 7 / 8;
	if(h < 1) {
		h = 1;
	}
	if(w < 1) {
		w = 1;
	}
	*widthp  = w;
	*heightp = h;

	for(col = colori; col; col = col->next) {
		if(col->color == cp.back) {
			break;
		}
	}
	if(col != NULL) {
		return (col->pix);
	}
	col = malloc(sizeof(struct Colori));
	col->color = cp.back;
	col->pix   = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	col->next = colori;
	colori = col;

	Draw3DBorder(col->pix, 0, 0, w, h, 1, cp, off, true, false);
	for(i = 3; i + 5 < h; i += 5) {
		Draw3DBorder(col->pix, 4, i, w - 8, 3, 1, Scr->MenuC, off, true, false);
	}
	return (colori->pix);
}


Pixmap
CreateMenuIcon(int height, unsigned int *widthp, unsigned int *heightp)
{
	int h, w;
	int ih, iw;
	int ix, iy;
	int mh, mw;
	int tw, th;
	int lw, lh;
	int lx, ly;
	int lines, dly;
	int offset;
	int bw;

	h = height;
	w = h * 7 / 8;
	if(h < 1) {
		h = 1;
	}
	if(w < 1) {
		w = 1;
	}
	*widthp = w;
	*heightp = h;
	if(Scr->tbpm.menu == None) {
		Pixmap  pix;
		GC      gc;

		pix = Scr->tbpm.menu = XCreatePixmap(dpy, Scr->Root, w, h, 1);
		gc = XCreateGC(dpy, pix, 0L, NULL);
		XSetForeground(dpy, gc, 0L);
		XFillRectangle(dpy, pix, gc, 0, 0, w, h);
		XSetForeground(dpy, gc, 1L);
		ix = 1;
		iy = 1;
		ih = h - iy * 2;
		iw = w - ix * 2;
		offset = ih / 8;
		mh = ih - offset;
		mw = iw - offset;
		bw = mh / 16;
		if(bw == 0 && mw > 2) {
			bw = 1;
		}
		tw = mw - bw * 2;
		th = mh - bw * 2;
		XFillRectangle(dpy, pix, gc, ix, iy, mw, mh);
		XFillRectangle(dpy, pix, gc, ix + iw - mw, iy + ih - mh, mw, mh);
		XSetForeground(dpy, gc, 0L);
		XFillRectangle(dpy, pix, gc, ix + bw, iy + bw, tw, th);
		XSetForeground(dpy, gc, 1L);
		lw = tw / 2;
		if((tw & 1) ^ (lw & 1)) {
			lw++;
		}
		lx = ix + bw + (tw - lw) / 2;

		lh = th / 2 - bw;
		if((lh & 1) ^ ((th - bw) & 1)) {
			lh++;
		}
		ly = iy + bw + (th - bw - lh) / 2;

		lines = 3;
		if((lh & 1) && lh < 6) {
			lines--;
		}
		dly = lh / (lines - 1);
		while(lines--) {
			XFillRectangle(dpy, pix, gc, lx, ly, lw, bw);
			ly += dly;
		}
		XFreeGC(dpy, gc);
	}
	return Scr->tbpm.menu;
}



Pixmap
Create3DIconManagerIcon(ColorPair cp)
{
	unsigned int w, h;
	struct Colori *col;
	static struct Colori *colori = NULL;

	w = siconify_width;
	h = siconify_height;

	for(col = colori; col; col = col->next) {
		if(col->color == cp.back) {
			break;
		}
	}
	if(col != NULL) {
		return (col->pix);
	}
	col = malloc(sizeof(struct Colori));
	col->color = cp.back;
	col->pix   = XCreatePixmap(dpy, Scr->Root, w, h, Scr->d_depth);
	Draw3DBorder(col->pix, 0, 0, w, h, 4, cp, off, true, false);
	col->next = colori;
	colori = col;

	return (colori->pix);
}
