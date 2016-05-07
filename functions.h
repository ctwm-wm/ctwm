/*
 * f.whatever function dispatcher
 */

#ifndef _EXECUTE_FUNCTION_H
#define _EXECUTE_FUNCTION_H

#include <stdbool.h>

/* All the outside world sees */
bool ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                     XEvent *eventp, int context, int pulldown);

/* Needed in events.c */
extern int ConstMove;
extern int ConstMoveDir;
extern int ConstMoveX;
extern int ConstMoveY;

void draw_info_window(void);


/* Leaks to a few places */
extern int  RootFunction;
extern int  MoveFunction;
extern bool WindowMoved;
extern int  ResizeOrigX;
extern int  ResizeOrigY;

#endif /* _EXECUTE_FUNCTION_H */
