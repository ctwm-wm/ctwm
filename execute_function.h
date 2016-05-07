/*
 * f.whatever function dispatcher
 */

#ifndef _EXECUTE_FUNCTION_H
#define _EXECUTE_FUNCTION_H

/* All the outside world sees */
int ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                    XEvent *eventp, int context, int pulldown);

/* Needed in events.c */
extern int ConstMove;
extern int ConstMoveDir;
extern int ConstMoveX;
extern int ConstMoveY;
extern int ConstMoveXL;
extern int ConstMoveXR;
extern int ConstMoveYT;
extern int ConstMoveYB;

void draw_info_window(void);

extern int InfoLines;  /* [Ab]used as a "InfoWindow mapped" flag */

/* Leaks to a few places */
extern int RootFunction;
extern int MoveFunction;
extern int WindowMoved;
extern int ResizeOrigX;
extern int ResizeOrigY;

#endif /* _EXECUTE_FUNCTION_H */
