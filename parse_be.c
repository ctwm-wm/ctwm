/*
 * Parser backend routines.
 *
 * Roughly, these are the routines that the lex/yacc output calls to do
 * its work.
 *
 * This is very similar to the meaning of parse_yacc.c; the two may be
 * merged at some point.
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <X11/Xatom.h>

#include "ctwm_atoms.h"
#include "screen.h"
#include "util.h"
#include "animate.h"
#include "image.h"
#include "parse.h"
#include "parse_be.h"
#include "parse_yacc.h"
#ifdef SOUNDS
#  include "sound.h"
#endif

#include "gram.tab.h"


static int ParseRandomPlacement(char *s);
static int ParseButtonStyle(char *s);
static int ParseUsePPosition(char *s);



/**********************************************************************
 *
 *  Parsing table and routines
 *
 ***********************************************************************/

typedef struct _TwmKeyword {
	const char *name;
	int value;
	int subnum;
} TwmKeyword;

#define kw0_NoDefaults                  1
#define kw0_AutoRelativeResize          2
#define kw0_ForceIcons                  3
#define kw0_NoIconManagers              4
#define kw0_InterpolateMenuColors       6
//#define kw0_NoVersion                 7
#define kw0_SortIconManager             8
#define kw0_NoGrabServer                9
#define kw0_NoMenuShadows               10
#define kw0_NoRaiseOnMove               11
#define kw0_NoRaiseOnResize             12
#define kw0_NoRaiseOnDeiconify          13
#define kw0_DontMoveOff                 14
#define kw0_NoBackingStore              15
#define kw0_NoSaveUnders                16
#define kw0_RestartPreviousState        17
#define kw0_ClientBorderWidth           18
#define kw0_NoTitleFocus                19
#define kw0_DecorateTransients          21
#define kw0_ShowIconManager             22
#define kw0_NoCaseSensitive             23
#define kw0_NoRaiseOnWarp               24
#define kw0_WarpUnmapped                25
#define kw0_ShowWorkspaceManager        27
#define kw0_StartInMapState             28
#define kw0_NoShowOccupyAll             29
#define kw0_AutoOccupy                  30
#define kw0_TransientHasOccupation      31
#define kw0_DontPaintRootWindow         32
#define kw0_Use3DMenus                  33
#define kw0_Use3DTitles                 34
#define kw0_Use3DIconManagers           35
#define kw0_Use3DBorders                36
#define kw0_SunkFocusWindowTitle        37
#define kw0_BeNiceToColormap            38
#define kw0_WarpRingOnScreen            40
#define kw0_NoIconManagerFocus          41
#define kw0_StayUpMenus                 42
#define kw0_ClickToFocus                43
#define kw0_BorderResizeCursors         44
#define kw0_ReallyMoveInWorkspaceManager 45
#define kw0_ShowWinWhenMovingInWmgr     46
#define kw0_Use3DWMap                   47
#define kw0_ReverseCurrentWorkspace     48
#define kw0_DontWarpCursorInWMap        49
#define kw0_CenterFeedbackWindow        50
#define kw0_WarpToDefaultMenuEntry      51
#define kw0_ShrinkIconTitles            52
#define kw0_AutoRaiseIcons              53
#define kw0_use3DIconBorders            54
#define kw0_UseSunkTitlePixmap          55
#define kw0_ShortAllWindowsMenus        56
#define kw0_RaiseWhenAutoUnSqueeze      57
#define kw0_RaiseOnClick                58
#define kw0_IgnoreLockModifier          59
#define kw0_AutoFocusToTransients       60 /* kai */
#define kw0_PackNewWindows              61
#define kw0_IgnoreCaseInMenuSelection   62
#define kw0_SloppyFocus                 63
#define kw0_NoImagesInWorkSpaceManager  64
#define kw0_NoWarpToMenuTitle           65
#define kw0_SaveWorkspaceFocus          66 /* blais */
#define kw0_RaiseOnWarp                 67
#define kw0_DontShowWelcomeWindow       68
#define kw0_AutoPriority                69
#define kw0_DontToggleWorkspacemanagerState 70

#define kws_UsePPosition                1
#define kws_IconFont                    2
#define kws_ResizeFont                  3
#define kws_MenuFont                    4
#define kws_TitleFont                   5
#define kws_IconManagerFont             6
#define kws_UnknownIcon                 7
#define kws_IconDirectory               8
#define kws_MaxWindowSize               9
#define kws_PixmapDirectory             10
/* RandomPlacement moved because it's now a string string keyword */
#define kws_IconJustification           12
#define kws_TitleJustification          13
#define kws_IconRegionJustification     14
#define kws_IconRegionAlignement        15
#define kws_SoundHost                   16
#define kws_WMgrButtonStyle             17
#define kws_WorkSpaceFont               18
#define kws_IconifyStyle                19
#define kws_IconSize                    20
#define kws_RplaySoundHost              21

#define kwss_RandomPlacement            1

#define kwn_ConstrainedMoveTime         1
#define kwn_MoveDelta                   2
#define kwn_XorValue                    3
#define kwn_FramePadding                4
#define kwn_TitlePadding                5
#define kwn_ButtonIndent                6
#define kwn_BorderWidth                 7
#define kwn_IconBorderWidth             8
#define kwn_TitleButtonBorderWidth      9
#define kwn_RaiseDelay                  10
#define kwn_TransientOnTop              11
#define kwn_OpaqueMoveThreshold         12
#define kwn_OpaqueResizeThreshold       13
#define kwn_WMgrVertButtonIndent        14
#define kwn_WMgrHorizButtonIndent       15
#define kwn_ClearShadowContrast         16
#define kwn_DarkShadowContrast          17
#define kwn_WMgrButtonShadowDepth       18
#define kwn_MaxIconTitleWidth           19
#define kwn_AnimationSpeed              20
#define kwn_ThreeDBorderWidth           21
#define kwn_MoveOffResistance           22
#define kwn_BorderShadowDepth           23
#define kwn_TitleShadowDepth            24
#define kwn_TitleButtonShadowDepth      25
#define kwn_MenuShadowDepth             26
#define kwn_IconManagerShadowDepth      27
#define kwn_MovePackResistance          28
#define kwn_XMoveGrid                   29
#define kwn_YMoveGrid                   30
#define kwn_OpenWindowTimeout           31
#define kwn_RaiseOnClickButton          32

#define kwn_BorderTop                   33
#define kwn_BorderBottom                34
#define kwn_BorderLeft                  35
#define kwn_BorderRight                 36

#define kwcl_BorderColor                1
#define kwcl_IconManagerHighlight       2
#define kwcl_BorderTileForeground       3
#define kwcl_BorderTileBackground       4
#define kwcl_TitleForeground            5
#define kwcl_TitleBackground            6
#define kwcl_IconForeground             7
#define kwcl_IconBackground             8
#define kwcl_IconBorderColor            9
#define kwcl_IconManagerForeground      10
#define kwcl_IconManagerBackground      11
#define kwcl_MapWindowBackground        12
#define kwcl_MapWindowForeground        13

#define kwc_DefaultForeground           1
#define kwc_DefaultBackground           2
#define kwc_MenuForeground              3
#define kwc_MenuBackground              4
#define kwc_MenuTitleForeground         5
#define kwc_MenuTitleBackground         6
#define kwc_MenuShadowColor             7


/*
 * The following is sorted alphabetically according to name (which must be
 * in lowercase and only contain the letters a-z).  It is fed to a binary
 * search to parse keywords.
 */
static TwmKeyword keytable[] = {
	{ "a",                      ALTER, 0 },
	{ "all",                    ALL, 0 },
	{ "alter",                  ALTER, 0 },
	{ "alwaysontop",            ALWAYS_ON_TOP, 0 },
	{ "alwaysshowwindowwhenmovingfromworkspacemanager", KEYWORD, kw0_ShowWinWhenMovingInWmgr },
	{ "alwayssqueezetogravity", ALWAYSSQUEEZETOGRAVITY, 0 },
	{ "animationspeed",         NKEYWORD, kwn_AnimationSpeed },
	{ "autofocustotransients",  KEYWORD, kw0_AutoFocusToTransients }, /* kai */
	{ "autolower",              AUTO_LOWER, 0 },
	{ "autooccupy",             KEYWORD, kw0_AutoOccupy },
	{ "autopopup",              AUTO_POPUP, 0 },
	{ "autopriority",           KEYWORD, kw0_AutoPriority },
	{ "autoraise",              AUTO_RAISE, 0 },
	{ "autoraiseicons",         KEYWORD, kw0_AutoRaiseIcons },
	{ "autorelativeresize",     KEYWORD, kw0_AutoRelativeResize },
	{ "autosqueeze",            AUTOSQUEEZE, 0 },
	{ "benicetocolormap",       KEYWORD, kw0_BeNiceToColormap },
	{ "borderbottom",           NKEYWORD, kwn_BorderBottom },
	{ "bordercolor",            CLKEYWORD, kwcl_BorderColor },
	{ "borderleft",             NKEYWORD, kwn_BorderLeft },
	{ "borderresizecursors",    KEYWORD, kw0_BorderResizeCursors },
	{ "borderright",            NKEYWORD, kwn_BorderRight },
	{ "bordershadowdepth",      NKEYWORD, kwn_BorderShadowDepth },
	{ "bordertilebackground",   CLKEYWORD, kwcl_BorderTileBackground },
	{ "bordertileforeground",   CLKEYWORD, kwcl_BorderTileForeground },
	{ "bordertop",              NKEYWORD, kwn_BorderTop },
	{ "borderwidth",            NKEYWORD, kwn_BorderWidth },
	{ "button",                 BUTTON, 0 },
	{ "buttonindent",           NKEYWORD, kwn_ButtonIndent },
	{ "c",                      CONTROL, 0 },
	{ "center",                 SIJENUM, SIJ_CENTER },
	{ "centerfeedbackwindow",   KEYWORD, kw0_CenterFeedbackWindow },
	{ "changeworkspacefunction", CHANGE_WORKSPACE_FUNCTION, 0 },
	{ "clearshadowcontrast",    NKEYWORD, kwn_ClearShadowContrast },
	{ "clicktofocus",           KEYWORD, kw0_ClickToFocus },
	{ "clientborderwidth",      KEYWORD, kw0_ClientBorderWidth },
	{ "color",                  COLOR, 0 },
	{ "constrainedmovetime",    NKEYWORD, kwn_ConstrainedMoveTime },
	{ "control",                CONTROL, 0 },
	{ "cursors",                CURSORS, 0 },
	{ "darkshadowcontrast",     NKEYWORD, kwn_DarkShadowContrast },
	{ "decoratetransients",     KEYWORD, kw0_DecorateTransients },
	{ "defaultbackground",      CKEYWORD, kwc_DefaultBackground },
	{ "defaultforeground",      CKEYWORD, kwc_DefaultForeground },
	{ "defaultfunction",        DEFAULT_FUNCTION, 0 },
	{ "deiconifyfunction",      DEICONIFY_FUNCTION, 0 },
	{ "destroy",                KILL, 0 },
	{ "donticonifybyunmapping", DONT_ICONIFY_BY_UNMAPPING, 0 },
	{ "dontmoveoff",            KEYWORD, kw0_DontMoveOff },
	{ "dontpaintrootwindow",    KEYWORD, kw0_DontPaintRootWindow },
	{ "dontsave",               DONT_SAVE, 0 },
	{ "dontsetinactive",        DONTSETINACTIVE, 0 },
	{ "dontshowwelcomewindow",  KEYWORD, kw0_DontShowWelcomeWindow },
	{ "dontsqueezetitle",       DONT_SQUEEZE_TITLE, 0 },
	{ "donttoggleworkspacemanagerstate", KEYWORD, kw0_DontToggleWorkspacemanagerState},
	{ "dontwarpcursorinwmap",   KEYWORD, kw0_DontWarpCursorInWMap },
	{ "east",                   DKEYWORD, D_EAST },
	{ "ewmhignore",             EWMH_IGNORE, 0 },
	{ "f",                      FRAME, 0 },
	{ "f.addtoworkspace",       FSKEYWORD, F_ADDTOWORKSPACE },
	{ "f.adoptwindow",          FKEYWORD, F_ADOPTWINDOW },
	{ "f.altcontext",           FKEYWORD, F_ALTCONTEXT },
	{ "f.altkeymap",            FSKEYWORD, F_ALTKEYMAP },
	{ "f.autolower",            FKEYWORD, F_AUTOLOWER },
	{ "f.autoraise",            FKEYWORD, F_AUTORAISE },
	{ "f.backiconmgr",          FKEYWORD, F_BACKICONMGR },
	{ "f.backmapiconmgr",       FKEYWORD, F_BACKMAPICONMGR },
	{ "f.beep",                 FKEYWORD, F_BEEP },
	{ "f.bottomzoom",           FKEYWORD, F_BOTTOMZOOM },
	{ "f.changepriority",       FSKEYWORD, F_CHANGEPRIORITY },
	{ "f.changesize",           FSKEYWORD, F_CHANGESIZE },
	{ "f.circledown",           FKEYWORD, F_CIRCLEDOWN },
	{ "f.circleup",             FKEYWORD, F_CIRCLEUP },
	{ "f.colormap",             FSKEYWORD, F_COLORMAP },
	{ "f.deiconify",            FKEYWORD, F_DEICONIFY },
	{ "f.delete",               FKEYWORD, F_DELETE },
	{ "f.deleteordestroy",      FKEYWORD, F_DELETEORDESTROY },
	{ "f.deltastop",            FKEYWORD, F_DELTASTOP },
	{ "f.destroy",              FKEYWORD, F_DESTROY },
	{ "f.downiconmgr",          FKEYWORD, F_DOWNICONMGR },
	{ "f.downworkspace",        FKEYWORD, F_DOWNWORKSPACE },
	{ "f.exec",                 FSKEYWORD, F_EXEC },
	{ "f.fill",                 FSKEYWORD, F_FILL },
	{ "f.fittocontent",         FKEYWORD, F_FITTOCONTENT },
	{ "f.focus",                FKEYWORD, F_FOCUS },
	{ "f.forcemove",            FKEYWORD, F_FORCEMOVE },
	{ "f.forwiconmgr",          FKEYWORD, F_FORWICONMGR },
	{ "f.forwmapiconmgr",       FKEYWORD, F_FORWMAPICONMGR },
	{ "f.fullscreenzoom",       FKEYWORD, F_FULLSCREENZOOM },
	{ "f.fullzoom",             FKEYWORD, F_FULLZOOM },
	{ "f.function",             FSKEYWORD, F_FUNCTION },
	{ "f.gotoworkspace",        FSKEYWORD, F_GOTOWORKSPACE },
	{ "f.hbzoom",               FKEYWORD, F_BOTTOMZOOM },
	{ "f.hideiconmgr",          FKEYWORD, F_HIDELIST },
	{ "f.hideworkspacemgr",     FKEYWORD, F_HIDEWORKMGR },
	{ "f.horizoom",             FKEYWORD, F_HORIZOOM },
	{ "f.htzoom",               FKEYWORD, F_TOPZOOM },
	{ "f.hypermove",            FKEYWORD, F_HYPERMOVE },
	{ "f.hzoom",                FKEYWORD, F_HORIZOOM },
	{ "f.iconify",              FKEYWORD, F_ICONIFY },
	{ "f.identify",             FKEYWORD, F_IDENTIFY },
	{ "f.initsize",             FKEYWORD, F_INITSIZE },
	{ "f.jumpdown",             FSKEYWORD, F_JUMPDOWN },
	{ "f.jumpleft",             FSKEYWORD, F_JUMPLEFT },
	{ "f.jumpright",            FSKEYWORD, F_JUMPRIGHT },
	{ "f.jumpup",               FSKEYWORD, F_JUMPUP },
	{ "f.lefticonmgr",          FKEYWORD, F_LEFTICONMGR },
	{ "f.leftworkspace",        FKEYWORD, F_LEFTWORKSPACE },
	{ "f.leftzoom",             FKEYWORD, F_LEFTZOOM },
	{ "f.lower",                FKEYWORD, F_LOWER },
	{ "f.menu",                 FSKEYWORD, F_MENU },
	{ "f.move",                 FKEYWORD, F_MOVE },
	{ "f.movemenu",             FKEYWORD, F_MOVEMENU },
	{ "f.movepack",             FKEYWORD, F_MOVEPACK },
	{ "f.movepush",             FKEYWORD, F_MOVEPUSH },
	{ "f.moveresize",           FSKEYWORD, F_MOVERESIZE },
	{ "f.movetitlebar",         FKEYWORD, F_MOVETITLEBAR },
	{ "f.movetonextworkspace",  FKEYWORD, F_MOVETONEXTWORKSPACE },
	{ "f.movetonextworkspaceandfollow",  FKEYWORD, F_MOVETONEXTWORKSPACEANDFOLLOW },
	{ "f.movetoprevworkspace",  FKEYWORD, F_MOVETOPREVWORKSPACE },
	{ "f.movetoprevworkspaceandfollow",  FKEYWORD, F_MOVETOPREVWORKSPACEANDFOLLOW },
	{ "f.nexticonmgr",          FKEYWORD, F_NEXTICONMGR },
	{ "f.nextworkspace",        FKEYWORD, F_NEXTWORKSPACE },
	{ "f.nop",                  FKEYWORD, F_NOP },
	{ "f.occupy",               FKEYWORD, F_OCCUPY },
	{ "f.occupyall",            FKEYWORD, F_OCCUPYALL },
	{ "f.pack",                 FSKEYWORD, F_PACK },
	{ "f.pin",                  FKEYWORD, F_PIN },
	{ "f.previconmgr",          FKEYWORD, F_PREVICONMGR },
	{ "f.prevworkspace",        FKEYWORD, F_PREVWORKSPACE },
	{ "f.priorityswitching",    FKEYWORD, F_PRIORITYSWITCHING },
	{ "f.quit",                 FKEYWORD, F_QUIT },
	{ "f.raise",                FKEYWORD, F_RAISE },
	{ "f.raiseicons",           FKEYWORD, F_RAISEICONS },
	{ "f.raiselower",           FKEYWORD, F_RAISELOWER },
	{ "f.raiseorsqueeze",       FKEYWORD, F_RAISEORSQUEEZE },
	{ "f.refresh",              FKEYWORD, F_REFRESH },
	{ "f.removefromworkspace",  FSKEYWORD, F_REMOVEFROMWORKSPACE },
#ifdef SOUNDS
	{ "f.rereadsounds",         FKEYWORD, F_REREADSOUNDS },
#endif
	{ "f.rescuewindows",        FKEYWORD, F_RESCUE_WINDOWS },
	{ "f.resize",               FKEYWORD, F_RESIZE },
	{ "f.restart",              FKEYWORD, F_RESTART },
	{ "f.restoregeometry",      FKEYWORD, F_RESTOREGEOMETRY },
	{ "f.righticonmgr",         FKEYWORD, F_RIGHTICONMGR },
	{ "f.rightworkspace",       FKEYWORD, F_RIGHTWORKSPACE },
	{ "f.rightzoom",            FKEYWORD, F_RIGHTZOOM },
	{ "f.ring",                 FKEYWORD, F_RING },
	{ "f.savegeometry",         FKEYWORD, F_SAVEGEOMETRY },
	{ "f.saveyourself",         FKEYWORD, F_SAVEYOURSELF },
	{ "f.separator",            FKEYWORD, F_SEPARATOR },
	{ "f.setbuttonsstate",      FKEYWORD, F_SETBUTTONSTATE },
	{ "f.setmapstate",          FKEYWORD, F_SETMAPSTATE },
	{ "f.setpriority",          FSKEYWORD, F_SETPRIORITY },
	{ "f.showbackground",       FKEYWORD, F_SHOWBGRD },
	{ "f.showiconmgr",          FKEYWORD, F_SHOWLIST },
	{ "f.showworkspacemgr",     FKEYWORD, F_SHOWWORKMGR },
	{ "f.slowdownanimation",    FKEYWORD, F_SLOWDOWNANIMATION },
	{ "f.sorticonmgr",          FKEYWORD, F_SORTICONMGR },
	{ "f.speedupanimation",     FKEYWORD, F_SPEEDUPANIMATION },
	{ "f.squeeze",              FKEYWORD, F_SQUEEZE },
	{ "f.startanimation",       FKEYWORD, F_STARTANIMATION },
	{ "f.stopanimation",        FKEYWORD, F_STOPANIMATION },
	{ "f.switchpriority",       FKEYWORD, F_SWITCHPRIORITY },
	{ "f.tinylower",            FKEYWORD, F_TINYLOWER },
	{ "f.tinyraise",            FKEYWORD, F_TINYRAISE },
	{ "f.title",                FKEYWORD, F_TITLE },
	{ "f.toggleoccupation",     FSKEYWORD, F_TOGGLEOCCUPATION },
#ifdef SOUNDS
	{ "f.togglesound",          FKEYWORD, F_TOGGLESOUND },
#endif
	{ "f.togglestate",          FKEYWORD, F_TOGGLESTATE },
	{ "f.toggleworkspacemgr",   FKEYWORD, F_TOGGLEWORKMGR },
	{ "f.topzoom",              FKEYWORD, F_TOPZOOM },
	{ "f.trace",                FSKEYWORD, F_TRACE },
	{ "f.twmrc",                FKEYWORD, F_RESTART },
	{ "f.unfocus",              FKEYWORD, F_UNFOCUS },
	{ "f.unsqueeze",            FKEYWORD, F_UNSQUEEZE },
	{ "f.upiconmgr",            FKEYWORD, F_UPICONMGR },
	{ "f.upworkspace",          FKEYWORD, F_UPWORKSPACE },
	{ "f.vanish",               FKEYWORD, F_VANISH },
	{ "f.version",              FKEYWORD, F_VERSION },
	{ "f.vlzoom",               FKEYWORD, F_LEFTZOOM },
	{ "f.vrzoom",               FKEYWORD, F_RIGHTZOOM },
	{ "f.warphere",             FSKEYWORD, F_WARPHERE },
	{ "f.warpring",             FSKEYWORD, F_WARPRING },
	{ "f.warpto",               FSKEYWORD, F_WARPTO },
	{ "f.warptoiconmgr",        FSKEYWORD, F_WARPTOICONMGR },
	{ "f.warptoscreen",         FSKEYWORD, F_WARPTOSCREEN },
	{ "f.winrefresh",           FKEYWORD, F_WINREFRESH },
	{ "f.zoom",                 FKEYWORD, F_VERTZOOM },
	{ "forceicons",             KEYWORD, kw0_ForceIcons },
	{ "frame",                  FRAME, 0 },
	{ "framepadding",           NKEYWORD, kwn_FramePadding },
	{ "function",               FUNCTION, 0 },
	{ "i",                      ICON, 0 },
	{ "icon",                   ICON, 0 },
	{ "iconbackground",         CLKEYWORD, kwcl_IconBackground },
	{ "iconbordercolor",        CLKEYWORD, kwcl_IconBorderColor },
	{ "iconborderwidth",        NKEYWORD, kwn_IconBorderWidth },
	{ "icondirectory",          SKEYWORD, kws_IconDirectory },
	{ "iconfont",               SKEYWORD, kws_IconFont },
	{ "iconforeground",         CLKEYWORD, kwcl_IconForeground },
	{ "iconifybyunmapping",     ICONIFY_BY_UNMAPPING, 0 },
	{ "iconifyfunction",        ICONIFY_FUNCTION, 0 },
	{ "iconifystyle",           SKEYWORD, kws_IconifyStyle },
	{ "iconjustification",      SKEYWORD, kws_IconJustification },
	{ "iconmanagerbackground",  CLKEYWORD, kwcl_IconManagerBackground },
	{ "iconmanagerdontshow",    ICONMGR_NOSHOW, 0 },
	{ "iconmanagerfont",        SKEYWORD, kws_IconManagerFont },
	{ "iconmanagerforeground",  CLKEYWORD, kwcl_IconManagerForeground },
	{ "iconmanagergeometry",    ICONMGR_GEOMETRY, 0 },
	{ "iconmanagerhighlight",   CLKEYWORD, kwcl_IconManagerHighlight },
	{ "iconmanagers",           ICONMGRS, 0 },
	{ "iconmanagershadowdepth", NKEYWORD, kwn_IconManagerShadowDepth },
	{ "iconmanagershow",        ICONMGR_SHOW, 0 },
	{ "iconmenudontshow",       ICONMENU_DONTSHOW, 0 },
	{ "iconmgr",                ICONMGR, 0 },
	{ "iconregion",             ICON_REGION, 0 },
	{ "iconregionalignement",   SKEYWORD, kws_IconRegionAlignement },
	{ "iconregionjustification", SKEYWORD, kws_IconRegionJustification },
	{ "icons",                  ICONS, 0 },
	{ "iconsize",               SKEYWORD, kws_IconSize },
	{ "ignorecaseinmenuselection",      KEYWORD, kw0_IgnoreCaseInMenuSelection },
	{ "ignorelockmodifier",     KEYWORD, kw0_IgnoreLockModifier },
	{ "ignoremodifier",         IGNOREMODIFIER, 0 },
	{ "ignoretransient",        IGNORE_TRANSIENT, 0 },
	{ "interpolatemenucolors",  KEYWORD, kw0_InterpolateMenuColors },
	{ "l",                      LOCK, 0 },
	{ "left",                   SIJENUM, SIJ_LEFT },
	{ "lefttitlebutton",        LEFT_TITLEBUTTON, 0 },
	{ "lock",                   LOCK, 0 },
	{ "m",                      META, 0 },
	{ "maketitle",              MAKE_TITLE, 0 },
	{ "mapwindowbackground",    CLKEYWORD, kwcl_MapWindowBackground },
	{ "mapwindowcurrentworkspace", MAPWINDOWCURRENTWORKSPACE, 0},
	{ "mapwindowdefaultworkspace", MAPWINDOWDEFAULTWORKSPACE, 0},
	{ "mapwindowforeground",    CLKEYWORD, kwcl_MapWindowForeground },
	{ "maxicontitlewidth",      NKEYWORD, kwn_MaxIconTitleWidth },
	{ "maxwindowsize",          SKEYWORD, kws_MaxWindowSize },
	{ "menu",                   MENU, 0 },
	{ "menubackground",         CKEYWORD, kwc_MenuBackground },
	{ "menufont",               SKEYWORD, kws_MenuFont },
	{ "menuforeground",         CKEYWORD, kwc_MenuForeground },
	{ "menushadowcolor",        CKEYWORD, kwc_MenuShadowColor },
	{ "menushadowdepth",        NKEYWORD, kwn_MenuShadowDepth },
	{ "menutitlebackground",    CKEYWORD, kwc_MenuTitleBackground },
	{ "menutitleforeground",    CKEYWORD, kwc_MenuTitleForeground },
	{ "meta",                   META, 0 },
	{ "mod",                    META, 0 },  /* fake it */
	{ "monochrome",             MONOCHROME, 0 },
	{ "move",                   MOVE, 0 },
	{ "movedelta",              NKEYWORD, kwn_MoveDelta },
	{ "moveoffresistance",      NKEYWORD, kwn_MoveOffResistance },
	{ "movepackresistance",     NKEYWORD, kwn_MovePackResistance },
	{ "mwmignore",              MWM_IGNORE, 0 },
	{ "nobackingstore",         KEYWORD, kw0_NoBackingStore },
	{ "noborder",               NO_BORDER, 0 },
	{ "nocasesensitive",        KEYWORD, kw0_NoCaseSensitive },
	{ "nodefaults",             KEYWORD, kw0_NoDefaults },
	{ "nograbserver",           KEYWORD, kw0_NoGrabServer },
	{ "nohighlight",            NO_HILITE, 0 },
	{ "noiconmanagerfocus",     KEYWORD, kw0_NoIconManagerFocus },
	{ "noiconmanagers",         KEYWORD, kw0_NoIconManagers },
	{ "noicontitle",            NO_ICON_TITLE, 0  },
	{ "noimagesinworkspacemanager", KEYWORD, kw0_NoImagesInWorkSpaceManager },
	{ "nomenushadows",          KEYWORD, kw0_NoMenuShadows },
	{ "noopaquemove",           NOOPAQUEMOVE, 0 },
	{ "noopaqueresize",         NOOPAQUERESIZE, 0 },
	{ "noraiseondeiconify",     KEYWORD, kw0_NoRaiseOnDeiconify },
	{ "noraiseonmove",          KEYWORD, kw0_NoRaiseOnMove },
	{ "noraiseonresize",        KEYWORD, kw0_NoRaiseOnResize },
	{ "noraiseonwarp",          KEYWORD, kw0_NoRaiseOnWarp },
	{ "north",                  DKEYWORD, D_NORTH },
	{ "nosaveunders",           KEYWORD, kw0_NoSaveUnders },
	{ "noshowoccupyall",        KEYWORD, kw0_NoShowOccupyAll },
	{ "nostackmode",            NO_STACKMODE, 0 },
	{ "notitle",                NO_TITLE, 0 },
	{ "notitlefocus",           KEYWORD, kw0_NoTitleFocus },
	{ "notitlehighlight",       NO_TITLE_HILITE, 0 },
	{ "nowarptomenutitle",      KEYWORD, kw0_NoWarpToMenuTitle },
	{ "occupy",                 OCCUPYLIST, 0 },
	{ "occupyall",              OCCUPYALL, 0 },
	{ "ontoppriority",          ON_TOP_PRIORITY, 0 },
	{ "opaquemove",             OPAQUEMOVE, 0 },
	{ "opaquemovethreshold",    NKEYWORD, kwn_OpaqueMoveThreshold },
	{ "opaqueresize",           OPAQUERESIZE, 0 },
	{ "opaqueresizethreshold",  NKEYWORD, kwn_OpaqueResizeThreshold },
	{ "openwindowtimeout",      NKEYWORD, kwn_OpenWindowTimeout },
	{ "packnewwindows",         KEYWORD, kw0_PackNewWindows },
	{ "pixmapdirectory",        SKEYWORD, kws_PixmapDirectory },
	{ "pixmaps",                PIXMAPS, 0 },
	{ "prioritynotswitching",   PRIORITY_NOT_SWITCHING, 0 },
	{ "priorityswitching",      PRIORITY_SWITCHING, 0 },
	{ "r",                      ROOT, 0 },
	{ "raisedelay",             NKEYWORD, kwn_RaiseDelay },
	{ "raiseonclick",           KEYWORD, kw0_RaiseOnClick },
	{ "raiseonclickbutton",     NKEYWORD, kwn_RaiseOnClickButton },
	{ "raiseonwarp",            KEYWORD, kw0_RaiseOnWarp },
	{ "raisewhenautounsqueeze", KEYWORD, kw0_RaiseWhenAutoUnSqueeze },
	{ "randomplacement",        SSKEYWORD, kwss_RandomPlacement },
	{ "reallymoveinworkspacemanager",   KEYWORD, kw0_ReallyMoveInWorkspaceManager },
	{ "resize",                 RESIZE, 0 },
	{ "resizefont",             SKEYWORD, kws_ResizeFont },
	{ "restartpreviousstate",   KEYWORD, kw0_RestartPreviousState },
	{ "reversecurrentworkspace", KEYWORD, kw0_ReverseCurrentWorkspace },
	{ "right",                  SIJENUM, SIJ_RIGHT },
	{ "righttitlebutton",       RIGHT_TITLEBUTTON, 0 },
	{ "root",                   ROOT, 0 },
	{ "rplaysoundhost",         SKEYWORD, kws_RplaySoundHost },
	{ "rplaysounds",            RPLAY_SOUNDS, 0 },
	{ "s",                      SHIFT, 0 },
	{ "savecolor",              SAVECOLOR, 0},
	{ "saveworkspacefocus",     KEYWORD, kw0_SaveWorkspaceFocus },
	{ "schrinkicontitles",      KEYWORD, kw0_ShrinkIconTitles },
	{ "select",                 SELECT, 0 },
	{ "shift",                  SHIFT, 0 },
	{ "shortallwindowsmenus",   KEYWORD, kw0_ShortAllWindowsMenus },
	{ "showiconmanager",        KEYWORD, kw0_ShowIconManager },
	{ "showworkspacemanager",   KEYWORD, kw0_ShowWorkspaceManager },
	{ "shrinkicontitles",       KEYWORD, kw0_ShrinkIconTitles },
	{ "sloppyfocus",            KEYWORD, kw0_SloppyFocus },
	{ "sorticonmanager",        KEYWORD, kw0_SortIconManager },
	{ "soundhost",              SKEYWORD, kws_SoundHost },
	{ "south",                  DKEYWORD, D_SOUTH },
	{ "squeezetitle",           SQUEEZE_TITLE, 0 },
	{ "starticonified",         START_ICONIFIED, 0 },
	{ "startinmapstate",        KEYWORD, kw0_StartInMapState },
	{ "startsqueezed",          STARTSQUEEZED, 0 },
	{ "stayupmenus",            KEYWORD, kw0_StayUpMenus },
	{ "sunkfocuswindowtitle",   KEYWORD, kw0_SunkFocusWindowTitle },
	{ "t",                      TITLE, 0 },
	{ "threedborderwidth",      NKEYWORD, kwn_ThreeDBorderWidth },
	{ "title",                  TITLE, 0 },
	{ "titlebackground",        CLKEYWORD, kwcl_TitleBackground },
	{ "titlebuttonborderwidth", NKEYWORD, kwn_TitleButtonBorderWidth },
	{ "titlebuttonshadowdepth", NKEYWORD, kwn_TitleButtonShadowDepth },
	{ "titlefont",              SKEYWORD, kws_TitleFont },
	{ "titleforeground",        CLKEYWORD, kwcl_TitleForeground },
	{ "titlehighlight",         TITLE_HILITE, 0 },
	{ "titlejustification",     SKEYWORD, kws_TitleJustification },
	{ "titlepadding",           NKEYWORD, kwn_TitlePadding },
	{ "titleshadowdepth",       NKEYWORD, kwn_TitleShadowDepth },
	{ "transienthasoccupation", KEYWORD, kw0_TransientHasOccupation },
	{ "transientontop",         NKEYWORD, kwn_TransientOnTop },
	{ "unknownicon",            SKEYWORD, kws_UnknownIcon },
	{ "unmapbymovingfaraway",   UNMAPBYMOVINGFARAWAY, 0 },
	{ "usepposition",           SKEYWORD, kws_UsePPosition },
	{ "usesunktitlepixmap",     KEYWORD, kw0_UseSunkTitlePixmap },
	{ "usethreedborders",       KEYWORD, kw0_Use3DBorders },
	{ "usethreediconborders",   KEYWORD, kw0_use3DIconBorders },
	{ "usethreediconmanagers",  KEYWORD, kw0_Use3DIconManagers },
	{ "usethreedmenus",         KEYWORD, kw0_Use3DMenus },
	{ "usethreedtitles",        KEYWORD, kw0_Use3DTitles },
	{ "usethreedwmap",          KEYWORD, kw0_Use3DWMap },
	{ "virtualscreens",         VIRTUAL_SCREENS, 0 },
	{ "w",                      WINDOW, 0 },
	{ "wait",                   WAITC, 0 },
	{ "warpcursor",             WARP_CURSOR, 0 },
	{ "warpondeiconify",        WARP_ON_DEICONIFY, 0 },
	{ "warpringonscreen",       KEYWORD, kw0_WarpRingOnScreen },
	{ "warptodefaultmenuentry", KEYWORD, kw0_WarpToDefaultMenuEntry },
	{ "warpunmapped",           KEYWORD, kw0_WarpUnmapped },
	{ "west",                   DKEYWORD, D_WEST },
	{ "window",                 WINDOW, 0 },
	{ "windowbox",              WINDOW_BOX, 0 },
	{ "windowfunction",         WINDOW_FUNCTION, 0 },
	{ "windowgeometries",       WINDOW_GEOMETRIES, 0 },
	{ "windowregion",           WINDOW_REGION, 0 },
	{ "windowring",             WINDOW_RING, 0 },
	{ "windowringexclude",      WINDOW_RING_EXCLUDE, 0},
	{ "wmgrbuttonshadowdepth",  NKEYWORD, kwn_WMgrButtonShadowDepth },
	{ "wmgrbuttonstyle",        SKEYWORD, kws_WMgrButtonStyle },
	{ "wmgrhorizbuttonindent",  NKEYWORD, kwn_WMgrHorizButtonIndent },
	{ "wmgrvertbuttonindent",   NKEYWORD, kwn_WMgrVertButtonIndent },
	{ "workspace",              WORKSPACE, 0 },
	{ "workspacefont",          SKEYWORD, kws_WorkSpaceFont },
	{ "workspacemanagergeometry", WORKSPCMGR_GEOMETRY, 0 },
	{ "workspaces",             WORKSPACES, 0},
	{ "xmovegrid",              NKEYWORD, kwn_XMoveGrid },
	{ "xorvalue",               NKEYWORD, kwn_XorValue },
	{ "xpmicondirectory",       SKEYWORD, kws_PixmapDirectory },
	{ "ymovegrid",              NKEYWORD, kwn_YMoveGrid },
	{ "zoom",                   ZOOM, 0 },
};

static int numkeywords = (sizeof(keytable) / sizeof(keytable[0]));

int
parse_keyword(char *s, int *nump)
{
	int lower = 0, upper = numkeywords - 1;

	while(lower <= upper) {
		int middle = (lower + upper) / 2;
		TwmKeyword *p = &keytable[middle];
		int res = strcasecmp(p->name, s);

		if(res < 0) {
			lower = middle + 1;
		}
		else if(res == 0) {
			*nump = p->subnum;
			return p->value;
		}
		else {
			upper = middle - 1;
		}
	}
	return ERRORTOKEN;
}


/*
 * Simple tester function
 */
void
chk_keytable_order(void)
{
	int i;

	for(i = 0 ; i < (numkeywords - 1) ; i++) {
		if(strcasecmp(keytable[i].name, keytable[i + 1].name) >= 0) {
			fprintf(stderr, "%s: INTERNAL ERROR: keytable sorting: "
			        "'%s' >= '%s'\n", ProgramName,
			        keytable[i].name, keytable[i + 1].name);
		}
	}
}



/*
 * action routines called by grammar
 */

int
do_single_keyword(int keyword)
{
	switch(keyword) {
		case kw0_NoDefaults:
			Scr->NoDefaults = true;
			return 1;

		case kw0_AutoRelativeResize:
			Scr->AutoRelativeResize = true;
			return 1;

		case kw0_ForceIcons:
			if(Scr->FirstTime) {
				Scr->ForceIcon = true;
			}
			return 1;

		case kw0_NoIconManagers:
			Scr->NoIconManagers = true;
			return 1;

		case kw0_InterpolateMenuColors:
			if(Scr->FirstTime) {
				Scr->InterpolateMenuColors = true;
			}
			return 1;

		case kw0_SortIconManager:
			if(Scr->FirstTime) {
				Scr->SortIconMgr = true;
			}
			return 1;

		case kw0_NoGrabServer:
			Scr->NoGrabServer = true;
			return 1;

		case kw0_NoMenuShadows:
			if(Scr->FirstTime) {
				Scr->Shadow = false;
			}
			return 1;

		case kw0_NoRaiseOnMove:
			if(Scr->FirstTime) {
				Scr->NoRaiseMove = true;
			}
			return 1;

		case kw0_NoRaiseOnResize:
			if(Scr->FirstTime) {
				Scr->NoRaiseResize = true;
			}
			return 1;

		case kw0_NoRaiseOnDeiconify:
			if(Scr->FirstTime) {
				Scr->NoRaiseDeicon = true;
			}
			return 1;

		case kw0_DontMoveOff:
			Scr->DontMoveOff = true;
			return 1;

		case kw0_NoBackingStore:
			Scr->BackingStore = false;
			return 1;

		case kw0_NoSaveUnders:
			Scr->SaveUnder = false;
			return 1;

		case kw0_RestartPreviousState:
			RestartPreviousState = true;
			return 1;

		case kw0_ClientBorderWidth:
			if(Scr->FirstTime) {
				Scr->ClientBorderWidth = true;
			}
			return 1;

		case kw0_NoTitleFocus:
			Scr->TitleFocus = false;
			return 1;

		case kw0_DecorateTransients:
			Scr->DecorateTransients = true;
			return 1;

		case kw0_ShowIconManager:
			Scr->ShowIconManager = true;
			return 1;

		case kw0_ShowWorkspaceManager:
			Scr->ShowWorkspaceManager = true;
			return 1;

		case kw0_StartInMapState:
			Scr->workSpaceMgr.initialstate = MAPSTATE;
			return 1;

		case kw0_NoShowOccupyAll:
			Scr->workSpaceMgr.noshowoccupyall = true;
			return 1;

		case kw0_AutoOccupy:
			Scr->AutoOccupy = true;
			return 1;

		case kw0_AutoPriority:
			Scr->AutoPriority = true;
			return 1;

		case kw0_TransientHasOccupation:
			Scr->TransientHasOccupation = true;
			return 1;

		case kw0_DontPaintRootWindow:
			Scr->DontPaintRootWindow = true;
			return 1;

		case kw0_UseSunkTitlePixmap:
			Scr->UseSunkTitlePixmap = true;
			return 1;

		case kw0_Use3DBorders:
			Scr->use3Dborders = true;
			return 1;

		case kw0_Use3DIconManagers:
			Scr->use3Diconmanagers = true;
			return 1;

		case kw0_Use3DMenus:
			Scr->use3Dmenus = true;
			return 1;

		case kw0_Use3DTitles:
			Scr->use3Dtitles = true;
			return 1;

		case kw0_Use3DWMap:
			Scr->use3Dwmap = true;
			return 1;

		case kw0_SunkFocusWindowTitle:
			Scr->SunkFocusWindowTitle = true;
			return 1;

		case kw0_BeNiceToColormap:
			Scr->BeNiceToColormap = true;
			return 1;

		case kw0_BorderResizeCursors:
			Scr->BorderCursors = true;
			return 1;

		case kw0_NoCaseSensitive:
			Scr->CaseSensitive = false;
			return 1;

		case kw0_NoRaiseOnWarp:
			Scr->RaiseOnWarp = false;
			return 1;

		case kw0_RaiseOnWarp:
			Scr->RaiseOnWarp = true;
			return 1;

		case kw0_WarpUnmapped:
			Scr->WarpUnmapped = true;
			return 1;

		case kw0_WarpRingOnScreen:
			Scr->WarpRingAnyWhere = false;
			return 1;

		case kw0_NoIconManagerFocus:
			Scr->IconManagerFocus = false;
			return 1;

		case kw0_StayUpMenus:
			Scr->StayUpMenus = true;
			return 1;

		case kw0_ClickToFocus:
			Scr->ClickToFocus = true;
			return 1;

		case kw0_ReallyMoveInWorkspaceManager:
			Scr->ReallyMoveInWorkspaceManager = true;
			return 1;

		case kw0_ShowWinWhenMovingInWmgr:
			Scr->ShowWinWhenMovingInWmgr = true;
			return 1;

		case kw0_ReverseCurrentWorkspace:
			Scr->ReverseCurrentWorkspace = true;
			return 1;

		case kw0_DontWarpCursorInWMap:
			Scr->DontWarpCursorInWMap = true;
			return 1;

		case kw0_CenterFeedbackWindow:
			Scr->CenterFeedbackWindow = true;
			return 1;

		case kw0_WarpToDefaultMenuEntry:
			Scr->WarpToDefaultMenuEntry = true;
			return 1;

		case kw0_ShrinkIconTitles:
			Scr->ShrinkIconTitles = true;
			return 1;

		case kw0_AutoRaiseIcons:
			Scr->AutoRaiseIcons = true;
			return 1;

		/* kai */
		case kw0_AutoFocusToTransients:
			Scr->AutoFocusToTransients = true;
			return 1;

		case kw0_use3DIconBorders:
			Scr->use3Diconborders = true;
			return 1;

		case kw0_ShortAllWindowsMenus:
			Scr->ShortAllWindowsMenus = true;
			return 1;

		case kw0_RaiseWhenAutoUnSqueeze:
			Scr->RaiseWhenAutoUnSqueeze = true;
			return 1;

		case kw0_RaiseOnClick:
			Scr->RaiseOnClick = true;
			return 1;

		case kw0_IgnoreLockModifier:
			Scr->IgnoreModifier |= LockMask;
			return 1;

		case kw0_PackNewWindows:
			Scr->PackNewWindows = true;
			return 1;

		case kw0_IgnoreCaseInMenuSelection:
			Scr->IgnoreCaseInMenuSelection = true;
			return 1;

		case kw0_SloppyFocus:
			Scr->SloppyFocus = true;
			return 1;

		case kw0_SaveWorkspaceFocus:
			Scr->SaveWorkspaceFocus = true;
			return 1;

		case kw0_NoImagesInWorkSpaceManager:
			Scr->NoImagesInWorkSpaceManager = true;
			return 1;

		case kw0_NoWarpToMenuTitle:
			Scr->NoWarpToMenuTitle = true;
			return 1;

		case kw0_DontShowWelcomeWindow:
			Scr->ShowWelcomeWindow = false;
			return 1;

		case kw0_DontToggleWorkspacemanagerState:
			Scr->DontToggleWorkspaceManagerState = true;
			return 1;

	}
	return 0;
}


int
do_string_string_keyword(int keyword, char *s1, char *s2)
{
	switch(keyword) {
		case kwss_RandomPlacement: {
			int rp = ParseRandomPlacement(s1);
			if(rp < 0) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid RandomPlacement argument 1 \"%s\"\n", s1);
			}
			else {
				Scr->RandomPlacement = rp;
			}
		}
		{
			if(s2 == NULL) {
				return 1;
			}
			JunkMask = XParseGeometry(s2, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
#ifdef DEBUG
			fprintf(stderr, "DEBUG:: JunkMask = %x, WidthValue = %x, HeightValue = %x\n",
			        JunkMask, WidthValue, HeightValue);
			fprintf(stderr, "DEBUG:: JunkX = %d, JunkY = %d\n", JunkX, JunkY);
#endif
			if((JunkMask & (XValue | YValue)) !=
			                (XValue | YValue)) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid RandomPlacement displacement \"%s\"\n", s2);
			}
			else {
				Scr->RandomDisplacementX = JunkX;
				Scr->RandomDisplacementY = JunkY;
			}
			return 1;
		}
	}
	return 0;
}


int
do_string_keyword(int keyword, char *s)
{
	switch(keyword) {
		case kws_UsePPosition: {
			int ppos = ParseUsePPosition(s);
			if(ppos < 0) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid UsePPosition argument \"%s\"\n", s);
			}
			else {
				Scr->UsePPosition = ppos;
			}
			return 1;
		}

		case kws_IconFont:
			if(!Scr->HaveFonts) {
				Scr->IconFont.basename = s;
			}
			return 1;

		case kws_ResizeFont:
			if(!Scr->HaveFonts) {
				Scr->SizeFont.basename = s;
			}
			return 1;

		case kws_MenuFont:
			if(!Scr->HaveFonts) {
				Scr->MenuFont.basename = s;
			}
			return 1;

		case kws_WorkSpaceFont:
			if(!Scr->HaveFonts) {
				Scr->workSpaceMgr.windowFont.basename = s;
			}
			return 1;

		case kws_TitleFont:
			if(!Scr->HaveFonts) {
				Scr->TitleBarFont.basename = s;
			}
			return 1;

		case kws_IconManagerFont:
			if(!Scr->HaveFonts) {
				Scr->IconManagerFont.basename = s;
			}
			return 1;

		case kws_UnknownIcon:
			if(Scr->FirstTime) {
				Scr->UnknownImage = GetImage(s, Scr->IconC);
			}
			return 1;

		case kws_IconDirectory:
			if(Scr->FirstTime) {
				Scr->IconDirectory = ExpandFilePath(s);
			}
			return 1;

		case kws_PixmapDirectory:
			if(Scr->FirstTime) {
				Scr->PixmapDirectory = ExpandFilePath(s);
			}
			return 1;

		case kws_MaxWindowSize:
			JunkMask = XParseGeometry(s, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
			if((JunkMask & (WidthValue | HeightValue)) !=
			                (WidthValue | HeightValue)) {
				twmrc_error_prefix();
				fprintf(stderr, "bad MaxWindowSize \"%s\"\n", s);
				return 0;
			}
			if(JunkWidth == 0 || JunkHeight == 0) {
				twmrc_error_prefix();
				fprintf(stderr, "MaxWindowSize \"%s\" must be non-zero\n", s);
				return 0;
			}
			Scr->MaxWindowWidth = JunkWidth;
			Scr->MaxWindowHeight = JunkHeight;
			return 1;

		case kws_IconJustification: {
			int just = ParseTitleJustification(s);

			if((just < 0) || (just == TJ_UNDEF)) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid IconJustification argument \"%s\"\n", s);
			}
			else {
				Scr->IconJustification = just;
			}
			return 1;
		}
		case kws_IconRegionJustification: {
			int just = ParseIRJustification(s);

			if(just < 0 || (just == IRJ_UNDEF)) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid IconRegionJustification argument \"%s\"\n", s);
			}
			else {
				Scr->IconRegionJustification = just;
			}
			return 1;
		}
		case kws_IconRegionAlignement: {
			int just = ParseAlignement(s);

			if(just < 0) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid IconRegionAlignement argument \"%s\"\n", s);
			}
			else {
				Scr->IconRegionAlignement = just;
			}
			return 1;
		}

		case kws_TitleJustification: {
			int just = ParseTitleJustification(s);

			if((just < 0) || (just == TJ_UNDEF)) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid TitleJustification argument \"%s\"\n", s);
			}
			else {
				Scr->TitleJustification = just;
			}
			return 1;
		}
		case kws_RplaySoundHost:
		case kws_SoundHost:
			if(Scr->FirstTime) {
				/* Warning to be enabled in the future before removal */
				if(0 && keyword == kws_SoundHost) {
					twmrc_error_prefix();
					fprintf(stderr, "SoundHost is deprecated, please "
					        "use RplaySoundHost instead.\n");
				}
#ifdef SOUNDS
				set_sound_host(s);
#else
				twmrc_error_prefix();
				fprintf(stderr, "Ignoring %sSoundHost; rplay not ronfigured.\n",
				        (keyword == kws_RplaySoundHost ? "Rplay" : ""));
#endif
			}
			return 1;

		case kws_WMgrButtonStyle: {
			int style = ParseButtonStyle(s);

			if(style < 0) {
				twmrc_error_prefix();
				fprintf(stderr,
				        "ignoring invalid WMgrButtonStyle argument \"%s\"\n", s);
			}
			else {
				Scr->workSpaceMgr.buttonStyle = style;
			}
			return 1;
		}

		case kws_IconifyStyle: {
			if(strlen(s) == 0) {
				twmrc_error_prefix();
				fprintf(stderr, "ignoring invalid IconifyStyle argument \"%s\"\n", s);
			}
			if(strcasecmp(s, DEFSTRING) == 0) {
				Scr->IconifyStyle = ICONIFY_NORMAL;
			}
			if(strcasecmp(s, "normal") == 0) {
				Scr->IconifyStyle = ICONIFY_NORMAL;
			}
			if(strcasecmp(s, "mosaic") == 0) {
				Scr->IconifyStyle = ICONIFY_MOSAIC;
			}
			if(strcasecmp(s, "zoomin") == 0) {
				Scr->IconifyStyle = ICONIFY_ZOOMIN;
			}
			if(strcasecmp(s, "zoomout") == 0) {
				Scr->IconifyStyle = ICONIFY_ZOOMOUT;
			}
			if(strcasecmp(s, "fade") == 0) {
				Scr->IconifyStyle = ICONIFY_FADE;
			}
			if(strcasecmp(s, "sweep") == 0) {
				Scr->IconifyStyle = ICONIFY_SWEEP;
			}
			return 1;
		}

#ifdef EWMH
		case kws_IconSize:
			if(sscanf(s, "%dx%d", &Scr->PreferredIconWidth,
			                &Scr->PreferredIconHeight) == 2) {
				/* ok */
			}
			else if(sscanf(s, "%d", &Scr->PreferredIconWidth) == 1) {
				Scr->PreferredIconHeight = Scr->PreferredIconWidth;
			}
			else {
				Scr->PreferredIconHeight = Scr->PreferredIconWidth = 48;
			}
			return 1;
#endif
	}
	return 0;
}


int
do_number_keyword(int keyword, int num)
{
	switch(keyword) {
		case kwn_ConstrainedMoveTime:
			ConstrainedMoveTime = num;
			return 1;

		case kwn_MoveDelta:
			Scr->MoveDelta = num;
			return 1;

		case kwn_MoveOffResistance:
			Scr->MoveOffResistance = num;
			return 1;

		case kwn_MovePackResistance:
			if(num < 0) {
				num = 20;
			}
			Scr->MovePackResistance = num;
			return 1;

		case kwn_XMoveGrid:
			if(num <   1) {
				num =   1;
			}
			if(num > 100) {
				num = 100;
			}
			Scr->XMoveGrid = num;
			return 1;

		case kwn_YMoveGrid:
			if(num <   1) {
				num =   1;
			}
			if(num > 100) {
				num = 100;
			}
			Scr->YMoveGrid = num;
			return 1;

		case kwn_XorValue:
			if(Scr->FirstTime) {
				Scr->XORvalue = num;
			}
			return 1;

		case kwn_FramePadding:
			if(Scr->FirstTime) {
				Scr->FramePadding = num;
			}
			return 1;

		case kwn_TitlePadding:
			if(Scr->FirstTime) {
				Scr->TitlePadding = num;
			}
			return 1;

		case kwn_ButtonIndent:
			if(Scr->FirstTime) {
				Scr->ButtonIndent = num;
			}
			return 1;

		case kwn_ThreeDBorderWidth:
			if(Scr->FirstTime) {
				Scr->ThreeDBorderWidth = num;
			}
			return 1;

		case kwn_BorderWidth:
			if(Scr->FirstTime) {
				Scr->BorderWidth = num;
			}
			return 1;

		case kwn_IconBorderWidth:
			if(Scr->FirstTime) {
				Scr->IconBorderWidth = num;
			}
			return 1;

		case kwn_TitleButtonBorderWidth:
			if(Scr->FirstTime) {
				Scr->TBInfo.border = num;
			}
			return 1;

		case kwn_RaiseDelay:
			RaiseDelay = num;
			return 1;

		case kwn_TransientOnTop:
			if(Scr->FirstTime) {
				Scr->TransientOnTop = num;
			}
			return 1;

		case kwn_OpaqueMoveThreshold:
			if(Scr->FirstTime) {
				Scr->OpaqueMoveThreshold = num;
			}
			return 1;

		case kwn_OpaqueResizeThreshold:
			if(Scr->FirstTime) {
				Scr->OpaqueResizeThreshold = num;
			}
			return 1;

		case kwn_WMgrVertButtonIndent:
			if(Scr->FirstTime) {
				Scr->WMgrVertButtonIndent = num;
			}
			if(Scr->WMgrVertButtonIndent < 0) {
				Scr->WMgrVertButtonIndent = 0;
			}
			Scr->workSpaceMgr.vspace = Scr->WMgrVertButtonIndent;
			Scr->workSpaceMgr.occupyWindow->vspace = Scr->WMgrVertButtonIndent;
			return 1;

		case kwn_WMgrHorizButtonIndent:
			if(Scr->FirstTime) {
				Scr->WMgrHorizButtonIndent = num;
			}
			if(Scr->WMgrHorizButtonIndent < 0) {
				Scr->WMgrHorizButtonIndent = 0;
			}
			Scr->workSpaceMgr.hspace = Scr->WMgrHorizButtonIndent;
			Scr->workSpaceMgr.occupyWindow->hspace = Scr->WMgrHorizButtonIndent;
			return 1;

		case kwn_WMgrButtonShadowDepth:
			if(Scr->FirstTime) {
				Scr->WMgrButtonShadowDepth = num;
			}
			if(Scr->WMgrButtonShadowDepth < 1) {
				Scr->WMgrButtonShadowDepth = 1;
			}
			return 1;

		case kwn_MaxIconTitleWidth:
			if(Scr->FirstTime) {
				Scr->MaxIconTitleWidth = num;
			}
			return 1;

		case kwn_ClearShadowContrast:
			if(Scr->FirstTime) {
				Scr->ClearShadowContrast = num;
			}
			if(Scr->ClearShadowContrast <   0) {
				Scr->ClearShadowContrast =   0;
			}
			if(Scr->ClearShadowContrast > 100) {
				Scr->ClearShadowContrast = 100;
			}
			return 1;

		case kwn_DarkShadowContrast:
			if(Scr->FirstTime) {
				Scr->DarkShadowContrast = num;
			}
			if(Scr->DarkShadowContrast <   0) {
				Scr->DarkShadowContrast =   0;
			}
			if(Scr->DarkShadowContrast > 100) {
				Scr->DarkShadowContrast = 100;
			}
			return 1;

		case kwn_AnimationSpeed:
			if(num < 0) {
				num = 0;
			}
			SetAnimationSpeed(num);
			return 1;

		case kwn_BorderShadowDepth:
			if(Scr->FirstTime) {
				Scr->BorderShadowDepth = num;
			}
			if(Scr->BorderShadowDepth < 0) {
				Scr->BorderShadowDepth = 2;
			}
			return 1;

		case kwn_BorderLeft:
			if(Scr->FirstTime) {
				Scr->BorderLeft = num;
			}
			if(Scr->BorderLeft < 0) {
				Scr->BorderLeft = 0;
			}
			return 1;

		case kwn_BorderRight:
			if(Scr->FirstTime) {
				Scr->BorderRight = num;
			}
			if(Scr->BorderRight < 0) {
				Scr->BorderRight = 0;
			}
			return 1;

		case kwn_BorderTop:
			if(Scr->FirstTime) {
				Scr->BorderTop = num;
			}
			if(Scr->BorderTop < 0) {
				Scr->BorderTop = 0;
			}
			return 1;

		case kwn_BorderBottom:
			if(Scr->FirstTime) {
				Scr->BorderBottom = num;
			}
			if(Scr->BorderBottom < 0) {
				Scr->BorderBottom = 0;
			}
			return 1;

		case kwn_TitleButtonShadowDepth:
			if(Scr->FirstTime) {
				Scr->TitleButtonShadowDepth = num;
			}
			if(Scr->TitleButtonShadowDepth < 0) {
				Scr->TitleButtonShadowDepth = 2;
			}
			return 1;

		case kwn_TitleShadowDepth:
			if(Scr->FirstTime) {
				Scr->TitleShadowDepth = num;
			}
			if(Scr->TitleShadowDepth < 0) {
				Scr->TitleShadowDepth = 2;
			}
			return 1;

		case kwn_IconManagerShadowDepth:
			if(Scr->FirstTime) {
				Scr->IconManagerShadowDepth = num;
			}
			if(Scr->IconManagerShadowDepth < 0) {
				Scr->IconManagerShadowDepth = 2;
			}
			return 1;

		case kwn_MenuShadowDepth:
			if(Scr->FirstTime) {
				Scr->MenuShadowDepth = num;
			}
			if(Scr->MenuShadowDepth < 0) {
				Scr->MenuShadowDepth = 2;
			}
			return 1;

		case kwn_OpenWindowTimeout:
			if(Scr->FirstTime) {
				Scr->OpenWindowTimeout = num;
			}
			if(Scr->OpenWindowTimeout < 0) {
				Scr->OpenWindowTimeout = 0;
			}
			return 1;

		case kwn_RaiseOnClickButton:
			if(Scr->FirstTime) {
				Scr->RaiseOnClickButton = num;
			}
			if(Scr->RaiseOnClickButton < 1) {
				Scr->RaiseOnClickButton = 1;
			}
			if(Scr->RaiseOnClickButton > MAX_BUTTONS) {
				Scr->RaiseOnClickButton = MAX_BUTTONS;
			}
			return 1;


	}

	return 0;
}

name_list **
do_colorlist_keyword(int keyword, int colormode, char *s)
{
	switch(keyword) {
		case kwcl_BorderColor:
			GetColor(colormode, &Scr->BorderColorC.back, s);
			return &Scr->BorderColorL;

		case kwcl_IconManagerHighlight:
			GetColor(colormode, &Scr->IconManagerHighlight, s);
			return &Scr->IconManagerHighlightL;

		case kwcl_BorderTileForeground:
			GetColor(colormode, &Scr->BorderTileC.fore, s);
			return &Scr->BorderTileForegroundL;

		case kwcl_BorderTileBackground:
			GetColor(colormode, &Scr->BorderTileC.back, s);
			return &Scr->BorderTileBackgroundL;

		case kwcl_TitleForeground:
			GetColor(colormode, &Scr->TitleC.fore, s);
			return &Scr->TitleForegroundL;

		case kwcl_TitleBackground:
			GetColor(colormode, &Scr->TitleC.back, s);
			return &Scr->TitleBackgroundL;

		case kwcl_IconForeground:
			GetColor(colormode, &Scr->IconC.fore, s);
			return &Scr->IconForegroundL;

		case kwcl_IconBackground:
			GetColor(colormode, &Scr->IconC.back, s);
			return &Scr->IconBackgroundL;

		case kwcl_IconBorderColor:
			GetColor(colormode, &Scr->IconBorderColor, s);
			return &Scr->IconBorderColorL;

		case kwcl_IconManagerForeground:
			GetColor(colormode, &Scr->IconManagerC.fore, s);
			return &Scr->IconManagerFL;

		case kwcl_IconManagerBackground:
			GetColor(colormode, &Scr->IconManagerC.back, s);
			return &Scr->IconManagerBL;

		case kwcl_MapWindowBackground:
			GetColor(colormode, &Scr->workSpaceMgr.windowcp.back, s);
			Scr->workSpaceMgr.windowcpgiven = true;
			return &Scr->workSpaceMgr.windowBackgroundL;

		case kwcl_MapWindowForeground:
			GetColor(colormode, &Scr->workSpaceMgr.windowcp.fore, s);
			Scr->workSpaceMgr.windowcpgiven = true;
			return &Scr->workSpaceMgr.windowForegroundL;
	}
	return NULL;
}

int
do_color_keyword(int keyword, int colormode, char *s)
{
	switch(keyword) {
		case kwc_DefaultForeground:
			GetColor(colormode, &Scr->DefaultC.fore, s);
			return 1;

		case kwc_DefaultBackground:
			GetColor(colormode, &Scr->DefaultC.back, s);
			return 1;

		case kwc_MenuForeground:
			GetColor(colormode, &Scr->MenuC.fore, s);
			return 1;

		case kwc_MenuBackground:
			GetColor(colormode, &Scr->MenuC.back, s);
			return 1;

		case kwc_MenuTitleForeground:
			GetColor(colormode, &Scr->MenuTitleC.fore, s);
			return 1;

		case kwc_MenuTitleBackground:
			GetColor(colormode, &Scr->MenuTitleC.back, s);
			return 1;

		case kwc_MenuShadowColor:
			GetColor(colormode, &Scr->MenuShadowColor, s);
			return 1;

	}

	return 0;
}

/*
 * put_pixel_on_root() Save a pixel value in twm root window color property.
 */
static void
put_pixel_on_root(Pixel pixel)
{
	int           i, addPixel = 1;
	Atom          retAtom;
	int           retFormat;
	unsigned long nPixels, retAfter;
	Pixel        *retProp;

	if(XGetWindowProperty(dpy, Scr->Root, XA__MIT_PRIORITY_COLORS, 0, 8192,
	                      False, XA_CARDINAL, &retAtom,
	                      &retFormat, &nPixels, &retAfter,
	                      (unsigned char **)&retProp) != Success || !retProp) {
		return;
	}

	for(i = 0; i < nPixels; i++)
		if(pixel == retProp[i]) {
			addPixel = 0;
		}

	XFree((char *)retProp);

	if(addPixel)
		XChangeProperty(dpy, Scr->Root, XA__MIT_PRIORITY_COLORS,
		                XA_CARDINAL, 32, PropModeAppend,
		                (unsigned char *)&pixel, 1);
}

/*
 * do_string_savecolor() save a color from a string in the twmrc file.
 */
int
do_string_savecolor(int colormode, char *s)
{
	Pixel p;
	GetColor(colormode, &p, s);
	put_pixel_on_root(p);
	return 0;
}

/*
 * do_var_savecolor() save a color from a var in the twmrc file.
 */
typedef struct _cnode {
	int i;
	struct _cnode *next;
} Cnode, *Cptr;
static Cptr chead = NULL;

int
do_var_savecolor(int key)
{
	Cptr cptrav, cpnew;
	if(!chead) {
		chead = malloc(sizeof(Cnode));
		chead->i = key;
		chead->next = NULL;
	}
	else {
		cptrav = chead;
		while(cptrav->next != NULL) {
			cptrav = cptrav->next;
		}
		cpnew = malloc(sizeof(Cnode));
		cpnew->i = key;
		cpnew->next = NULL;
		cptrav->next = cpnew;
	}
	return 0;
}

/*
 * assign_var_savecolor() traverse the var save color list placeing the pixels
 *                        in the root window property.
 */
void
assign_var_savecolor(void)
{
	Cptr cp = chead;
	while(cp != NULL) {
		switch(cp->i) {
			case kwcl_BorderColor:
				put_pixel_on_root(Scr->BorderColorC.back);
				break;
			case kwcl_IconManagerHighlight:
				put_pixel_on_root(Scr->IconManagerHighlight);
				break;
			case kwcl_BorderTileForeground:
				put_pixel_on_root(Scr->BorderTileC.fore);
				break;
			case kwcl_BorderTileBackground:
				put_pixel_on_root(Scr->BorderTileC.back);
				break;
			case kwcl_TitleForeground:
				put_pixel_on_root(Scr->TitleC.fore);
				break;
			case kwcl_TitleBackground:
				put_pixel_on_root(Scr->TitleC.back);
				break;
			case kwcl_IconForeground:
				put_pixel_on_root(Scr->IconC.fore);
				break;
			case kwcl_IconBackground:
				put_pixel_on_root(Scr->IconC.back);
				break;
			case kwcl_IconBorderColor:
				put_pixel_on_root(Scr->IconBorderColor);
				break;
			case kwcl_IconManagerForeground:
				put_pixel_on_root(Scr->IconManagerC.fore);
				break;
			case kwcl_IconManagerBackground:
				put_pixel_on_root(Scr->IconManagerC.back);
				break;
			case kwcl_MapWindowForeground:
				put_pixel_on_root(Scr->workSpaceMgr.windowcp.fore);
				break;
			case kwcl_MapWindowBackground:
				put_pixel_on_root(Scr->workSpaceMgr.windowcp.back);
				break;
		}
		cp = cp->next;
	}
	if(chead) {
		free(chead);
		chead = NULL;
	}
}

static int
ParseRandomPlacement(char *s)
{
	if(s == NULL) {
		return RP_ALL;
	}
	if(strlen(s) == 0) {
		return RP_ALL;
	}
	if(strcasecmp(s, DEFSTRING) == 0) {
		return RP_ALL;
	}
	if(strcasecmp(s, "off") == 0) {
		return RP_OFF;
	}
	if(strcasecmp(s, "on") == 0) {
		return RP_ALL;
	}
	if(strcasecmp(s, "all") == 0) {
		return RP_ALL;
	}
	if(strcasecmp(s, "unmapped") == 0) {
		return RP_UNMAPPED;
	}
	return (-1);
}


/*
 * Parse out IconRegionJustification string.
 *
 * X-ref comment on ParseAlignement about return value.
 */
int
ParseIRJustification(const char *s)
{
	if(strlen(s) == 0) {
		return -1;
	}

#define CHK(str, ret) if(strcasecmp(s, str) == 0) { return IRJ_##ret; }
	CHK(DEFSTRING, CENTER);
	CHK("undef",   UNDEF);
	CHK("left",    LEFT);
	CHK("center",  CENTER);
	CHK("right",   RIGHT);
	CHK("border",  BORDER);
#undef CHK

	return -1;
}


/*
 * Parse out string for title justification.  From TitleJustification,
 * IconJustification, iconjust arg to IconRegion.
 *
 * X-ref comment on ParseAlignement about return value.
 */
int
ParseTitleJustification(const char *s)
{
	if(strlen(s) == 0) {
		return -1;
	}

#define CHK(str, ret) if(strcasecmp(s, str) == 0) { return TJ_##ret; }
	/* XXX Different uses really have different defaults... */
	CHK(DEFSTRING, CENTER);
	CHK("undef",   UNDEF);
	CHK("left",    LEFT);
	CHK("center",  CENTER);
	CHK("right",   RIGHT);
#undef CHK

	return -1;
}


/*
 * Parse out the string specifier for IconRegion Alignement[sic].
 * Strictly speaking, this [almost always] returns an IRAlignement enum
 * value.  However, it's specified as int to allow the -1 return for
 * invalid values.  enum's start numbering from 0 (unless specific values
 * are given), so that's a safe out-of-bounds value.  And making an
 * IRA_INVALID value would just add unnecessary complication, since
 * during parsing is the only time it makes sense.
 */
int
ParseAlignement(const char *s)
{
	if(strlen(s) == 0) {
		return -1;
	}

#define CHK(str, ret) if(strcasecmp(s, str) == 0) { return IRA_##ret; }
	CHK(DEFSTRING, CENTER);
	CHK("center",  CENTER);
	CHK("top",     TOP);
	CHK("bottom",  BOTTOM);
	CHK("border",  BORDER);
	CHK("undef",   UNDEF);
#undef CHK

	return -1;
}

static int
ParseUsePPosition(char *s)
{
	if(strlen(s) == 0) {
		return (-1);
	}
	if(strcasecmp(s, DEFSTRING) == 0) {
		return PPOS_OFF;
	}
	if(strcasecmp(s, "off") == 0) {
		return PPOS_OFF;
	}
	if(strcasecmp(s, "on") == 0) {
		return PPOS_ON;
	}
	if(strcasecmp(s, "non-zero") == 0) {
		return PPOS_NON_ZERO;
	}
	if(strcasecmp(s, "nonzero") == 0) {
		return PPOS_NON_ZERO;
	}
	return (-1);
}

static int
ParseButtonStyle(char *s)
{
	if(s == NULL || strlen(s) == 0) {
		return -1;
	}

#define CHK(str, ret) if(strcasecmp(s, str) == 0) { return STYLE_##ret; }
	CHK(DEFSTRING, NORMAL);
	CHK("normal",  NORMAL);
	CHK("style1",  STYLE1);
	CHK("style2",  STYLE2);
	CHK("style3",  STYLE3);
#undef CHK

	return -1;
}

int
do_squeeze_entry(name_list **slist,  /* squeeze or dont-squeeze list */
                 char *name,       /* window name */
                 SIJust justify,   /* left, center, right */
                 int num,          /* signed num */
                 int denom)        /* 0 or indicates fraction denom */
{
	int absnum = (num < 0 ? -num : num);

	if(denom < 0) {
		twmrc_error_prefix();
		fprintf(stderr, "negative SqueezeTitle denominator %d\n", denom);
		return (1);
	}
	if(absnum > denom && denom != 0) {
		twmrc_error_prefix();
		fprintf(stderr, "SqueezeTitle fraction %d/%d outside window\n",
		        num, denom);
		return (1);
	}
	/* Process the special cases from the manual here rather than
	 * each time we calculate the position of the title bar
	 * in ComputeTitleLocation().
	 * In fact, it's better to get rid of them entirely, but we
	 * probably should not do that for compatibility's sake.
	 * By using a non-zero denominator the position will be relative.
	 */
	if(denom == 0 && num == 0) {
		if(justify == SIJ_CENTER) {
			num = 1;
			denom = 2;
		}
		else if(justify == SIJ_RIGHT) {
			num = 2;
			denom = 2;
		}
		twmrc_error_prefix();
		fprintf(stderr, "deprecated SqueezeTitle faction 0/0, assuming %d/%d\n",
		        num, denom);
	}

	if(HasShape) {
		SqueezeInfo *sinfo;
		sinfo = malloc(sizeof(SqueezeInfo));

		if(!sinfo) {
			twmrc_error_prefix();
			fprintf(stderr, "unable to allocate %lu bytes for squeeze info\n",
			        (unsigned long) sizeof(SqueezeInfo));
			return (1);
		}
		sinfo->justify = justify;
		sinfo->num = num;
		sinfo->denom = denom;
		AddToList(slist, name, sinfo);
	}
	return (0);
}


/*
 * Parsing for EWMHIgnore { } lists
 */
void
proc_ewmh_ignore(void)
{
#ifndef EWMH
	twmrc_error_prefix();
	fprintf(stderr, "EWMH not enabled, EWMHIgnore { } ignored.\n");
	ParseError = true;
	return;
#endif
	/* else nada */
	return;
}
void
add_ewmh_ignore(char *s)
{
#ifndef EWMH
	return;
#else

#define HANDLE(x) \
        if(strcasecmp(s, (x)) == 0) { \
                AddToList(&Scr->EWMHIgnore, (x), ""); \
                return; \
        }
	HANDLE("STATE_MAXIMIZED_VERT");
	HANDLE("STATE_MAXIMIZED_HORZ");
	HANDLE("STATE_FULLSCREEN");
	HANDLE("STATE_SHADED");
	HANDLE("STATE_ABOVE");
	HANDLE("STATE_BELOW");
#undef HANDLE

	twmrc_error_prefix();
	fprintf(stderr, "Unexpected EWMHIgnore value '%s'\n", s);
	ParseError = true;
	return;
#endif /* EWMH */
}


/*
 * Parsing for MWMIgnore { } lists
 */
void
proc_mwm_ignore(void)
{
	/* Nothing to do */
	return;
}
void
add_mwm_ignore(char *s)
{
#define HANDLE(x) \
        if(strcasecmp(s, (x)) == 0) { \
                AddToList(&Scr->MWMIgnore, (x), ""); \
                return; \
        }
	HANDLE("DECOR_BORDER");
	HANDLE("DECOR_TITLE");
#undef HANDLE

	twmrc_error_prefix();
	fprintf(stderr, "Unexpected MWMIgnore value '%s'\n", s);
	ParseError = true;
	return;
}
