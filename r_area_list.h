/*
 * Copyright notice...
 */

#ifndef _CTWM_R_AREA_LIST_H
#define _CTWM_R_AREA_LIST_H

#include "r_structs.h"


RAreaList *RAreaListNew(int cap, ...);

void RAreaListFree(RAreaList *self);

RAreaList *RAreaListCopyCropped(const RAreaList *self, int left_margin,
                                int right_margin,
                                int top_margin, int bottom_margin);

void RAreaListAdd(RAreaList *self, const RArea *area);

RAreaList *RAreaListHorizontalUnion(const RAreaList *self);
RAreaList *RAreaListVerticalUnion(const RAreaList *self);

RAreaList *RAreaListIntersect(const RAreaList *self, const RArea *area);
void RAreaListForeach(const RAreaList *self,
                      bool (*func)(RArea *area, void *data),
                      void *data);

RArea RAreaListBigArea(const RAreaList *self);
RArea RAreaListBestTarget(const RAreaList *self, const RArea *area);

int RAreaListMaxX(const RAreaList *self);
int RAreaListMaxY(const RAreaList *self);
int RAreaListMinX2(const RAreaList *self);
int RAreaListMinY2(const RAreaList *self);

void RAreaListPrint(const RAreaList *self);

#endif  /* _CTWM_R_AREA_LIST_H */
