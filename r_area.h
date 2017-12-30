/*
 * Copyright notice...
 */

#ifndef _CTWM_R_AREA_H
#define _CTWM_R_AREA_H

#include "r_structs.h"


RArea *RAreaNew(int x, int y, int width, int height);
void RAreaNewIn(int x, int y, int width, int height, RArea *area);
RArea *RAreaCopy(RArea *self);

void RAreaFree(RArea *self);
int RAreaX2(RArea *self);
int RAreaY2(RArea *self);
int RAreaArea(RArea *self);
RArea *RAreaIntersect(RArea *self, RArea *other);
int RAreaIsIntersect(RArea *self, RArea *other);
RAreaList *RAreaHorizontalUnion(RArea *self, RArea *other);
RAreaList *RAreaVerticalUnion(RArea *self, RArea *other);

void RAreaPrint(RArea *self);

#endif  /* _CTWM_R_AREA_H */
