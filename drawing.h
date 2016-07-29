/*
 * General drawing routines
 */

#ifndef _CTWM_DRAWING_H
#define _CTWM_DRAWING_H

void Draw3DBorder(Window w, int x, int y, int width, int height, int bw,
                  ColorPair cp, int state, bool fill, bool forcebw);


typedef enum {
	WSPCWINDOW,
	OCCUPYWINDOW,
	OCCUPYBUTTON,
} PWBType;

void PaintWsButton(PWBType which, VirtualScreen *vs, Window w,
                   char *label, ColorPair cp, ButtonState state);


#endif // _CTWM_DRAWING_H
