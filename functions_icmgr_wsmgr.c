/*
 * Functions related to icon managers and the workspace manager.
 */

#include "ctwm.h"

#include "functions_internal.h"
#include "iconmgr.h"
#include "otp.h"
#include "win_iconify.h"
#include "workspace_manager.h"
#include "workspace_utils.h"


/*
 * Moving around in the icon manager.
 *
 * XXX These backend funcs in iconmgr.c are passed func directly.  That's
 * a bit of a layering violation; they should maybe be changed to have
 * their own idea of directionality...
 */
DFHANDLER(upiconmgr)    { MoveIconManager(func); }
DFHANDLER(downiconmgr)  { MoveIconManager(func); }
DFHANDLER(lefticonmgr)  { MoveIconManager(func); }
DFHANDLER(righticonmgr) { MoveIconManager(func); }
DFHANDLER(forwiconmgr)  { MoveIconManager(func); }
DFHANDLER(backiconmgr)  { MoveIconManager(func); }

/* XXX These two functions really should be merged... */
DFHANDLER(forwmapiconmgr) { MoveMappedIconManager(func); }
DFHANDLER(backmapiconmgr) { MoveMappedIconManager(func); }

/* Moving between icon managers */
DFHANDLER(nexticonmgr) { JumpIconManager(func); }
DFHANDLER(previconmgr) { JumpIconManager(func); }
