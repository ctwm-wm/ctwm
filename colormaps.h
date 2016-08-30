/*
 * Colormap handling funcs
 */
#ifndef _CTWM_COLORMAPS_H
#define _CTWM_COLORMAPS_H


int InstallWindowColormaps(int type, TwmWindow *tmp);
int InstallColormaps(int type, Colormaps *cmaps);
void InstallRootColormap(void);
void UninstallRootColormap(void);

TwmColormap *CreateTwmColormap(Colormap c);
ColormapWindow *CreateColormapWindow(Window w, bool creating_parent,
                                     bool property_window);
void FetchWmColormapWindows(TwmWindow *tmp);

#endif /* _CTWM_COLORMAPS_H */
