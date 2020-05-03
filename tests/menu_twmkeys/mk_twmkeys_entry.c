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
			fprintf(stderr, "Expected '%s', got '%s'\n", expect, ret); \
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


	// OK then
	exit(0);
}
