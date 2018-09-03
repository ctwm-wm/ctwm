/*
 * Copyright notice...
 */

#ifndef _CTWM_R_STRUCTS_H
#define _CTWM_R_STRUCTS_H

struct RArea {
	int x;
	int y;
	int width;
	int height;
};

struct RAreaList {
	int len;
	int cap;
	RArea *areas;
};


struct RLayout {
	RAreaList *monitors;
	RAreaList *horiz;
	RAreaList *vert;
	char **names;
};

static inline int max(int a, int b)
{
	return a > b ? a : b;
}

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

#endif  /* _CTWM_R_STRUCTS_H */
