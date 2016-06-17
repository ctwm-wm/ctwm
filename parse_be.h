/*
 * Parser backend header bits.  These are mostly things that wind up
 * called from the yacc routines
 */
#ifndef _CTWM_PARSE_BE_H
#define _CTWM_PARSE_BE_H

int parse_keyword(char *s, int *nump);

int do_single_keyword(int keyword);
int do_string_keyword(int keyword, char *s);
int do_string_string_keyword(int keyword, char *s1, char *s2);
int do_number_keyword(int keyword, int num);
name_list **do_colorlist_keyword(int keyword, int colormode, char *s);
int do_color_keyword(int keyword, int colormode, char *s);
int do_string_savecolor(int colormode, char *s);
int do_var_savecolor(int key);
int do_squeeze_entry(name_list **list,  /* squeeze or dont-squeeze list */
                     char *name,        /* window name */
                     SIJust justify,    /* left, center, right */
                     int num,           /* signed num */
                     int denom          /* 0 or indicates fraction denom */
                    );
void proc_ewmh_ignore(void);
void add_ewmh_ignore(char *s);
void proc_mwm_ignore(void);
void add_mwm_ignore(char *s);

#endif /* _CTWM_PARSE_BE_H */
