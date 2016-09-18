/*
 * Window util funcs
 */
#ifndef _CTWM_WIN_UTILS_H
#define _CTWM_WIN_UTILS_H


void GetWindowSizeHints(TwmWindow *tmp_win);
void FetchWmProtocols(TwmWindow *tmp);
void GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp);
TwmWindow *GetTwmWindow(Window w);
unsigned char *GetWMPropertyString(Window w, Atom prop);
void FreeWMPropertyString(char *prop);
bool visible(const TwmWindow *tmp_win);


#endif /* _CTWM_WIN_UTILS_H */
