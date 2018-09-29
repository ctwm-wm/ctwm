/*
 * Copyright notice...
 */

#include "ctwm.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "r_layout.h"
#include "r_area_list.h"
#include "r_area.h"
#include "util.h"


/**
 * Create an RLayout for a given set of monitors.
 *
 * This stashes up the list of monitors, and precalculates the
 * horizontal/vertical stripes that compose it.
 */
RLayout *
RLayoutNew(RAreaList *monitors)
{
	RLayout *layout = malloc(sizeof(RLayout));
	if(layout == NULL) {
		abort();
	}

	layout->monitors = monitors;
	layout->horiz = RAreaListHorizontalUnion(monitors);
	layout->vert = RAreaListVerticalUnion(monitors);
	layout->names = NULL;

	return layout;
}


/**
 * Create a copy of an RLayout with given amounts cropped off the sides.
 * This is used anywhere we need to pretend our display area is smaller
 * than it actually is (e.g., via the BorderBottom/Top/Left/Right config
 * params)
 */
RLayout *
RLayoutCopyCropped(RLayout *self, int left_margin, int right_margin,
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


/**
 * Clean up and free any RLayout.names there might be in an RLayout.
 */
static void
_RLayoutFreeNames(RLayout *self)
{
	if(self->names != NULL) {
		free(self->names);
		self->names = NULL;
	}
}


/**
 * Clean up and free an RLayout.
 */
void
RLayoutFree(RLayout *self)
{
	RAreaListFree(self->monitors);
	RAreaListFree(self->horiz);
	RAreaListFree(self->vert);
	_RLayoutFreeNames(self);
	free(self);
}


/**
 * Set the names for our monitors in an RLayout.  This is only used for
 * the RLayout that describes our complete monitor layout, which fills in
 * the RANDR names for each output.
 */
RLayout *
RLayoutSetMonitorsNames(RLayout *self, char **names)
{
	_RLayoutFreeNames(self);
	self->names = names;
	return self;
}


/**
 * Given an RArea that doesn't reside in any of the areas in our RLayout,
 * create an maximally-tall RArea that is and that the window could go
 * into, and figure where that would fit in our RLayout.
 *
 * This is the vertical-stripe-returning counterpart of
 * _RLayoutRecenterHorizontally().
 *
 * This will create an RArea that's always the width of far_area, moved
 * horizontally as little as possible to ensure that the left or right
 * edge is on-screen, and the full height of our RLayout.  That area is
 * then RAreaListIntersect()'ed with self, yielding the set of vertical
 * stripes in self that new position will be in.
 *
 * This is called only by _RLayoutVerticalIntersect() when given an RArea
 * that doesn't already intersect the RLayout.  Will probably not tell
 * you something useful if given a far_area that already _does_ intersect
 * self.
 *
 * \param self     Our current monitor layout
 * \param far_area The area to act on
 */
static RAreaList *
_RLayoutRecenterVertically(RLayout *self, RArea *far_area)
{
	//  |_V_|
	//  |   |
	// L|   |R
	//  |___|
	//  | V |
	RArea big = RAreaListBigArea(self->monitors), tmp;

	// Where did it wind up?
	if((far_area->x >= big.x && far_area->x <= RAreaX2(&big))
	                || (RAreaX2(far_area) >= big.x && RAreaX2(far_area) <= RAreaX2(&big))) {
		// In one of the V areas.  It's already in a horizontal position
		// that would fit, so keep that.  Come up with a new area of that
		// horizontal pos/width, which vertically covers the full height
		// of self.
		tmp = RAreaNew(far_area->x, big.y,
		               far_area->width, big.height);
	}
	else if(RAreaX2(far_area) < big.x) {
		// Off to the left side, so make an area that moves it over far
		// enough to just eek into self.
		tmp = RAreaNew(big.x - far_area->width + 1, big.y,
		               far_area->width, big.height);
	}
	else {
		// Off to the right side, so move it over just enough for the
		// left edge to be on-screen.
		tmp = RAreaNew(RAreaX2(&big), big.y,
		               far_area->width, big.height);
	}

	// Intersect that new vertical stripe with our screen
	return RAreaListIntersect(self->vert, &tmp);
}


/**
 * Given an RArea that doesn't reside in any of the areas in our RLayout,
 * create an maximally-wide RArea that is and that the window could go
 * into, and figure where that would fit in our RLayout.
 *
 * This is the horizontal-stripe-returning counterpart of
 * _RLayoutRecenterVertically().
 *
 * This will create an RArea that's always the height of far_area, moved
 * veritcally as little as possible to ensure that the top or bottom edge
 * is on-screen, and the full width of our RLayout.  That area is then
 * RAreaListIntersect()'ed with self, yielding the set of horizontal
 * stripes in self that new position will be in.
 *
 * This is called only by _RLayoutHorizontalIntersect() and
 * RLayoutFull1() when given an RArea that doesn't already intersect the
 * RLayout.  Will probably not tell you something useful if given a
 * far_area that already _does_ intersect self.
 *
 * \param self     Our current monitor layout
 * \param far_area The area to act on
 */
static RAreaList *
_RLayoutRecenterHorizontally(RLayout *self, RArea *far_area)
{
	// ___T___
	//  |   |
	// H|   |H
	// _|___|_
	//    B
	RArea big = RAreaListBigArea(self->monitors), tmp;

	// Where is it?
	if((far_area->y >= big.y && far_area->y <= RAreaY2(&big))
	                || (RAreaY2(far_area) >= big.y && RAreaY2(far_area) <= RAreaY2(&big))) {
		// In one of the H areas.  Already in a valid place vertically,
		// so create a horizontal stripe there.
		tmp = RAreaNew(big.x, far_area->y,
		               big.width, far_area->height);
	}
	else if(RAreaY2(far_area) < big.y) {
		// Off the top (T); make a horizontal stripe low enough that the
		// bottom of far_area winds up on-screen.
		tmp = RAreaNew(big.x, big.y - far_area->height + 1,
		               big.width, far_area->height);
	}
	else {
		// Off the bottom (B); make the stripe high enough for the top of
		// far_area to peek on-screen.
		tmp = RAreaNew(big.x, RAreaY2(&big),
		               big.width, far_area->height);
	}

	// And find which horizontal stripes of the screen that falls into.
	return RAreaListIntersect(self->horiz, &tmp);
}


/**
 * Find which vertical regions of our monitor layout a given RArea (often
 * a window) is in.  If it's not in any, shift it horizontally until it'd
 * be inside in that dimension, and return the vertical stripes that
 * horizontal extent would be in.
 *
 * Note that for our purposes, it doesn't matter whether the RArea is
 * _vertically_ within the RLayout.  This function is used only by
 * RLayoutFindTopBottomEdges()
 */
static RAreaList *
_RLayoutVerticalIntersect(RLayout *self, RArea *area)
{
	RAreaList *mit = RAreaListIntersect(self->vert, area);

	if(mit->len == 0) {
		// Not in any of the areas; find the nearest horizontal shift to
		// put in in one.
		RAreaListFree(mit);
		mit = _RLayoutRecenterVertically(self, area);
	}
	return mit;
}


/**
 * Find which horizontal regions of our monitor layout a given RArea
 * (often a window) is in.  If it's not in any, shift it vertically until
 * it'd be inside in that dimension, and return the horizontal stripes
 * that vertical extent would be in.
 *
 * Note that for our purposes, it doesn't matter whether the RArea is
 * _horizontally_ within the RLayout.  This function is used only by
 * RLayoutFindLeftRightEdges()
 */
static RAreaList *
_RLayoutHorizontalIntersect(RLayout *self, RArea *area)
{
	RAreaList *mit = RAreaListIntersect(self->horiz, area);

	if(mit->len == 0) {
		RAreaListFree(mit);

		// Out of screen, recenter the window
		mit = _RLayoutRecenterHorizontally(self, area);
	}

	return mit;
}


/**
 * Figure the position (or nearest practical position) of an area in our
 * screen layout, and return into about the bottom/top stripes it fits
 * into.
 *
 * Note that the return values (params) are slightly counterintuitive;
 * top tells you where the top of the lowest stripe that area intersects
 * with is, and bottom tells you the bottom of the highest.  This is used
 * as a backend piece of various calculations trying to be sure something
 * winds up on-screen.
 *
 * \param[in]  self   The monitor layout to work from
 * \param[in]  area   The area to be fit into the monitors
 * \param[out] top    The top of the lowest stripe area fits into.
 * \param[out] bottom The bottom of the higher stripe area fits into.
 */
void
RLayoutFindTopBottomEdges(RLayout *self, RArea *area, int *top,
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


/**
 * Find the bottom of the top stripe of self that area fits into.  A
 * shortcut to get only the second return value of
 * RLayoutFindTopBottomEdges().
 */
int
RLayoutFindBottomEdge(RLayout *self, RArea *area)
{
	int min_y2;
	RLayoutFindTopBottomEdges(self, area, NULL, &min_y2);
	return min_y2;
}


/**
 * Find the top of the bottom stripe of self that area fits into.  A
 * shortcut to get only the first return value of
 * RLayoutFindTopBottomEdges().
 */
int
RLayoutFindTopEdge(RLayout *self, RArea *area)
{
	int max_y;
	RLayoutFindTopBottomEdges(self, area, &max_y, NULL);
	return max_y;
}


/**
 * Figure the position (or nearest practical position) of an area in our
 * screen layout, and return into about the left/rightmost stripes it fits
 * into.
 *
 * As with RLayoutFindTopBottomEdges(), the return values (params) are
 * slightly counterintuitive.  left tells you where the left-side of the
 * right-most stripe that area intersects with is, and right tells you
 * the right side of the left-most.  This is used as a backend piece of
 * various calculations trying to be sure something winds up on-screen.
 *
 * \param[in]  self   The monitor layout to work from
 * \param[in]  area   The area to be fit into the monitors
 * \param[out] left   The left edge of the right-most stripe area fits into.
 * \param[out] right  The right edge of the left-most stripe area fits into.
 */
void
RLayoutFindLeftRightEdges(RLayout *self, RArea *area, int *left,
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


/**
 * Find the left edge of the right-most stripe of self that area fits
 * into.  A shortcut to get only the first return value of
 * RLayoutFindLeftRightEdges().
 */
int
RLayoutFindLeftEdge(RLayout *self, RArea *area)
{
	int max_x;
	RLayoutFindLeftRightEdges(self, area, &max_x, NULL);
	return max_x;
}


/**
 * Find the right edge of the left-most stripe of self that area fits
 * into.  A shortcut to get only the second return value of
 * RLayoutFindLeftRightEdges().
 */
int
RLayoutFindRightEdge(RLayout *self, RArea *area)
{
	int min_x2;
	RLayoutFindLeftRightEdges(self, area, NULL, &min_x2);
	return min_x2;
}


struct monitor_finder_xy {
	RArea *area;
	int x, y;
};

static int _findMonitorByXY(RArea *cur, void *vdata)
{
	struct monitor_finder_xy *data = (struct monitor_finder_xy *)vdata;

	if(RAreaContainsXY(cur, data->x, data->y)) {
		data->area = cur;
		return 1;
	}
	return 0;
}

RArea RLayoutGetAreaAtXY(RLayout *self, int x, int y)
{
	struct monitor_finder_xy data = { NULL, x, y };

	RAreaListForeach(self->monitors, _findMonitorByXY, &data);

	return data.area == NULL ? self->monitors->areas[0] : *data.area;
}

RArea RLayoutGetAreaIndex(RLayout *self, int index)
{
	if(index >= self->monitors->len) {
		index = self->monitors->len - 1;
	}

	return self->monitors->areas[index];
}

RArea RLayoutGetAreaByName(RLayout *self, const char *name, int len)
{
	if(self->names != NULL) {
		int index;

		if(len < 0) {
			len = strlen(name);
		}

		for(index = 0; index < self->monitors->len
		                && self->names[index] != NULL; index++) {
			if(strncmp(self->names[index], name, len) == 0) {
				return self->monitors->areas[index];
			}
		}
	}

	return RAreaInvalid();
}


/**
 * Generate maximal spanning RArea.
 *
 * This is a trivial wrapper of RAreaListBigArea() to hide knowledge of
 * RLayout internals.  Currently only used once; maybe should just be
 * deref'd there...
 */
RArea RLayoutBigArea(RLayout *self)
{
	return RAreaListBigArea(self->monitors);
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

static int _findMonitorBottomEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && RAreaY2(cur) > RAreaY2(data->area)
	                && (!data->found || RAreaY2(cur) < data->u.min_y2)) {
		data->u.min_y2 = RAreaY2(cur);
		data->found = 1;
	}
	return 0;
}

int RLayoutFindMonitorBottomEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorBottomEdge, &data);

	return data.found ? data.u.min_y2 : RLayoutFindBottomEdge(self, area);
}

static int _findMonitorTopEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && cur->y < data->area->y
	                && (!data->found || cur->y > data->u.max_y)) {
		data->u.max_y = cur->y;
		data->found = 1;
	}
	return 0;
}

int RLayoutFindMonitorTopEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorTopEdge, &data);

	return data.found ? data.u.max_y : RLayoutFindTopEdge(self, area);
}

static int _findMonitorLeftEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && cur->x < data->area->x
	                && (!data->found || cur->x > data->u.max_x)) {
		data->u.max_x = cur->x;
		data->found = 1;
	}
	return 0;
}

int RLayoutFindMonitorLeftEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorLeftEdge, &data);

	return data.found ? data.u.max_x : RLayoutFindLeftEdge(self, area);
}

static int _findMonitorRightEdge(RArea *cur, void *vdata)
{
	struct monitor_edge_finder *data = (struct monitor_edge_finder *)vdata;

	// Does the area intersect the current list area
	if(RAreaIsIntersect(cur, data->area)
	                && RAreaX2(cur) > RAreaX2(data->area)
	                && (!data->found || RAreaX2(cur) < data->u.min_x2)) {
		data->u.min_x2 = RAreaX2(cur);
		data->found = 1;
	}
	return 0;
}

int RLayoutFindMonitorRightEdge(RLayout *self, RArea *area)
{
	struct monitor_edge_finder data = { area };

	RAreaListForeach(self->monitors, _findMonitorRightEdge, &data);

	return data.found ? data.u.min_x2 : RLayoutFindRightEdge(self, area);
}

RArea RLayoutFullHoriz(RLayout *self, RArea *area)
{
	int max_x, min_x2;

	RLayoutFindLeftRightEdges(self, area, &max_x, &min_x2);

	return RAreaNew(max_x, area->y, min_x2 - max_x + 1, area->height);
}

RArea RLayoutFullVert(RLayout *self, RArea *area)
{
	int max_y, min_y2;

	RLayoutFindTopBottomEdges(self, area, &max_y, &min_y2);

	return RAreaNew(area->x, max_y, area->width, min_y2 - max_y + 1);
}

RArea RLayoutFull(RLayout *self, RArea *area)
{
	RArea full_horiz, full_vert, full1, full2;

	full_horiz = RLayoutFullHoriz(self, area);
	full_vert = RLayoutFullVert(self, area);

	full1 = RLayoutFullVert(self, &full_horiz);
	full2 = RLayoutFullHoriz(self, &full_vert);

	return RAreaArea(&full1) > RAreaArea(&full2) ? full1 : full2;
}

RArea RLayoutFullHoriz1(RLayout *self, RArea *area)
{
	RArea target = RLayoutFull1(self, area);
	int max_y, min_y2;

	max_y = max(area->y, target.y);
	min_y2 = min(RAreaY2(area), RAreaY2(&target));

	// target.x OK
	target.y = max_y;
	// target.width OK
	target.height = min_y2 - max_y + 1;

	return target;
}

RArea RLayoutFullVert1(RLayout *self, RArea *area)
{
	RArea target = RLayoutFull1(self, area);
	int max_x, min_x2;

	max_x = max(area->x, target.x);
	min_x2 = min(RAreaX2(area), RAreaX2(&target));

	target.x = max_x;
	// target.y OK
	target.width = min_x2 - max_x + 1;
	// target.height OK

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

RArea RLayoutFull1(RLayout *self, RArea *area)
{
	RArea target;
	RAreaList *mit = RAreaListIntersect(self->monitors, area);

	if(mit->len == 0) {
		RAreaListFree(mit);

		// Out of screen, recenter the window
		mit = _RLayoutRecenterHorizontally(self, area);
	}

	target = RAreaListBestTarget(mit, area);
	RAreaListFree(mit);
	return target;
}

void RLayoutPrint(RLayout *self)
{
	fprintf(stderr, "[monitors=");
	RAreaListPrint(self->monitors);
	fprintf(stderr, "\n horiz=");
	RAreaListPrint(self->horiz);
	fprintf(stderr, "\n vert=");
	RAreaListPrint(self->vert);
	fprintf(stderr, "]\n");
}
