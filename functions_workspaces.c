/*
 * Functions related to window occupation and workspaces.  Not the
 * workspace manager itself; that's off with the icon managers.
 */

#include "ctwm.h"

#include "functions_internal.h"
#include "screen.h"
#include "occupation.h"
#include "workspace_utils.h"


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


DFHANDLER(occupy)
{
	Occupy(tmp_win);
}

DFHANDLER(occupyall)
{
	OccupyAll(tmp_win);
}

DFHANDLER(gotoworkspace)
{
	GotoWorkSpaceByName(Scr->currentvs, action);
}

DFHANDLER(prevworkspace)
{
	GotoPrevWorkSpace(Scr->currentvs);
}

DFHANDLER(nextworkspace)
{
	GotoNextWorkSpace(Scr->currentvs);
}

DFHANDLER(rightworkspace)
{
	GotoRightWorkSpace(Scr->currentvs);
}

DFHANDLER(leftworkspace)
{
	GotoLeftWorkSpace(Scr->currentvs);
}

DFHANDLER(upworkspace)
{
	GotoUpWorkSpace(Scr->currentvs);
}

DFHANDLER(downworkspace)
{
	GotoDownWorkSpace(Scr->currentvs);
}
