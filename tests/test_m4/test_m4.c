/**
 * Test that m4 config rewriting works.
 */

#include "ctwm.h"

#include <stdio.h>

#include "ctwm_main.h"
#include "screen.h"


/**
 * Callback: after the config file gets parsed, make sure we got the info
 * we expected out of it.
 */
static int
check_wsm_geom(void)
{
	const char *expected_geom = "300x100+10+10";
	const int expected_columns = 2;

	// Guard against things that shouldn't happen
	if(Scr == NULL) {
		fprintf(stderr, "BUG: Scr == NULL\n");
		return 1;
	}
	if(Scr->workSpaceMgr.geometry == NULL) {
		fprintf(stderr, "BUG: Scr->workSpaceMgr.geometry == NULL\n");
		return 1;
	}


	// Force failure
	return 1;
}



/*
 * Connect up our callback and kick off ctwm.
 *
 * XXX We should probably have the various necessary args hardcoded into
 * here instead of relying on our caller, but this is a workable first
 * step.
 */
extern int (*ctwm_test_postparse)(void);

int
main(int argc, char *argv[])
{
	ctwm_test_postparse = check_wsm_geom;
	return ctwm_main(argc, argv);
}
