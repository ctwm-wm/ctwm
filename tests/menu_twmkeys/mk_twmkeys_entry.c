/*
 * Test mk_twmkeys_entry()
 */

#include "ctwm.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "menus.h"

int
main(int argc, char *argv[])
{
	FuncKey key;
	int mods;

#define SET() do { \
                memset(&key, 0, sizeof(key)); \
                key.name   = "KEY"; \
                key.action = "ACT"; \
                key.mods   = mods; \
        } while(0) \

#define TST(expect) do { \
                SET(); \
                const char *ret = mk_twmkeys_entry(&key); \
                if(strcmp(ret, expect) != 0) { \
                        fprintf(stderr, "Expected '%s', got '%s'\n", \
                                        expect, ret); \
                        exit(1); \
                } \
        } while(0)

	// Simple
	mods = ShiftMask;
	TST("[S+KEY] ACT");

	mods = ControlMask;
	TST("[C+KEY] ACT");

	// Combo
	mods = Mod1Mask | Alt1Mask;
	TST("[M+A1+KEY] ACT");

	mods = Alt1Mask | Alt2Mask | Alt3Mask | Alt4Mask | Alt5Mask;
	TST("[A1+A2+A3+A4+A5+KEY] ACT");

	// All the mods!
	mods = ShiftMask | ControlMask
	       | Mod1Mask | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask
	       | Alt1Mask | Alt2Mask | Alt3Mask | Alt4Mask | Alt5Mask;
	TST("[M+S+C+M2+M3+M4+M5+A1+A2+A3+A4+A5+KEY] ACT");


	// Magic value used to test overflow
	{
		// Overflow by 1: inherit mods from above, and add a bit
		mods |= 1 << 30;
		SET();

		const char *ret = mk_twmkeys_entry(&key);
		if(ret != NULL) {
			fprintf(stderr, "Should have blown up for Over1, instead "
			        "got '%s'.\n", ret);
			exit(1);
		}
	}
	{
		// Overflow by itself
		mods = 1 << 31;
		SET();

		const char *ret = mk_twmkeys_entry(&key);
		if(ret != NULL) {
			fprintf(stderr, "Should have blown up for OverAll, instead "
			        "got '%s'.\n", ret);
			exit(1);
		}
	}


	// OK then
	exit(0);
}
