/*
 * Parser -- M4 specific routines.  Some additional stuff from parse.c
 * should probably migrate here over time.
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#include "screen.h"
#include "parse.h"
#include "parse_int.h"
#include "util.h"
#include "version.h"


static char *m4_defs(Display *display, char *host);


/*
 * Primary entry point to do m4 parsing of a startup file
 */
FILE *start_m4(FILE *fraw)
{
	int fno;
	int fids[2];
	int fres;               /* Fork result */

	fno = fileno(fraw);
	/* if (-1 == fcntl(fno, F_SETFD, 0)) perror("fcntl()"); */
	pipe(fids);
	fres = fork();
	if(fres < 0) {
		perror("Fork for " M4CMD " failed");
		exit(23);
	}
	if(fres == 0) {
		char *tmp_file;

		/* Child */
		close(0);               /* stdin */
		close(1);               /* stdout */
		dup2(fno, 0);           /* stdin = fraw */
		dup2(fids[1], 1);       /* stdout = pipe to parent */
		tmp_file = m4_defs(dpy, CLarg.display_name);
		execlp(M4CMD, M4CMD, "-s", tmp_file, "-", NULL);

		/* If we get here we are screwed... */
		perror("Can't execlp() " M4CMD);
		exit(124);
	}
	/* Parent */
	close(fids[1]);
	return ((FILE *)fdopen(fids[0], "r"));
}

/* Code taken and munged from xrdb.c */
#define Resolution(pixels, mm)  ((((pixels) * 100000 / (mm)) + 50) / 100)
#define EXTRA   16 /* Egad */

static const char *MkDef(const char *name, const char *def)
{
	static char *cp = NULL;
	static int maxsize = 0;
	int n, nl;

	if(def == NULL) {
		return ("");        /* XXX JWS: prevent segfaults */
	}
	/* The char * storage only lasts for 1 call... */
	if((n = EXTRA + ((nl = strlen(name)) +  strlen(def))) > maxsize) {
		maxsize = MAX(n, 4096);
		/* Safety net: this is wildly overspec */
		if(maxsize > 1024 * 1024 * 1024) {
			fprintf(stderr, "Cowardly refusing to malloc() a gig.\n");
			exit(1);
		}

		free(cp);
		cp = malloc(maxsize);
	}
	/* Otherwise cp is aready big 'nuf */
	if(cp == NULL) {
		fprintf(stderr, "Can't get %d bytes for arg parm\n", maxsize);
		exit(468);
	}

	snprintf(cp, maxsize, "define(`%s', `%s')\n", name, def);
	return(cp);
}

static const char *MkNum(const char *name, int def)
{
	char num[20];

	sprintf(num, "%d", def);
	return(MkDef(name, num));
}


/* Technically should sysconf() this, but good enough for our purposes */
#define MAXHOSTNAME 255

/*
 * Writes out a temp file of all the m4 defs appropriate for this run,
 * and returns the file name
 */
static char *m4_defs(Display *display, char *host)
{
	Screen *screen;
	Visual *visual;
	char client[MAXHOSTNAME], server[MAXHOSTNAME], *colon;
	struct hostent *hostname;
	char *vc;               /* Visual Class */
	static char tmp_name[] = "/tmp/twmrcXXXXXX";
	int fd;
	FILE *tmpf;
	char *user;

	/* Create temp file */
	fd = mkstemp(tmp_name);
	if(fd < 0) {
		perror("mkstemp failed in m4_defs");
		exit(377);
	}
	tmpf = fdopen(fd, "w+");


	/*
	 * Now start writing the defs into it.
	 */
#define WR_DEF(k, v) fputs(MkDef((k), (v)), tmpf)
#define WR_NUM(k, v) fputs(MkNum((k), (v)), tmpf)

	/*
	 * The machine running the window manager process (and, presumably,
	 * most of the other clients the user is running)
	 */
	if(gethostname(client, MAXHOSTNAME) < 0) {
		perror("gethostname failed in m4_defs");
		exit(1);
	}
	WR_DEF("CLIENTHOST", client);

	/*
	 * A guess at the machine running the X server.  We take the full
	 * $DISPLAY and chop off the screen specification.
	 */
	/* stpncpy() a better choice */
	snprintf(server, sizeof(server) - 1, "%s", XDisplayName(host));
	colon = strchr(server, ':');
	if(colon != NULL) {
		*colon = '\0';
	}
	/* :0 or unix socket connection means it's the same as CLIENTHOST */
	if((server[0] == '\0') || (!strcmp(server, "unix"))) {
		strcpy(server, client);
	}
	WR_DEF("SERVERHOST", server);

	/* DNS (/NSS) lookup can't be the best way to do this, but... */
	hostname = gethostbyname(client);
	if(hostname) {
		WR_DEF("HOSTNAME", hostname->h_name);
	}
	else {
		WR_DEF("HOSTNAME", client);
	}

	/*
	 * Info about the user and their environment
	 */
	if(!(user = getenv("USER")) && !(user = getenv("LOGNAME"))) {
		user = "unknown";
	}
	WR_DEF("USER", user);
	WR_DEF("HOME", getenv("HOME"));

	/*
	 * ctwm meta
	 */
	WR_DEF("TWM_TYPE", "ctwm");
	WR_DEF("TWM_VERSION", VersionNumber);

	/*
	 * X server meta
	 */
	WR_NUM("VERSION", ProtocolVersion(display));
	WR_NUM("REVISION", ProtocolRevision(display));
	WR_DEF("VENDOR", ServerVendor(display));
	WR_NUM("RELEASE", VendorRelease(display));

	/*
	 * Information about the display
	 */
	screen = ScreenOfDisplay(display, Scr->screen);
	visual = DefaultVisualOfScreen(screen);
	WR_NUM("WIDTH", screen->width);
	WR_NUM("HEIGHT", screen->height);
	WR_NUM("X_RESOLUTION", Resolution(screen->width, screen->mwidth));
	WR_NUM("Y_RESOLUTION", Resolution(screen->height, screen->mheight));
	WR_NUM("PLANES", DisplayPlanes(display, Scr->screen));
	WR_NUM("BITS_PER_RGB", visual->bits_per_rgb);
	switch(visual->class) {
		case(StaticGray):
			vc = "StaticGray";
			break;
		case(GrayScale):
			vc = "GrayScale";
			break;
		case(StaticColor):
			vc = "StaticColor";
			break;
		case(PseudoColor):
			vc = "PseudoColor";
			break;
		case(TrueColor):
			vc = "TrueColor";
			break;
		case(DirectColor):
			vc = "DirectColor";
			break;
		default:
			vc = "NonStandard";
			break;
	}
	WR_DEF("CLASS", vc);
	if(visual->class != StaticGray && visual->class != GrayScale) {
		WR_DEF("COLOR", "Yes");
	}
	else {
		WR_DEF("COLOR", "No");
	}

	/*
	 * Bits of "how this ctwm invocation is being run" data
	 */
	if(CLarg.is_captive && Scr->captivename) {
		WR_DEF("TWM_CAPTIVE", "Yes");
		WR_DEF("TWM_CAPTIVE_NAME", Scr->captivename);
	}
	else {
		WR_DEF("TWM_CAPTIVE", "No");
	}

	/*
	 * Various compile-time options.
	 */
#ifdef PIXMAP_DIRECTORY
	WR_DEF("PIXMAP_DIRECTORY", PIXMAP_DIRECTORY);
#endif
#ifdef XPM
	WR_DEF("XPM", "Yes");
#endif
#ifdef JPEG
	WR_DEF("JPEG", "Yes");
#endif
#ifdef SOUNDS
	WR_DEF("SOUNDS", "Yes");
#endif
#ifdef EWMH
	WR_DEF("EWMH", "Yes");
#endif
	/* Since this is no longer an option, it should be removed in the future */
	WR_DEF("I18N", "Yes");

#undef WR_NUM
#undef WR_DEF


	/*
	 * We might be keeping it, in which case tell the user where it is;
	 * this is mostly a debugging option.  Otherwise, delete it by
	 * telling m4 to do so when it reads it; this is fairly fugly, and I
	 * have more than half a mind to dike it out and properly clean up
	 * ourselves.
	 */
	if(CLarg.KeepTmpFile) {
		fprintf(stderr, "Left file: %s\n", tmp_name);
	}
	else {
		fprintf(tmpf, "syscmd(/bin/rm %s)\n", tmp_name);
	}


	/* Close out and hand it back */
	fclose(tmpf);
	return(tmp_name);
}
