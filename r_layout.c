/*
 * Copyright notice...
 */

#include <stdlib.h>
#include <stdio.h>

#include "r_layout.h"
#include "r_area_list.h"
#include "r_area.h"


RLayout *RLayoutNew(RAreaList *monitors)
{
	RLayout *layout = malloc(sizeof(RLayout));
	if(layout == NULL) {
		abort();
	}

	layout->monitors = monitors;
	layout->horiz = RAreaListHorizontalUnion(monitors);
	layout->vert = RAreaListVerticalUnion(monitors);

	return layout;
}

RLayout *RLayoutCopyCropped(RLayout *self, int left_margin, int right_margin,
                            int top_margin, int bottom_margin)
{
	RAreaList *cropped_monitors = RAreaListCopyCropped(self->monitors,
	                              left_margin, right_margin,
	                              top_margin, bottom_margin);
	if(cropped_monitors == NULL) {
		return NULL;        // nothing to crop, same layout as passed
	}

	return RLayoutNew(cropped_monitors);
}

void RLayoutFree(RLayout *self)
{
	RAreaListFree(self->monitors);
	RAreaListFree(self->horiz);
	RAreaListFree(self->vert);
	free(self);
}

static RAreaList *_RLayoutRecenterVertically(RLayout *self, RArea *far_area)
{
	//  |_V_|
	//  |   |
	// L|   |R
	//  |___|
	//  | V |
	RAreaList *mit;
	RArea *big = RAreaListBigArea(self->monitors), *tmp;

	// In one of V areas?
	if((far_area->x >= big->x && far_area->x <= RAreaX2(big))
	                || (RAreaX2(far_area) >= big->x && RAreaX2(far_area) <= RAreaX2(big))) {
		// Take it back vertically
		tmp = RAreaNew(far_area->x, big->y,
		               far_area->width, big->height);
	}
	// On left? (L area)
	else if(RAreaX2(far_area) < big->x) {
		// Take it back vertically with its right border at pos big->x
		tmp = RAreaNew(big->x - far_area->width + 1, big->y,
		               far_area->width, big->height);
	}
	// On right (R area)
	else {
		// Take it back vertically with its left border at pos RAreaX2(big)
		tmp = RAreaNew(RAreaX2(big), big->y,
		               far_area->width, big->height);
	}
	mit = RAreaListIntersect(self->vert, tmp);
	RAreaFree(tmp);

	return mit;
}

static RAreaList *_RLayoutRecenterHorizontally(RLayout *self, RArea *far_area)
{
	// ___T___
	//  |   |
	// H|   |H
	// _|___|_
	//    B
	RAreaList *mit;
	RArea *big = RAreaListBigArea(self->monitors), *tmp;

	// In one of H areas?
	if((far_area->y >= big->y && far_area->y <= RAreaY2(big))
	                || (RAreaY2(far_area) >= big->y && RAreaY2(far_area) <= RAreaY2(big))) {
		// Take it back horizontally
		tmp = RAreaNew(big->x, far_area->y,
		               big->width, far_area->height);
	}
	// On top? (T area)
	else if(RAreaY2(far_area) < big->y) {
		// Take it back horizontally with its bottom border at pos big->y
		tmp = RAreaNew(big->x, big->y - far_area->height + 1,
		               big->width, far_area->height);
	}
	// On bottom (B areas)
	else {
		// Take it back horizontally with its top border at pos RAreaY2(big)
		tmp = RAreaNew(big->x, RAreaY2(big),
		               big->width, far_area->height);
	}

	mit = RAreaListIntersect(self->horiz, tmp);
	RAreaFree(tmp);

	return mit;
}

static RAreaList *_RLayoutVerticalIntersect(RLayout *self, RArea *area)
{
	RAreaList *mit = RAreaListIntersect(self->vert, area);

	if(mit->len == 0) {
		RAreaListFree(mit);

		// Out of screen, recenter the window
		mit = _RLayoutRecenterVertically(self, area);
	}
	return mit;
}

static RAreaList *_RLayoutHorizontalIntersect(RLayout *self, RArea *area)
{
	RAreaList *mit = RAreaListIntersect(self->horiz, area);

	if(mit->len == 0) {
		RAreaListFree(mit);

		// Out of screen, recenter the window
		mit = _RLayoutRecenterHorizontally(self, area);
	}

	return mit;
}

void RLayoutFindTopBottomEdges(RLayout *self, RArea *area, int *top,
                               int *bottom)
{
	RAreaList *mit = _RLayoutVerticalIntersect(self, area);

	if(top != NULL) {
		*top = RAreaListMaxY(mit);
	}

	if(bottom != NULL) {
		*bottom = RAreaListMinY2(mit);
	}

	RAreaListFree(mit);
}

int RLayoutFindBottomEdge(RLayout *self, RArea *area)
{
	int min_y2;
	RLayoutFindTopBottomEdges(self, area, NULL, &min_y2);
	return min_y2;
}

int RLayoutFindTopEdge(RLayout *self, RArea *area)
{
	int max_y;
	RLayoutFindTopBottomEdges(self, area, &max_y, NULL);
	return max_y;
}

void RLayoutFindLeftRightEdges(RLayout *self, RArea *area, int *left,
                               int *right)
{
	RAreaList *mit = _RLayoutHorizontalIntersect(self, area);

	if(left != NULL) {
		*left = RAreaListMaxX(mit);
	}

	if(right != NULL) {
		*right = RAreaListMinX2(mit);
	}

	RAreaListFree(mit);
}

int RLayoutFindLeftEdge(RLayout *self, RArea *area)
{
	int max_x;
	RLayoutFindLeftRightEdges(self, area, &max_x, NULL);
	return max_x;
}

int RLayoutFindRightEdge(RLayout *self, RArea *area)
{
	int min_x2;
	RLayoutFindLeftRightEdges(self, area, NULL, &min_x2);
	return min_x2;
}

struct monitor_edge_finder {
	RArea *area;
	union {
		int max_x;
		int max_y;
		int min_x2;
		int min_y2;
	} u;
	int found;
};

static void _findMonitorBottomEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && RAreaY2(cur) > RAreaY2(data->area)
	                && (!data->found || RAreaY2(cur) < data->u.min_y2)) {
		data->u.min_y2 = RAreaY2(cur);
		data->found = 1;
	}
}

int RLayoutFindMonitorBottomEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorBottomEdge, &data);

	return data.found ? data.u.min_y2 : RLayoutFindBottomEdge(self, area);
}

static void _findMonitorTopEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && cur->y < data->area->y
	                && (!data->found || cur->y > data->u.max_y)) {
		data->u.max_y = cur->y;
		data->found = 1;
	}
}

int RLayoutFindMonitorTopEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorTopEdge, &data);

	return data.found ? data.u.max_y : RLayoutFindTopEdge(self, area);
}

static void _findMonitorLeftEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && cur->x < data->area->x
	                && (!data->found || cur->x > data->u.max_x)) {
		data->u.max_x = cur->x;
		data->found = 1;
	}
}

int RLayoutFindMonitorLeftEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorLeftEdge, &data);

	return data.found ? data.u.max_x : RLayoutFindLeftEdge(self, area);
}

static void _findMonitorRightEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	printf("  > cur=");
	RAreaPrint(cur);
	printf(" vs win=");
	RAreaPrint(data->area);
	if(RAreaIsIntersect(cur, data->area)
	                && RAreaX2(cur) > RAreaX2(data->area)
	                && (!data->found || RAreaX2(cur) < data->u.min_x2)) {
		data->u.min_x2 = RAreaX2(cur);
		data->found = 1;
		printf(" min_x2=%d", data->u.min_x2);
	}
	printf(" (%d/%d/%d)\n", RAreaIsIntersect(cur, data->area), data->u.min_x2,
	       data->found);
}

int RLayoutFindMonitorRightEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	printf("RLayoutFindMonitorRightEdge ");
	RLayoutPrint(self);
	RAreaListForeach(self->monitors, _findMonitorRightEdge, &data);

	return data.found ? data.u.min_x2 : RLayoutFindRightEdge(self, area);
}

RArea *RLayoutFullHoriz(RLayout *self, RArea *area)
{
	int max_x, min_x2;

	RLayoutFindLeftRightEdges(self, area, &max_x, &min_x2);

	return RAreaNew(max_x, area->y, min_x2 - max_x + 1, area->height);
}

RArea *RLayoutFullVert(RLayout *self, RArea *area)
{
	int max_y, min_y2;

	RLayoutFindTopBottomEdges(self, area, &max_y, &min_y2);

	return RAreaNew(area->x, max_y, area->width, min_y2 - max_y + 1);
}

RArea *RLayoutFull(RLayout *self, RArea *area)
{
	RArea *full_horiz, *full_vert, *full1, *full2;

	full_horiz = RLayoutFullHoriz(self, area);
	full_vert = RLayoutFullVert(self, area);

	full1 = RLayoutFullVert(self, full_horiz);
	full2 = RLayoutFullHoriz(self, full_vert);

	RAreaFree(full_horiz);
	RAreaFree(full_vert);

	if(RAreaArea(full1) > RAreaArea(full2)) {
		RAreaFree(full2);
		return full1;
	}

	RAreaFree(full1);
	return full2;
}

RArea *RLayoutFullHoriz1(RLayout *self, RArea *area)
{
	RArea *target = RLayoutFull1(self, area);
	int max_y, min_y2;

	max_y = max(area->y, target->y);
	min_y2 = min(RAreaY2(area), RAreaY2(target));

	// target->x OK
	target->y = max_y;
	// target->width OK
	target->height = min_y2 - max_y + 1;

	return target;
}

RArea *RLayoutFullVert1(RLayout *self, RArea *area)
{
	RArea *target = RLayoutFull1(self, area);
	int max_x, min_x2;

	max_x = max(area->x, target->x);
	min_x2 = min(RAreaX2(area), RAreaX2(target));

	target->x = max_x;
	// target->y OK
	target->width = min_x2 - max_x + 1;
	// target->height OK

	return target;
}

/***********************************************************************
 *
 *  Procedure:
 *      RLayoutFull1 - resize the area to fill only one monitor
 *
 *  Returned Value:
 *      new resized area (need to be freed)
 *
 *  Inputs:
 *      self    - layout
 *      area    - area to resize
 *
 ***********************************************************************
 */

RArea *RLayoutFull1(RLayout *self, RArea *area)
{
	RAreaList *mit = RAreaListIntersect(self->monitors, area);

	if(mit->len == 0) {
		RAreaListFree(mit);

		// Out of screen, recenter the window
		mit = _RLayoutRecenterHorizontally(self, area);
	}

	area = RAreaListBestTarget(mit, area);
	RAreaListFree(mit);
	return area;
}

void RLayoutPrint(RLayout *self)
{
	printf("[monitors=");
	RAreaListPrint(self->monitors);
	printf("\n horiz=");
	RAreaListPrint(self->horiz);
	printf("\n vert=");
	RAreaListPrint(self->vert);
	printf("]\n");
}
