/*
 * Copyright notice...
 */

#include "xrandr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/extensions/Xrandr.h>

#include "r_area_list.h"
#include "r_area.h"


RLayout *XrandrNewLayout(Display *dpy, Window rootw)
{
	int i_nmonitors = 0, index;
	XRRMonitorInfo *ps_monitors;
	char **monitor_names, *name;
	RAreaList *areas;
	RArea cur_area;

	ps_monitors = XRRGetMonitors(dpy, rootw, 1, &i_nmonitors);
	if(ps_monitors == NULL || i_nmonitors == 0) {
		fprintf(stderr, "XRRGetMonitors failed");
		return NULL;
	}

	monitor_names = malloc((i_nmonitors + 1) * sizeof(char *));
	if(monitor_names == NULL) {
		abort();
	}

	areas = RAreaListNew(i_nmonitors, NULL);
	for(index = 0; index < i_nmonitors; index++) {
		cur_area = RAreaNew(ps_monitors[index].x,
		                    ps_monitors[index].y,
		                    ps_monitors[index].width,
		                    ps_monitors[index].height);

		name = XGetAtomName(dpy, ps_monitors[index].name);
#ifdef DEBUG
		fprintf(stderr, "NEW area: %s%s",
		        name != NULL ? name : "",
		        name != NULL ? ":" : "");
		RAreaPrint(&cur_area);
		fprintf(stderr, "\n");
#endif
		if(name != NULL) {
			monitor_names[index] = strdup(name);
			XFree(name);
		}
		else {
			monitor_names[index] = strdup("");
		}

		RAreaListAdd(areas, &cur_area);
	}
	monitor_names[index] = NULL;

	XRRFreeMonitors(ps_monitors);

	return RLayoutSetMonitorsNames(RLayoutNew(areas), monitor_names);
}
