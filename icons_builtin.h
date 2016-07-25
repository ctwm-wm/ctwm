/*
 * Builtin icon builders
 */

#ifndef _CTWM_ICONS_BUILTIN_H
#define _CTWM_ICONS_BUILTIN_H

Pixmap CreateMenuIcon(int height, unsigned int *widthp, unsigned int *heightp);
Pixmap Create3DMenuIcon(unsigned int height,
                        unsigned int *widthp, unsigned int *heightp,
                        ColorPair cp);
Pixmap Create3DIconManagerIcon(ColorPair cp);


#endif // _CTWM_ICONS_BUILTIN_H
