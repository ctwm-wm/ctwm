/*
 * Copyright notice...
 */

#include "xparsegeometry.h"

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "r_layout.h"
#include "r_area.h"


int RLayoutXParseGeometry(RLayout *layout, const char *geometry, int *x, int *y,
                          unsigned int *width, unsigned int *height)
{
	char *sep;

	sep = strchr(geometry, ':');
	if(sep != NULL) {
		if(layout != NULL) {
			RArea mon = RLayoutGetAreaByName(layout, geometry, sep - geometry);
			if(RAreaIsValid(&mon)) {
				int mask = XParseGeometry(sep + 1, x, y, width, height);
				RArea big = RLayoutBigArea(layout);

				if(mask & XValue) {
					if(mask & XNegative) {
						*x -= big.width - mon.width - (mon.x - big.x);
					}
					else {
						*x += mon.x - big.x;
					}
				}

				if(mask & YValue) {
					if(mask & YNegative) {
						*y -= big.height - mon.height - (mon.y - big.y);
					}
					else {
						*y += mon.y - big.y;
					}
				}

				return mask;
			}
		}

		// Name not found, keep the geometry part as-is
		geometry = sep + 1;
	}

	return XParseGeometry(geometry, x, y, width, height);
}
