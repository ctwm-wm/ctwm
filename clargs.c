/*
 * Command-line arg handling
 */

#include "ctwm.h"

#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include "clargs.h"
#include "screen.h"
#include "version.h"

static void usage(void) __attribute__((noreturn));
static void print_version(void);
static void DisplayInfo(void);


/*
 * Command-line args.  Initialize with useful default values.
 */
ctwm_cl_args CLarg = {
	.MultiScreen     = TRUE,
	.Monochrome      = FALSE,
	.cfgchk          = 0,
	.InitFile        = NULL,
	.display_name    = NULL,
	.PrintErrorMessages = False,
#ifdef DEBUG
	.ShowWelcomeWindow  = False,
#else
	.ShowWelcomeWindow  = True,
#endif
	.is_captive      = FALSE,
	.capwin          = (Window) 0,
	.captivename     = NULL,
#ifdef USEM4
	.KeepTmpFile     = False,
	.keepM4_filename = NULL,
	.GoThroughM4     = True,
#endif
#ifdef EWMH
	.ewmh_replace    = 0,
#endif
	.client_id       = NULL,
	.restore_filename = NULL,
};



/*
 * Parse them out and setup CLargs.
 */
void
clargs_parse(int argc, char *argv[])
{
	int ch, optidx;

	/*
	 * Setup long options for arg parsing
	 */
	static struct option long_options[] = {
		/* Simple flags */
		{ "single",    no_argument,       &CLarg.MultiScreen, FALSE },
		{ "mono",      no_argument,       &CLarg.Monochrome, TRUE },
		{ "verbose",   no_argument,       NULL, 'v' },
		{ "quiet",     no_argument,       NULL, 'q' },
		{ "nowelcome", no_argument,       NULL, 'W' },

		/* Config/file related */
		{ "file",      required_argument, NULL, 'f' },
		{ "cfgchk",    no_argument,       &CLarg.cfgchk, 1 },

		/* Show something and exit right away */
		{ "help",      no_argument,       NULL, 'h' },
		{ "version",   no_argument,       NULL, 0 },
		{ "info",      no_argument,       NULL, 0 },

		/* Misc control bits */
		{ "display",   required_argument, NULL, 'd' },
		{ "window",    optional_argument, NULL, 'w' },
		{ "name",      required_argument, NULL, 0 },
		{ "xrm",       required_argument, NULL, 0 },

#ifdef EWMH
		{ "replace",   no_argument,       &CLarg.ewmh_replace, 1 },
#endif

		/* M4 control params */
#ifdef USEM4
		{ "keep-defs", no_argument,       NULL, 'k' },
		{ "keep",      required_argument, NULL, 'K' },
		{ "nom4",      no_argument,       NULL, 'n' },
#endif

		/* Random session-related bits */
		{ "clientId",  required_argument, NULL, 0 },
		{ "restore",   required_argument, NULL, 0 },
	};


	/*
	 * Short aliases for some
	 *
	 * I assume '::' for optional args is portable; getopt_long(3)
	 * doesn't describe it, but it's a GNU extension for getopt(3).
	 */
	const char *short_options = "vqWf:hd:w::"
#ifdef USEM4
	                            "kK:n"
#endif
	                            ;


	/*
	 * Backward-compat cheat: accept a few old-style long args if they
	 * came first.  Of course, this assumed argv[x] is editable, which on
	 * most systems it is, and C99 requires it.
	 */
	if(argc > 1) {
#define CHK(x) else if(strcmp(argv[1], (x)) == 0)
		if(0) {
			/* nada */
		}
		CHK("-version") {
			print_version();
			exit(0);
		}
		CHK("-info") {
			DisplayInfo();
			exit(0);
		}
		CHK("-cfgchk") {
			CLarg.cfgchk = 1;
			*argv[1] = '\0';
		}
		CHK("-display") {
			if(argc <= 2 || strlen(argv[2]) < 1) {
				usage();
			}
			CLarg.display_name = strdup(argv[2]);

			*argv[1] = '\0';
			*argv[2] = '\0';
		}
#undef CHK
	}


	/*
	 * Parse out the args
	 */
	optidx = 0;
	while((ch = getopt_long(argc, argv, short_options, long_options,
	                        &optidx)) != -1) {
		switch(ch) {
			/* First handle the simple cases that have short args */
			case 'v':
				CLarg.PrintErrorMessages = True;
				break;
			case 'q':
				CLarg.PrintErrorMessages = False;
				break;
			case 'W':
				CLarg.ShowWelcomeWindow = False;
				break;
			case 'f':
				CLarg.InitFile = optarg;
				break;
			case 'h':
				usage();
			case 'd':
				CLarg.display_name = optarg;
				break;
			case 'w':
				CLarg.is_captive = True;
				CLarg.MultiScreen = False;
				if(optarg != NULL) {
					sscanf(optarg, "%x", (unsigned int *)&CLarg.capwin);
					/* Failure will just leave capwin as initialized */
				}
				break;

#ifdef USEM4
			/* Args that only mean anything if we're built with m4 */
			case 'k':
				CLarg.KeepTmpFile = True;
				break;
			case 'K':
				CLarg.keepM4_filename = optarg;
				break;
			case 'n':
				CLarg.GoThroughM4 = False;
				break;
#endif


			/*
			 * Now the stuff that doesn't have short variants.  Many of
			 * them just set flags, so we don't need to do anything with
			 * them.  Only the ones with NULL flags matter.
			 */
			case 0:
				if(long_options[optidx].flag != NULL) {
					/* It only existed to set a flag; we're done */
					break;
				}

#define IFIS(x) if(strcmp(long_options[optidx].name, (x)) == 0)
				IFIS("version") {
					print_version();
					exit(0);
				}
				IFIS("info") {
					DisplayInfo();
					exit(0);
				}
				IFIS("name") {
					CLarg.captivename = optarg;
					break;
				}
				IFIS("xrm") {
					/*
					 * Quietly ignored by us; Xlib processes it
					 * internally in XtToolkitInitialize();
					 */
					break;
				}
				IFIS("clientId") {
					CLarg.client_id = optarg;
					break;
				}
				IFIS("restore") {
					CLarg.restore_filename = optarg;
					break;
				}
#undef IFIS

				/* Don't think it should be possible to get here... */
				fprintf(stderr, "Internal error in getopt: '%s' unhandled.\n",
				        long_options[optidx].name);
				usage();

			/* Something totally unexpected */
			case '?':
				/* getopt_long() already printed an error */
				usage();

			default:
				/* Uhhh...  */
				fprintf(stderr, "Internal error: getopt confused us.\n");
				usage();
		}
	}


	/* Should do it */
	return;
}


/*
 * Sanity check CLarg's
 */
void
clargs_check(void)
{

#ifdef USEM4
	/* If we're not doing m4, don't specify m4 options */
	if(!CLarg.GoThroughM4) {
		if(CLarg.KeepTmpFile) {
			fprintf(stderr, "--keep-defs is incompatible with --nom4.\n");
			usage();
		}
		if(CLarg.keepM4_filename) {
			fprintf(stderr, "--keep is incompatible with --nom4.\n");
			usage();
		}
	}
#endif

	/* If we're not captive, captivename is meaningless too */
	if(CLarg.captivename && !CLarg.is_captive) {
		fprintf(stderr, "--name is meaningless without --window.\n");
		usage();
	}

	/* Guess that's it */
	return;
}


/*
 * Small utils only currently used in this file.  Over time they may need
 * to be exported, if we start using them from more places.
 */
static void
usage(void)
{
	/* How far to indent continuation lines */
	int llen = 10;

	fprintf(stderr, "usage: %s [(--display | -d) dpy]  "
#ifdef EWMH
	        "[--replace]  "
#endif
	        "[--single]\n", ProgramName);

	fprintf(stderr, "%*s[(--file | -f) initfile]  [--cfgchk]\n", llen, "");

#ifdef USEM4
	fprintf(stderr, "%*s[--nom4 | -n]  [--keep-defs | -k]  "
	        "[(--keep | -K) m4file]\n", llen, "");
#endif

	fprintf(stderr, "%*s[--verbose | -v]  [--quiet | -q]  [--mono]  "
	        "[--xrm resource]\n", llen, "");

	fprintf(stderr, "%*s[--version]  [--info]  [--nowelcome | -W]\n",
	        llen, "");

	fprintf(stderr, "%*s[(--window | -w) [win-id]]  [--name name]\n", llen, "");

	/* Semi-intentionally not documenting --clientId/--restore */

	fprintf(stderr, "%*s[--help]\n", llen, "");


	exit(1);
}


static void
print_version(void)
{
	printf("ctwm %s\n", VersionNumber);
	if(VCSRevision) {
		printf(" (bzr:%s)\n", VCSRevision);
	}
}


static void
DisplayInfo(void)
{
	printf("Twm version:  %s\n", Version);
	printf("Compile time options :");
#ifdef XPM
	printf(" XPM");
#endif
#ifdef USEM4
	printf(" USEM4");
#endif
#ifdef SOUNDS
	printf(" SOUNDS");
#endif
#ifdef GNOME
	printf(" GNOME");
#endif
#ifdef EWMH
	printf(" EWMH");
#endif
	printf(" I18N");
	printf("\n");
}
