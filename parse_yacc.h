/*
 * Header for the various parse_yacc/gram.y backend funcs
 */
#ifndef _PARSE_YACC_H
#define _PARSE_YACC_H

void yyerror(char *s);

void InitGramVariables(void);
void RemoveDQuote(char *str);

MenuRoot *GetRoot(char *name, char *fore, char *back);

Bool CheckWarpScreenArg(char *s);
Bool CheckWarpRingArg(char *s);
Bool CheckColormapArg(char *s);
void GotButton(int butt, int func);
void GotKey(char *key, int func);
void GotTitleButton(char *bitmapname, int func, Bool rightside);


extern char *Action;
extern char *Name;
extern MenuRoot *root, *pull;
extern char *curWorkSpc;
extern char *client, *workspace;
extern MenuItem *lastmenuitem;

extern char *ptr;
extern name_list **curplist, **list;
extern int cont;
extern int color;
extern int mods;
extern unsigned int mods_used;


#define DEFSTRING "default"


#endif /* _PARSE_YACC_H */
