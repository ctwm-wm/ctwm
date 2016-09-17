/*
 * Window util funcs
 */
#ifndef _CTWM_WIN_UTILS_H
#define _CTWM_WIN_UTILS_H


void GetWindowSizeHints(TwmWindow *tmp_win);
void FetchWmProtocols(TwmWindow *tmp);
void GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp);
TwmWindow *GetTwmWindow(Window w);


#endif /* _CTWM_WIN_UTILS_H */
