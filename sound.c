/*
 *  [ ctwm ]
 *
 *  Copyright 1992 Claude Lecommandeur.
 *
 * Permission to use, copy, modify  and distribute this software  [ctwm] and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above  copyright notice appear  in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of  Claude Lecommandeur not be used in adverti-
 * sing or  publicity  pertaining to  distribution of  the software  without
 * specific, written prior permission. Claude Lecommandeur make no represen-
 * tations  about the suitability  of this software  for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Claude Lecommandeur DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL  Claude Lecommandeur  BE LIABLE FOR ANY SPECIAL,  INDIRECT OR
 * CONSEQUENTIAL  DAMAGES OR ANY  DAMAGES WHATSOEVER  RESULTING FROM LOSS OF
 * USE, DATA  OR PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR
 * OTHER  TORTIOUS ACTION,  ARISING OUT OF OR IN  CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Claude Lecommandeur [ lecom@sic.epfl.ch ][ April 1992 ]
 */
/*
 * These routines were extracted from the sound hack for olvwm3.3 by
 * Andrew "Ender" Scherpbier (turtle@sciences.sdsu.edu)
 * and modified by J.E. Sacco (jsacco @ssl.com)
 */

#include "ctwm.h"

#include <rplay.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "event_names.h"
#include "sound.h"

RPLAY **rp = NULL;

static int need_sound_init = 1;
static int sound_fd = 0;
static int sound_state = 1;
#define HOSTNAME_LEN 200
static char hostname[HOSTNAME_LEN];

/*
 * Function to trim away spaces at the start and end of a string
 */
static char *
trim_spaces(char *str)
{
	if(str != NULL) {
		char *p = str + strlen(str);
		while(*str != '\0' && *str != '\r' && *str != '\n' && isspace(*str)) {
			str++;
		}
		/* Assume all line end characters are at the end */
		while(p > str && isspace(p[-1])) {
			p--;
		}
		*p = '\0';
	}
	return str;
}

/*
 * Define stuff related to "magic" names.
 */
static const char *magic_events[] = {
	"Startup",
	"Shutdown",
};
#define NMAGICEVENTS (sizeof(magic_events) / sizeof(*magic_events))

static int
sound_magic_event_name2num(const char *name)
{
	int i;

	for(i = 0 ; i < NMAGICEVENTS ; i++) {
		if(strcasecmp(name, magic_events[i]) == 0) {
			/* We number these off the far end of the non-magic events */
			return event_names_size() + i;
		}
	}

	return -1;
}


/*
 * Now we know how many events we need to store up info for
 */
#define NEVENTS (event_names_size() + NMAGICEVENTS)



/*
 * initialize
 */
static void
sound_init(void)
{
	int i;
	FILE *fl;
	char buffer[100];
	char *home;
	char *soundfile;

	need_sound_init = 0;
	if(sound_fd == 0) {
		if(hostname[0] == '\0') {
			strncpy(hostname, rplay_default_host(), HOSTNAME_LEN - 1);
			hostname[HOSTNAME_LEN - 1] = '\0'; /* JIC */
		}

		if((sound_fd = rplay_open(hostname)) < 0) {
			rplay_perror("create");
		}
	}

	/*
	 * Init rp if necessary
	 */
	if(rp == NULL) {
		if((rp = calloc(NEVENTS, sizeof(RPLAY *))) == NULL) {
			perror("calloc() rplay control");
			exit(1);
			/*
			 * Should maybe just bomb out of sound stuff, but there's
			 * currently no provision for that.  If malloc fails, we're
			 * pretty screwed anyway, so it's not much loss to just die.
			 */
		}
	}

	/*
	 * Destroy any old sounds
	 */
	for(i = 0; i < NEVENTS; i++) {
		if(rp[i] != NULL) {
			rplay_destroy(rp[i]);
		}
		rp[i] = NULL;
	}

	/*
	 * Now read the file which contains the sounds
	 */
	if((home = getenv("HOME")) == NULL) {
		home = "";
	}
	if(asprintf(&soundfile, "%s/.ctwm-sounds", home) < 0) {
		perror("Failed building path to sound file");
		return;
	}
	fl = fopen(soundfile, "r");
	free(soundfile);
	if(fl == NULL) {
		return;
	}
	while(fgets(buffer, 100, fl) != NULL) {
		char *ename, *sndfile;

		ename = trim_spaces(strtok(buffer, ": \t"));
		if(ename == NULL || *ename == '#') {
			continue;
		}

		sndfile = trim_spaces(strtok(NULL, "\r\n"));
		if(sndfile == NULL || *sndfile == '#') {
			continue;
		}

		set_sound_event_name(ename, sndfile);
	}
	fclose(fl);
}


/*
 * Play sound
 */
void
play_sound(int snd)
{
	/* Bounds */
	if(snd < 0 || snd >= NEVENTS) {
		return;
	}

	/* Playing enabled */
	if(sound_state == 0) {
		return;
	}

	/* Init if we aren't */
	if(need_sound_init) {
		sound_init();
	}

	/* Skip if this isn't a sound we have set */
	if(rp[snd] == NULL) {
		return;
	}

	/* And if all else fails, play it */
	if(rplay(sound_fd, rp[snd]) < 0) {
		rplay_perror("rplay");
	}
}

void
play_startup_sound(void)
{
	play_sound(sound_magic_event_name2num("Startup"));
}

void
play_exit_sound(void)
{
	play_sound(sound_magic_event_name2num("Shutdown"));
}

/*
 * Toggle the sound on/off
 */
void
toggle_sound(void)
{
	sound_state ^= 1;
}


/*
 * Re-read the sounds mapping file
 */
void
reread_sounds(void)
{
	sound_init();
}

/*
 * Set the SoundHost and force the sound_fd to be re-opened.
 */
void
set_sound_host(char *host)
{
	strncpy(hostname, host, HOSTNAME_LEN - 1);
	hostname[HOSTNAME_LEN - 1] = '\0'; /* JIC */
	if(sound_fd != 0) {
		rplay_close(sound_fd);
	}
	sound_fd = 0;
}

/*
 * Set the sound to play for a given event
 */
void
set_sound_event_name(const char *ename, const char *soundfile)
{
	int i;

	/* Find the index we'll use in rp[] for it */
	i = sound_magic_event_name2num(ename);
	if(i < 0) {
		i = event_num_by_name(ename);
	}
	if(i < 0) {
		return;
	}

	/* Gotcha */
	set_sound_event(i, soundfile);
	return;
}

void
set_sound_event(int snd, const char *soundfile)
{
	/* This shouldn't get called before things are initialized */
	if(rp == NULL) {
		fprintf(stderr, "%s(): internal error: called before initialized.\n", __func__);
		exit(1);
	}

	/* Cleanup old if necessary */
	if(rp[snd] != NULL) {
		rplay_destroy(rp[snd]);
	}

	/* Setup new */
	rp[snd] = rplay_create(RPLAY_PLAY);
	if(rp[snd] == NULL) {
		rplay_perror("create");
		return;
	}
	if(rplay_set(rp[snd], RPLAY_INSERT, 0, RPLAY_SOUND, soundfile, NULL)
				    < 0) {
		rplay_perror("rplay");
	}

	return;
}
