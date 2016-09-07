/*
 * Various internal bits shared among the event code
 */
#ifndef _CTWM_EVENT_INTERNAL_H
#define _CTWM_EVENT_INTERNAL_H

/* event_core.c */
Window WindowOfEvent(XEvent *e);
void SynthesiseFocusOut(Window w);
void SynthesiseFocusIn(Window w);

/* event_utils.c */
/* AutoRaiseWindow in events.h (temporarily?) */
void SetRaiseWindow(TwmWindow *tmp);
void AutoPopupMaybe(TwmWindow *tmp);
void AutoLowerWindow(TwmWindow *tmp);


extern TwmWindow *Tmp_win;
extern bool ColortableThrashing;
extern bool enter_flag;
extern bool leave_flag;
extern TwmWindow *enter_win, *raise_win, *leave_win, *lower_win;
extern TwmWindow *ButtonWindow;

#endif /* _CTWM_EVENT_INTERNAL_H */
