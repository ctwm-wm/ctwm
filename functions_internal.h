/*
 * Internal bits for function handling
 */

#ifndef _CTWM_FUNCTIONS_INTERNAL_H
#define _CTWM_FUNCTIONS_INTERNAL_H


/* Keep in sync with ExecuteFunction() in external functions.h */
#define EF_FULLPROTO \
      int   func,   void *  action,   Window   w,   TwmWindow *  tmp_win, \
      XEvent *  eventp,   int   context,   bool   pulldown
#define EF_ARGS \
    /*int */func, /*void **/action, /*Window */w, /*TwmWindow **/tmp_win, \
    /*XEvent **/eventp, /*int */context, /*bool */pulldown

typedef void (ExFunc)(EF_FULLPROTO);

#define DFHANDLER(func) void f_##func##_impl(EF_FULLPROTO)


/*
 * Various handlers
 */

/* functions_icmgr_wsmgr.c */
DFHANDLER(upiconmgr);
DFHANDLER(downiconmgr);
DFHANDLER(lefticonmgr);
DFHANDLER(righticonmgr);
DFHANDLER(forwiconmgr);
DFHANDLER(backiconmgr);

DFHANDLER(forwmapiconmgr);
DFHANDLER(backmapiconmgr);

DFHANDLER(nexticonmgr);
DFHANDLER(previconmgr);



#endif /* _CTWM_FUNCTIONS_INTERNAL_H */
