/*
 * Copyright notice...
 */

#include "xrandr.h"

#include <stdio.h>
#include <X11/extensions/Xrandr.h>

#include "r_area_list.h"
#include "r_area.h"


RLayout *XrandrNewLayout(Display *dpy, Window rootw)
{
	int i_nmonitors = 0, index;
	XRRMonitorInfo *ps_monitors;
	RAreaList *areas;
	RArea cur_area;

	ps_monitors = XRRGetMonitors(dpy, rootw, 1, &i_nmonitors);
	if(ps_monitors == NULL || i_nmonitors == 0) {
		fprintf(stderr, "XRRGetMonitors failed");
		return NULL;
	}

	areas = RAreaListNew(i_nmonitors, NULL);
	for(index = 0; index < i_nmonitors; index++) {
		RAreaNewIn(ps_monitors[index].x,
		           ps_monitors[index].y,
		           ps_monitors[index].width,
		           ps_monitors[index].height,
		           &cur_area);
#ifdef DEBUG
		fprintf(stderr, "NEW area: ");
		RAreaPrint(&cur_area);
		fprintf(stderr, "\n");
#endif

		RAreaListAdd(areas, &cur_area);
	}
	return RLayoutNew(areas);
}
