/*
 * Occupation handling routines
 */

#ifndef _CTWM_OCCUPATION_H
#define _CTWM_OCCUPATION_H


struct OccupyWindow {
	Window        w;
	TwmWindow     *twm_win;
	char          *geometry;
	Window        *obuttonw;
	Window        OK, cancel, allworkspc;
	int           width, height;
	char          *name;
	char          *icon_name;
	int           lines, columns;
	int           hspace, vspace;         /* space between workspaces */
	int           bwidth, bheight;
	int           owidth;                 /* oheight == bheight */
	ColorPair     cp;
	MyFont        font;
	int           tmpOccupation;
};


void SetupOccupation(TwmWindow *twm_win, int occupation_hint);
void Occupy(TwmWindow *twm_win);
void OccupyHandleButtonEvent(XEvent *event);
void OccupyAll(TwmWindow *twm_win);
void AddToWorkSpace(char *wname, TwmWindow *twm_win);
void RemoveFromWorkSpace(char *wname, TwmWindow *twm_win);
void ToggleOccupation(char *wname, TwmWindow *twm_win);
void ChangeOccupation(TwmWindow *tmp_win, int newoccupation);
void WmgrRedoOccupation(TwmWindow *win);
void WMgrRemoveFromCurrentWorkSpace(VirtualScreen *vs, TwmWindow *win);
void WMgrAddToCurrentWorkSpaceAndWarp(VirtualScreen *vs, char *winname);

void CreateOccupyWindow(void);
void PaintOccupyWindow(void);
void ResizeOccupyWindow(TwmWindow *win);

unsigned int GetMaskFromProperty(unsigned char *prop, unsigned long len);
int GetPropertyFromMask(unsigned int mask, char **prop);

bool AddToClientsList(char *workspace, char *client);

void MoveToNextWorkSpace(VirtualScreen *vs, TwmWindow *twm_win);
void MoveToPrevWorkSpace(VirtualScreen *vs, TwmWindow *twm_win);
void MoveToNextWorkSpaceAndFollow(VirtualScreen *vs, TwmWindow *twm_win);
void MoveToPrevWorkSpaceAndFollow(VirtualScreen *vs, TwmWindow *twm_win);


extern int fullOccupation;

/* Hopefully temporary; x-ref comment in .c */
extern TwmWindow *occupyWin;

#endif // _CTWM_OCCUPATION_H
