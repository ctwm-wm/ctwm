/*
 * Copyright notice...
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "r_area_list.h"
#include "r_area.h"


RAreaList *RAreaListNew(int cap, ...)
{
	va_list ap;
	RAreaList *list;
	RArea *area;

	list = malloc(sizeof(RAreaList));
	if(list == NULL) {
		abort();
	}
	list->len = 0;

	if(cap <= 0) {
		cap = 1;
	}
	list->cap = cap;
	list->areas = malloc(cap * sizeof(RArea));
	if(list->areas == NULL) {
		abort();
	}

	va_start(ap, cap);

	while((area = va_arg(ap, RArea *)) != NULL) {
		RAreaListAdd(list, area);
	}

	va_end(ap);

	return list;
}

RAreaList *RAreaListCopy(RAreaList *self)
{
	RAreaList *new = RAreaListNew(self->cap, NULL);

	RAreaListAddList(new, self);

	return new;
}

RAreaList *RAreaListCopyCropped(RAreaList *self, int left_margin,
                                int right_margin,
                                int top_margin, int bottom_margin)
{
	if(left_margin > 0 || right_margin > 0
	                || top_margin > 0 || bottom_margin > 0) {
		RArea *big_area = RAreaListBigArea(self);

		if(left_margin < 0) {
			left_margin = 0;
		}
		if(right_margin < 0) {
			right_margin = 0;
		}
		if(top_margin < 0) {
			top_margin = 0;
		}
		if(bottom_margin < 0) {
			bottom_margin = 0;
		}

		big_area->x += left_margin;
		big_area->width -= left_margin + right_margin;
		big_area->y += top_margin;
		big_area->height -= top_margin + bottom_margin;

		if(big_area->width <= 0 || big_area->height <= 0) {
			return RAreaListNew(0, NULL); // empty list
		}
		return RAreaListIntersectCrop(self, big_area);
	}
	return NULL; // no margin, no crop possible
}

void RAreaListFree(RAreaList *self)
{
	free(self->areas);
	free(self);
}

void RAreaListDelete(RAreaList *self, int index)
{
	if(index >= self->len) {
		return;
	}

	self->len--;

	if(index == self->len) {
		return;
	}

	memcpy(&self->areas[index], &self->areas[index + 1], (self->len - index) * sizeof(RArea));
}

void RAreaListAdd(RAreaList *self, RArea *area)
{
	if(self->cap == self->len) {
		RArea *new_list = realloc(self->areas, (self->cap + 1) * sizeof(RArea));
		if(new_list == NULL) {
			abort();
		}

		self->cap++;
		self->areas = new_list;
	}

	self->areas[self->len++] = *area;
}

void RAreaListAddList(RAreaList *self, RAreaList *other)
{
	if(self->cap - self->len < other->len) {
		RArea *new_list = realloc(self->areas,
		                          (self->len + other->len) * sizeof(RArea));
		if(new_list == NULL) {
			abort();
		}

		self->cap = self->len + other->len;
		self->areas = new_list;
	}

	memcpy(&self->areas[self->len], other->areas, other->len * sizeof(RArea));

	self->len += other->len;
}

static int _cmpX(const void *av, const void *bv)
{
	const RArea *a = (const RArea *)av, *b = (const RArea *)bv;

	if(a->x < b->x) {
		return -1;
	}

	if(a->x > b->x) {
		return 1;
	}

	return (a->y > b->y) - (a->y < b->y);
}

void RAreaListSortX(RAreaList *self)
{
	if(self->len <= 1) {
		return;
	}

	qsort(self->areas, self->len, sizeof(RArea), _cmpX);
}

static int _cmpY(const void *av, const void *bv)
{
	const RArea *a = (const RArea *)av, *b = (const RArea *)bv;

	if(a->y < b->y) {
		return -1;
	}

	if(a->y > b->y) {
		return 1;
	}

	return (a->x > b->x) - (a->x < b->x);
}

void RAreaListSortY(RAreaList *self)
{
	if(self->len <= 1) {
		return;
	}

	qsort(self->areas, self->len, sizeof(RArea), _cmpY);
}

RAreaList *RAreaListHorizontalUnion(RAreaList *self)
{
	RAreaList *copy = RAreaListCopy(self);
	int i, j;

refine:
	RAreaListSortX(copy);

	for(i = 0; i < copy->len - 1; i++) {
		for(j = i + 1; j < copy->len; j++) {
			RAreaList *repl = RAreaHorizontalUnion(&copy->areas[i], &copy->areas[j]);
			if(repl != NULL) {
				if(repl->len) {
					RAreaListDelete(copy, j);
					RAreaListDelete(copy, i);
					RAreaListAddList(copy, repl);
					RAreaListFree(repl);
					goto refine;
				}
				RAreaListFree(repl);
			}
		}
	}

	return copy;
}

RAreaList *RAreaListVerticalUnion(RAreaList *self)
{
	RAreaList *copy = RAreaListCopy(self);
	int i, j;

refine:
	RAreaListSortY(copy);

	for(i = 0; i < copy->len - 1; i++) {
		for(j = i + 1; j < copy->len; j++) {
			RAreaList *repl = RAreaVerticalUnion(&copy->areas[i], &copy->areas[j]);
			if(repl != NULL) {
				if(repl->len) {
					RAreaListDelete(copy, j);
					RAreaListDelete(copy, i);
					RAreaListAddList(copy, repl);
					RAreaListFree(repl);
					goto refine;
				}
				RAreaListFree(repl);
			}
		}
	}

	return copy;
}

RAreaList *RAreaListIntersect(RAreaList *self, RArea *area)
{
	RAreaList *new = RAreaListNew(self->len, NULL);
	int index;

	for(index = 0; index < self->len; index++) {
		if(RAreaIsIntersect(&self->areas[index], area)) {
			RAreaListAdd(new, &self->areas[index]);
		}
	}

	return new;
}

void RAreaListForeach(RAreaList *self,
                      int (*func)(RArea *cur_area, void *data),
                      void *data)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];

	while(cur_area < area_end) {
		if(func(cur_area++, data)) {
			break;
		}
	}
}

RAreaList *RAreaListIntersectCrop(RAreaList *self, RArea *area)
{
	RAreaList *new = RAreaListNew(self->len, NULL);
	RArea *it;
	int index;

	for(index = 0; index < self->len; index++) {
		it = RAreaIntersect(&self->areas[index], area);
		if(it != NULL) {
			RAreaListAdd(new, it);
			RAreaFree(it);
		}
	}

	return new;
}

RArea *RAreaListBigArea(RAreaList *self)
{
	RArea *area;
	int index, x, y, x2, y2;

	for(index = 0, area = self->areas; index < self->len; area++, index++) {
		if(index == 0 || area->x < x) {
			x = area->x;
		}

		if(index == 0 || area->y < y) {
			y = area->y;
		}

		if(index == 0 || RAreaX2(area) > x2) {
			x2 = RAreaX2(area);
		}

		if(index == 0 || RAreaY2(area) > y2) {
			y2 = RAreaY2(area);
		}
	}

	return RAreaNew(x, y, x2 - x + 1, y2 - y + 1);
}

RArea *RAreaListBestTarget(RAreaList *self, RArea *area)
{
	RArea *full_area = NULL, *it;
	int index, max_area = -1;

	for(index = 0; index < self->len; index++) {
		it = RAreaIntersect(area, &self->areas[index]);
		if(it != NULL && (max_area < 0 || RAreaArea(it) > max_area)) {
			max_area = RAreaArea(it);
			full_area = &self->areas[index];
		}
		RAreaFree(it);
	}

	return full_area != NULL ? RAreaCopy(full_area) : NULL;
}

int RAreaListMaxX(RAreaList *self)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];
	int max_x = self->len ? cur_area->x : 0;

	while(++cur_area < area_end)
		if(cur_area->x > max_x) {
			max_x = cur_area->x;
		}

	return max_x;
}

int RAreaListMaxY(RAreaList *self)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];
	int max_y = self->len ? cur_area->y : 0;

	while(++cur_area < area_end)
		if(cur_area->y > max_y) {
			max_y = cur_area->y;
		}

	return max_y;
}

int RAreaListMinX2(RAreaList *self)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];
	int min_x = self->len ? RAreaX2(cur_area) : 0;

	while(++cur_area < area_end)
		if(RAreaX2(cur_area) < min_x) {
			min_x = RAreaX2(cur_area);
		}

	return min_x;
}

int RAreaListMinY2(RAreaList *self)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];
	int min_y = self->len ? RAreaY2(cur_area) : 0;

	while(++cur_area < area_end)
		if(RAreaY2(cur_area) < min_y) {
			min_y = RAreaY2(cur_area);
		}

	return min_y;
}

void RAreaListPrint(RAreaList *self)
{
	RArea *cur_area = &self->areas[0], *area_end = &self->areas[self->len];
	fprintf(stderr, "[len=%d cap=%d", self->len, self->cap);
	while(cur_area < area_end) {
		fprintf(stderr, " ");
		RAreaPrint(cur_area++);
	}
	fprintf(stderr, "]");
}
