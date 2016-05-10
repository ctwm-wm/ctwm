/*
 * Window decoration bits -- initialization
 */

#ifndef _CTWM_DECORATIONS_INIT_H
#define _CTWM_DECORATIONS_INIT_H


void InitTitlebarButtons(void);
void SetCurrentTBAction(int button, int mods, int func, char *action,
                        MenuRoot *menuroot);
extern int CreateTitleButton(char *name, int func, char *action,
                             MenuRoot *menuroot, Bool rightside,
                             Bool append);



#endif /* _CTWM_DECORATIONS_INIT_H */
