/*
 * Parser -- M4 specific routines.  Some additional stuff from parse.c
 * should probably migrate here over time.
 */

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>

#include <X11/Xmu/SysUtil.h>

#include "ctwm.h"
#include "screen.h"
#include "parse.h"
#include "parse_int.h"
#include "util.h"
#include "version.h"


/* Crazy historical */
#if defined(ultrix)
#define NOSTEMP
#endif


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
		tmp_file = m4_defs(dpy, display_name);
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
#define MAXHOSTNAME 255
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

#ifdef NOSTEMP
int mkstemp(str)
char *str;
{
	int fd;

	mktemp(str);
	fd = creat(str, 0744);
	if(fd == -1) {
		perror("mkstemp's creat");
	}
	return(fd);
}
#endif

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

	fd = mkstemp(tmp_name);         /* I *hope* mkstemp exists, because */
	/* I tried to find the "portable" */
	/* mktmp... */
	if(fd < 0) {
		perror("mkstemp failed in m4_defs");
		exit(377);
	}
	tmpf = (FILE *) fdopen(fd, "w+");
	XmuGetHostname(client, MAXHOSTNAME);
	hostname = gethostbyname(client);
	strcpy(server, XDisplayName(host));
	colon = strchr(server, ':');
	if(colon != NULL) {
		*colon = '\0';
	}
	if((server[0] == '\0') || (!strcmp(server, "unix"))) {
		strcpy(server, client);        /* must be connected to :0 or unix:0 */
	}
	/* The machine running the X server */
	fputs(MkDef("SERVERHOST", server), tmpf);
	/* The machine running the window manager process */
	fputs(MkDef("CLIENTHOST", client), tmpf);
	if(hostname) {
		fputs(MkDef("HOSTNAME", hostname->h_name), tmpf);
	}
	else {
		fputs(MkDef("HOSTNAME", client), tmpf);
	}

	if(!(user = getenv("USER")) && !(user = getenv("LOGNAME"))) {
		user = "unknown";
	}
	fputs(MkDef("USER", user), tmpf);
	fputs(MkDef("HOME", getenv("HOME")), tmpf);
#ifdef PIXMAP_DIRECTORY
	fputs(MkDef("PIXMAP_DIRECTORY", PIXMAP_DIRECTORY), tmpf);
#endif
	fputs(MkNum("VERSION", ProtocolVersion(display)), tmpf);
	fputs(MkNum("REVISION", ProtocolRevision(display)), tmpf);
	fputs(MkDef("VENDOR", ServerVendor(display)), tmpf);
	fputs(MkNum("RELEASE", VendorRelease(display)), tmpf);
	screen = ScreenOfDisplay(display, Scr->screen);
	visual = DefaultVisualOfScreen(screen);
	fputs(MkNum("WIDTH", screen->width), tmpf);
	fputs(MkNum("HEIGHT", screen->height), tmpf);
	fputs(MkNum("X_RESOLUTION", Resolution(screen->width, screen->mwidth)), tmpf);
	fputs(MkNum("Y_RESOLUTION", Resolution(screen->height, screen->mheight)), tmpf);
	fputs(MkNum("PLANES", DisplayPlanes(display, Scr->screen)), tmpf);
	fputs(MkNum("BITS_PER_RGB", visual->bits_per_rgb), tmpf);
	fputs(MkDef("TWM_TYPE", "ctwm"), tmpf);
	fputs(MkDef("TWM_VERSION", VersionNumber), tmpf);
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
	fputs(MkDef("CLASS", vc), tmpf);
	if(visual->class != StaticGray && visual->class != GrayScale) {
		fputs(MkDef("COLOR", "Yes"), tmpf);
	}
	else {
		fputs(MkDef("COLOR", "No"), tmpf);
	}
#ifdef XPM
	fputs(MkDef("XPM", "Yes"), tmpf);
#endif
#ifdef JPEG
	fputs(MkDef("JPEG", "Yes"), tmpf);
#endif
#ifdef GNOME
	fputs(MkDef("GNOME", "Yes"), tmpf);
#endif
#ifdef SOUNDS
	fputs(MkDef("SOUNDS", "Yes"), tmpf);
#endif
	fputs(MkDef("I18N", "Yes"), tmpf);
	if(captive && captivename) {
		fputs(MkDef("TWM_CAPTIVE", "Yes"), tmpf);
		fputs(MkDef("TWM_CAPTIVE_NAME", captivename), tmpf);
	}
	else {
		fputs(MkDef("TWM_CAPTIVE", "No"), tmpf);
	}
	if(CLarg.KeepTmpFile) {
		fprintf(stderr, "Left file: %s\n", tmp_name);
	}
	else {
		fprintf(tmpf, "syscmd(/bin/rm %s)\n", tmp_name);
	}
	fclose(tmpf);
	return(tmp_name);
}
