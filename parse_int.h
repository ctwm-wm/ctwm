/*
 * Parser internals that need to be shared across broken-up files
 */
#ifndef _PARSE_INT_H
#define _PARSE_INT_H

/* Stuff in parse_m4.c, if enabled */
#ifdef USEM4
FILE *start_m4(FILE *fraw);
#endif

#endif /* _PARSE_INT_H */
