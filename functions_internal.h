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
DFHANDLER(showiconmgr);
DFHANDLER(hideiconmgr);
DFHANDLER(sorticonmgr);

DFHANDLER(showworkspacemgr);
DFHANDLER(hideworkspacemgr);
DFHANDLER(toggleworkspacemgr);
DFHANDLER(togglestate);
DFHANDLER(setbuttonsstate);
DFHANDLER(setmapstate);


/* functions_win_moveresize.c */
DFHANDLER(initsize);
DFHANDLER(moveresize);
DFHANDLER(changesize);
DFHANDLER(savegeometry);
DFHANDLER(restoregeometry);
DFHANDLER(move);
DFHANDLER(forcemove);
DFHANDLER(movepack);
DFHANDLER(movepush);
DFHANDLER(zoom);
DFHANDLER(horizoom);
DFHANDLER(fullzoom);
DFHANDLER(fullscreenzoom);
DFHANDLER(leftzoom);
DFHANDLER(rightzoom);
DFHANDLER(topzoom);
DFHANDLER(bottomzoom);
DFHANDLER(resize);
DFHANDLER(pack);
DFHANDLER(fill);



/*
 * Extra exported from functions_icmgr_wsmgr.c for use in
 * f.delete{,ordestroy}.
 */
void HideIconManager(void);



#endif /* _CTWM_FUNCTIONS_INTERNAL_H */
