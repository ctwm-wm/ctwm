/*
 * Shutdown (and restart) bits
 */
#ifndef _CTWM_SHUTDOWN_H
#define _CTWM_SHUTDOWN_H

void RestoreWithdrawnLocation(TwmWindow *tmp);
void Done(void) __attribute__((noreturn));
void DoRestart(Time t);         /* Function to perform a restart */


#endif /* _CTWM_SHUTDOWN_H */
