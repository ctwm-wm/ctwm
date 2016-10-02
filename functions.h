/*
 * f.whatever function dispatcher
 */

#ifndef _CTWM_FUNCTIONS_H
#define _CTWM_FUNCTIONS_H

/* All the outside world sees */
void ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                     XEvent *eventp, int context, bool pulldown);


typedef enum {
	MOVE_NONE,
	MOVE_VERT,
	MOVE_HORIZ,
} CMoveDir;

/* Needed in event_handlers.c */
extern bool ConstMove;
extern CMoveDir ConstMoveDir;
extern int ConstMoveX;
extern int ConstMoveY;

void draw_info_window(void);


/* Leaks to a few places */
extern int  RootFunction;
extern int  MoveFunction;
extern bool WindowMoved;
extern int  ResizeOrigX;
extern int  ResizeOrigY;

#endif /* _CTWM_FUNCTIONS_H */
