/*
 * Copyright notice...
 */

#ifndef _CTWM_R_AREA_LIST_H
#define _CTWM_R_AREA_LIST_H

#include "r_structs.h"


RAreaList *RAreaListNew(int cap, ...);
RAreaList *RAreaListCopy(RAreaList *self);

void RAreaListFree(RAreaList *self);

RAreaList *RAreaListCopyCropped(RAreaList *self, int left_margin,
                                int right_margin,
                                int top_margin, int bottom_margin);

void RAreaListDelete(RAreaList *self, int index);
void RAreaListAdd(RAreaList *self, RArea *area);
void RAreaListAddList(RAreaList *self, RAreaList *other);

void RAreaListSortX(RAreaList *self);
void RAreaListSortY(RAreaList *self);

RAreaList *RAreaListHorizontalUnion(RAreaList *self);
RAreaList *RAreaListVerticalUnion(RAreaList *self);

RAreaList *RAreaListIntersect(RAreaList *self, RArea *area);
RAreaList *RAreaListIntersectCrop(RAreaList *self, RArea *area);
RArea *RAreaListBigArea(RAreaList *self);
RArea *RAreaListBestTarget(RAreaList *self, RArea *area);

int RAreaListMaxX(RAreaList *self);
int RAreaListMaxY(RAreaList *self);
int RAreaListMinX2(RAreaList *self);
int RAreaListMinY2(RAreaList *self);

void RAreaListPrint(RAreaList *self);

#endif  /* _CTWM_R_AREA_LIST_H */
