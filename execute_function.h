/*
 * f.whatever function dispatcher
 */

#ifndef _EXECUTE_FUNCTION_H
#define _EXECUTE_FUNCTION_H

/* All the outside world sees */
int ExecuteFunction(int func, void *action, Window w, TwmWindow *tmp_win,
                    XEvent *eventp, int context, int pulldown);


/* Leaks to a few places */
extern int RootFunction;

#endif /* _EXECUTE_FUNCTION_H */
