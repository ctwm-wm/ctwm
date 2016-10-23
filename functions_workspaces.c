/*
 * Functions related to window occupation and workspaces.  Not the
 * workspace manager itself; that's off with the icon managers.
 */

#include "ctwm.h"

#include "functions_internal.h"
#include "screen.h"
#include "occupation.h"


DFHANDLER(vanish)
{
	WMgrRemoveFromCurrentWorkSpace(Scr->currentvs, tmp_win);
}

DFHANDLER(warphere)
{
	WMgrAddToCurrentWorkSpaceAndWarp(Scr->currentvs, action);
}

DFHANDLER(addtoworkspace)
{
	AddToWorkSpace(action, tmp_win);
}

DFHANDLER(removefromworkspace)
{
	RemoveFromWorkSpace(action, tmp_win);
}

DFHANDLER(toggleoccupation)
{
	ToggleOccupation(action, tmp_win);
}

DFHANDLER(movetonextworkspace)
{
	MoveToNextWorkSpace(Scr->currentvs, tmp_win);
}

DFHANDLER(movetoprevworkspace)
{
	MoveToPrevWorkSpace(Scr->currentvs, tmp_win);
}

DFHANDLER(movetonextworkspaceandfollow)
{
	MoveToNextWorkSpaceAndFollow(Scr->currentvs, tmp_win);
}

DFHANDLER(movetoprevworkspaceandfollow)
{
	MoveToPrevWorkSpaceAndFollow(Scr->currentvs, tmp_win);
}

