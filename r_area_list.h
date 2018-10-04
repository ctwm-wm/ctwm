/*
 * Copyright notice...
 */

#ifndef _CTWM_R_AREA_LIST_H
#define _CTWM_R_AREA_LIST_H

#include "r_structs.h"


RAreaList *RAreaListNew(int cap, ...);

void RAreaListFree(RAreaList *self);

RAreaList *RAreaListCopyCropped(RAreaList *self, int left_margin,
                                int right_margin,
                                int top_margin, int bottom_margin);

void RAreaListAdd(RAreaList *self, RArea *area);

RAreaList *RAreaListHorizontalUnion(RAreaList *self);
RAreaList *RAreaListVerticalUnion(RAreaList *self);

RAreaList *RAreaListIntersect(RAreaList *self, RArea *area);
void RAreaListForeach(RAreaList *self, bool (*func)(RArea *area, void *data),
                      void *data);

RArea RAreaListBigArea(RAreaList *self);
RArea RAreaListBestTarget(RAreaList *self, RArea *area);

int RAreaListMaxX(RAreaList *self);
int RAreaListMaxY(RAreaList *self);
int RAreaListMinX2(RAreaList *self);
int RAreaListMinY2(RAreaList *self);

void RAreaListPrint(RAreaList *self);

#endif  /* _CTWM_R_AREA_LIST_H */
