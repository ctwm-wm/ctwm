/*
 * Copyright notice...
 */

#ifndef _CTWM_R_AREA_H
#define _CTWM_R_AREA_H

#include "r_structs.h"


RArea *RAreaNewStatic(int x, int y, int width, int height);
RArea RAreaNew(int x, int y, int width, int height);

RArea RAreaInvalid(void);
bool RAreaIsValid(RArea *self);

int RAreaX2(RArea *self);
int RAreaY2(RArea *self);
int RAreaArea(RArea *self);
RArea RAreaIntersect(RArea *self, RArea *other);
bool RAreaIsIntersect(RArea *self, RArea *other);
bool RAreaContainsXY(RArea *self, int x, int y);
RAreaList *RAreaHorizontalUnion(RArea *self, RArea *other);
RAreaList *RAreaVerticalUnion(RArea *self, RArea *other);

void RAreaPrint(RArea *self);

#endif  /* _CTWM_R_AREA_H */
