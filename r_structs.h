/*
 * Copyright notice...
 */

#ifndef _CTWM_R_STRUCTS_H
#define _CTWM_R_STRUCTS_H

typedef struct {
	int x;
	int y;
	int width;
	int height;
} RArea;

typedef struct {
	int len;
	int cap;
	RArea *areas;
} RAreaList;

typedef struct {
	RAreaList *monitors;
	RAreaList *horiz;
	RAreaList *vert;
} RLayout;

static inline int max(int a, int b)
{
	return a > b ? a : b;
}

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

#endif  /* _CTWM_R_STRUCTS_H */
