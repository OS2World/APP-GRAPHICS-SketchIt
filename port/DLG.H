#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>

#define HANDLESIZE      10       /* selection handle size 10 * 0.1 mm */

#define PATSYM_GRADIENTFILL  20L /* drawit specific pattern symbol */
#define PATSYM_FOUNTAINFILL  21L /* drawit specific pattern symbol */

#define CREATEMODE    105
#define IDC_SPIN        951      /* Spin button in layer dialog         */
#define IDD_QUERYPRINT 1000
#define IDD_QPLISTBOX  1005
#define IDD_QPJOBPROP  1010
#define IDD_INP        100
#define IDD_ABOUT      11

#define MAXLAYER      10               /* Max supported layers */
#define MINLAYER      1                /* First layer for this time */

#define SHADOWMIN    10
#define SHADOWMED    15
#define SHADOWMAX    30

#define FONT_INIT   1
#define FONT_SET    2

#define LCID_DRWSCREENFONT 3L           /* Screen font ID */
/*MESSAGES*/

#define IDS_FONTDLGTITLE        205
#define IDMSG_CANNOTLOADSTRING  206

#define WM_COLORING WM_USER + 2
#define WM_FROMTO   WM_USER + 3

/*define the object classes */
#define CLS_UNKNOWN  0
#define CLS_TXT      1
#define CLS_SQR      2
#define CLS_CIR      3
#define CLS_IMG      4
#define CLS_LIN      5
#define CLS_BOX      6         /* Box?                       */
#define CLS_META     7         /* METAFILE support           */
#define CLS_SPLINE   8         /* Polyline / Polyspline      */
#define CLS_GROUP    9         /* Used in Group contructions */
#define CLS_GROUPEND 10        /* Used as end of chain marker in a group */
#define CLS_BLOCKTEXT 11

/*-------------------------------------------------------------------------*/
/* Program states.                                                         */
/*-------------------------------------------------------------------------*/
#define NOSELECT      0
#define TEXTSELECT    1
#define IMGSELECT     2
#define CIRSELECT     3
#define SQRSELECT     4
#define MULTISELECT   5
#define IMGCROP       6
#define LINEDRAW      7
#define FREELINEDRAW  8
#define SPECLINEDRAW  9     /* SPECIAL LINE DRAW FEATURE  */
#define CIRCLEDRAW   10     /* CIRCLE DRAWING MODE        CLASS_CIRCLE       */
#define FCIRCLEDRAW  11     /* FILLED CIRCLE DRAWING MODE CLASS_CIRCLE       */
#define SQUAREDRAW   12     /* SQUARE DRAWING MODE        CLASS_SQUARE       */
#define FSQUAREDRAW  13     /* FILLED SQUARE DRAWING MODE CLASS_SQUARE       */
#define ELLIPSEDRAW  14     /* ELLIPSE DRAWING MODE       CLASS_CIRCLE       */
#define FLLIPSEDRAW  15     /* FILLED ELLIPSE DRAWING MODE CLASS_CIRCLE      */
#define FILLING      16     /* FILLING MODE, COLOR PALL   */
#define TRIANGLEDRAW 17     /* TRIANGLE DRAWING MODE      */
#define TRIANGLESELECT 18   /* TRIANGLE SELECTED          */
#define PARTIALARCDRAW 19   /* PARTIAL ARC DRAWING   CLASS_CIRCLE            */
#define LINESELECT     20   /* Yes we selected a line!!!  */
#define MULTIMOVE      21   /* Multimove mode...          */
#define MULTICOPY      22   /* Multicopy mode...          */
#define TEXTINPUT      23   /* Busy typing text.          */
#define METAPICSELECT  24   /* A metafile picture is selected */
#define SPLINESELECT   25   /* A spline is selected.      */
#define SPLINEDRAW     26   /* Busy drawing splines.      */
#define GROUPSELECT    27   /* A Group is selected.       */
#define OBJFORMCHANGE  28   /* Changing the form of an object */
#define REGPOLYDRAW    29  /* Creation of regular polypoint. CLS_SPLINE*/
#define REGPOLYSTAR    30  /* Creation of regular polystar   CLS_SPLINE*/
#define FINDCOLOR      31  /* Find the color under mouse ptr */
#define OBJROTATION    32  /* We start the rotation of the selectect obj*/
#define MOVECENTER     33  /* The center is selected and is about to move*/
#define FREESTYLE      34  /* Freestyle polygon drawing                  */
#define BOXDRAW        35
#define BOXSELECT      36
#define BLOCKTEXTSEL   37  /* BLOCKTEXT selected?        */
#define BLOCKTEXTCREATE 38 /* BLOCKTEXT creation.        */
#define INSERTPICTURE  39
/*
** Stretching objects
*/
#define LINESTRETCH    40   /* selected a line + we are at one of the ends   */
#define TEXTSTRETCH    42
#define IMAGESTRETCH   44
/*
** Supported formats.
*/
#define FILE_DRAWIT24   1
#define FILE_DRAWIT25   2
#define FILE_DRAWIT28   3
#define FILE_DRAWIT29   4
#define FILE_DRAWIT30   5
#define FILE_DRAWIT32   6    /* DrawIt version 3.2 */

extern HWND hwndColor;       /*Color ValueSet           */
extern HWND hwndFrame;       /*main frame handle        */
extern HWND hwndClient;      /*main client window handle*/
extern HAB  hab;

typedef struct  _ColorStruct {
                        USHORT rgb[3];
                        char   ClientTitle[30];
                        HWND   hwnd;
                     } *pColorStruct,ColorStruct;


/*
** Used to restore the original palette in palette based
** images.
**
** Is added to the pobject (non persistent data) structure in
** dlg.h
*/

typedef struct _drw_palette
{
   int    nColors;
   PRGB2  prgb2;
} drw_palette;
/*
** Paint and move method prototype for all objects. 
** see drwutl.c for details.
*/

typedef struct _object         *POBJECT;
typedef struct _wininfo        *PWINDOWINFO;

typedef void ( * moveoutlineproc )(POBJECT pObj, PWINDOWINFO pwi, SHORT dx, SHORT dy);
typedef void ( * paintproc       )(HPS hps,PWINDOWINFO pwi,POBJECT pObj,RECTL *rcl);
typedef void ( * invareaproc     )(POBJECT pObject,RECTL *prcl,PWINDOWINFO pwi, BOOL bInc);
/*
** OUR overall object structure.. 
** Contains the non persistant data and object state.
*/
#define MAX_NRWIDTH 255

typedef struct _object
{
   struct _object *Next;
   struct _object *Previous;
   struct _object *NextChild;
   struct _object *PrevChild;
   /*
   ** function pointers for moving, painting and invalidation area
   */
   moveoutlineproc moveOutline;
   paintproc       paint;
   invareaproc     getInvalidationArea;

   USHORT  usClass;
   BOOL    bMultiSel;  /* Are we multiselected as object?                   */
   RECTL   rclOutline; /* Object boundary in device independant format!!    */
   RECTLF  rclf;       /* Normalized object area. For clipping on printer PS*/
   BOOL    bSuppressfill; /* Suppress the filling, to speedup the drawing   */
   int     iHandle;      /* selected handle when draghandle is active.      */
   drw_palette *pDrwPal; /* Original palette of loaded image.               */
   BOOL    bLocked;      /* TRUE | FALSE. May this object be deleted...     */
   LONG    lWidth[MAX_NRWIDTH];  /* Character width table                   */
   int     iSelected;    /* Selected column in the block text object.       */
   BOOL    bPrepPrint;   /* Prepare printing. (see drwbtext.c)              */
   BOOL    bDirty;       /* Set after a fileload. Used in dlg_txt.c....     */
   void    *pvData;      /* Used for printing preparation.                  */
} OBJECT;

extern POBJECT straddres;

/*filled and used in dialog.c */

typedef  struct _wininfo
        {
        SHORT cxClient;        /* Client window width in pixels        */
        SHORT cyClient;        /* Client window heigth in pixels       */
        SHORT cxClnt;          /* Client window size at startup        */
        SHORT cyClnt;          /* Client window size at startup        */
        SHORT cxCanvas;        /* Canvas size in pixels.               */
        SHORT cyCanvas;        /* Canvas size in pixels.               */
        ULONG  lBackClr;       /* Current background color             */
        ULONG  ulColor;        /* current selected fore color          */
        ULONG  ulOutLineColor; /* Guess what??                         */
        ULONG  ulItem;         /* checked item in the clrvalset        */
        LONG   ColorPattern;   /* Current selected fill pattern        */
        LONG   lLntype;        /* Current selected linetype            */
        LONG   lLnWidth;       /* Current selected Line width          */
        LONG   lLnJoin;        /* Current selected Line join.          */
        LONG   lLnEnd;         /* Current selected Line end.           */
        LONG   xPixels;        /* Number of pixels/PU_LOMETRIC         */
        LONG   yPixels;        /* Number of pixels/PU_LOMETRIC         */
        USHORT uslayer;        /* Active layer                         */
        USHORT usdrawlayer;    /* Drawing layer used in PAINT          */
        USHORT usFormHeight;   /* The formheight of an A4              */
        USHORT usFormWidth;    /* The formwidth of an A4               */
        USHORT usHeight;       /* The formheight of an A4  [readonly]  */
        USHORT usWidth;        /* The formwidth of an A4   [readonly]  */
        float  fOffx;          /* The offset or relative pos to add    */
        float  fOffy;          /* The offset or relative pos to add    */
        FIXED  fxArc;          /* Number of degrees 0-360 for an arc   */
        float  uXfactor;       /* Screen = 1 while printer mostly > 1  */
        float  uYfactor;       /* Screen = 1 while printer mostly > 1  */
        FATTRS fattrs;
        HPS    hps;            /* Can be for the printer / window.     */
        HPS    hpsScreen;
        HPS    hpsMem;         /* Memory PS for bitmaps (printer & win)*/
        HDC    hdcMem;         /*                                      */
        HAB    hab;            /* Our main anchorblock handle.         */
        HDC    hdcClient;      /* Our device context.                  */
        HWND   hwndClient;     /* Our Client window  handle.           */
        HWND   hwndMain;       /* Our Main window, used to send um_* to*/
        HWND   hwndVscroll;    /* Vertical scrollbar windowhandle      */
        HWND   hwndHscroll;    /* Horizontal scrollbar windowhandle    */
        HWND   hwndImgMenu;    /* Image popupmenu.                     */
        HWND   hwndOptMenu;    /* Options popupmenu.                   */
        HWND   hwndTxtMenu;    /* Text popupmenu.                      */
        HWND   hwndClipMenu;   /* ClipPath popupmenu.                  */
        HWND   hwndBlockMenu;  /* Blocktext popupmenu.                 */
        HWND   hwndMetaMenu;   /* Metafile object popupmenu            */
        SIZEF  sizfx;          /* Font box size                        */
        ULONG  ulgridcx;       /* gridsize in PS units.                */
        ULONG  ulgridcy;       /* gridsize in PS units.                */
        ULONG  ulgriddisp;     /* display interval of grid.            */
        BOOL   bSelAll;        /* Can we select objects on other layers?*/
        USHORT paper;          /* show paper coords or win coords (Y/N)*/
        char   fontname[40];   /* fontname is shown in statusline.     */
        GRADIENT Gradient;     /* Contains the gradient settings. Which*/
                               /* can be changed in dlg_clr.c          */
        FOUNTAIN fountain;     /* fountain fill (see drwfount.c)       */
        char   szIniFile[256]; /* Inifile <bootdrive>:\OS2\DRAWIT.INI  */
        char   szCurrentDir[256]; /* Used in dragdrop implementation.  */
        char   szFilename[256];
        BOOL   bSuppress;      /* Suppress gradient fill for quick refr*/
        BOOL   bCompat;        /* Some VGA cards are giving problems.  */
        POBJECT pvCurrent;     /* Current selected object.             */
        BOOL   bFileHasChanged;/* Checks if the file/drawing has changed*/
        LONG   lFileFormat;    /* FILE_DRAWIT28 | FILE_DRAWIT24 etc    */
        ULONG  ulUnits;        /* Used units. exporting img PU_PELS    */
        MATRIXLF matOrg;       /* Used to reset the model transform    */
        LONG   lRound;         /* Box rounding                         */
        ULONG  fxPointsize;
        int    op_mode;
        LONG   lcid;           /* Font id.                             */
        drw_palette colorPalette;
        BOOL   bDraftPrinting;
        BOOL   bOnArea;        /* Selection mode. See drwcreat.c       */
        ARROW  arrow;          /* Arrows on single lines.              */
        BOOL   bPrinter;       /* Printer context TRUE | FALSE         */
        BOOL   bDBCS;          /* Running on a double byte system?     */
        SHADE  Shade;          /* Shading                              */
        LONG   lFigure;        /* Signed! Figure choosen from fig menu */
        BOOL   bGrid;          /* Drawing on a grid?  TRUE | FALSE     */
		BOOL   bActiveLayer;
} WINDOWINFO;
/*
** Actions on the windowinfo
*/
#define DRW_SETCOLOR        100
#define DRW_SETITEM         105
#define DRW_GETCOLOR        110
#define DRW_GETITEM         115
#define DRW_UPDATEVALSET    120
/*
** File info structure used for the various filedialog calls. 
** See drwimg.c and drwmeta.c ....
*/
typedef struct _LOADINFO   /* li */
{
   HFILE   hf;
   FILE    *fp;           /* Not used?*/
   int     handle;
   CHAR    szFileName[CCHMAXPATH];
   CHAR    szdefdir[CCHMAXPATH];
   CHAR    szExtension[8];
   ULONG   dlgflags;      /* standard file dialog behaviour */
} Loadinfo, *pLoadinfo;   /* pli */

extern MRESULT EXPENTRY ClrWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
extern MRESULT EXPENTRY EditClrWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
extern void ErrorBox(char *Str1, char *Str2);          /*Dialog.c*/
extern short  TextAngle;
extern void GetErrorInfo(HAB hab, PSZ pszString);
extern ULONG DrwWinValue(USHORT request, ULONG value);
MRESULT KeybEsc(void);
void   showFontInStatusLine(SIZEF,char *);
/*
** drwcreat.c
*/
MRESULT DrawCreate(HWND hwnd, WINDOWINFO *pwi);
void    iniSaveSettings(WINDOWINFO *pwi);
BOOL    createIni( WINDOWINFO *pwi );

/* dlg_val.c*/
void RebuildValueSet(void);
extern MRESULT EXPENTRY ColorValProc(HWND,ULONG,MPARAM,MPARAM);
extern HWND CreateColorValueSet(HWND);
extern void SetClrValueSetItem(ULONG,ULONG);
#ifdef __IBMC__
#define itoa _itoa
#endif

/*
** drwlayer.c
*/
void LayerConnect(WINDOWINFO *pw, USHORT option);
MRESULT EXPENTRY LayerDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY LayerWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
void setDraftPreview( BOOL bDraft);
MRESULT layerDetail( HWND hOwner, WINDOWINFO *pwi);
/*-------------Sinus & Cosinus table defined in dlg_sqr.c ----------------*/
/* angle = 0.01745329 * degrees.                                          */
/*------------------------------------------------------------------------*/
extern float COSTAB[];
extern float SINTAB[];
/*
** Debugging shit
*/
extern  VOID  DispErrorMsg(HAB hab, HWND hwndFrame, PCH FileName, LONG LineNum);

#define DispError(a,b)   DispErrorMsg(a,b,(PCH)__FILE__,(LONG)__LINE__)
/*
** USER MESSAGES USED IN THE MAIN WINDOWPROCEDURE TO UPDATE
** THE STATUSLINE.
*/
#define UM_SELECTTXT       WM_USER + 11
#define UM_SIZEHASCHANGED  WM_USER + 13 /* Selected /Created Object size has changed  */
#define UM_POSHASCHANGED   WM_USER + 14 /* Mouse pointer position has changed         */
#define UM_FNTHASCHANGED   WM_USER + 15 /* Fontname in status line.                   */
#define UM_LAYERHASCHANGED WM_USER + 16 /* layer in status line.                      */
#define UM_OBJCOLORCHANGE  WM_USER + 17 /* send when someone click on the valset      */
#define UM_OBJPATTERNCHG   WM_USER + 18 /* send when someone click on the pattern menu*/
#define UM_OBJINVAREA      WM_USER + 19 /* posted when the objarea should be invalidated  */
/*
** User message posted to the worker thread.
*/
#define UM_EXIT          WM_USER+20   /* exit from drawing thread.           */
#define UM_PRINT         WM_USER+21   /* printing!!                          */
#define UM_PAINT         WM_USER+22   /* posted paint message to 2th thread. */

#define UM_BUSINESS      WM_USER+23 /* Message for business box in status line!*/
#define UM_ENDDIALOG     WM_USER+24 /* posted at every dismissdlg.         */
#define UM_LNTYPECHANGE  WM_USER+25 /*The linetype is changed tell selected obj*/
#define UM_LNWIDTHCHANGE WM_USER+26 /*The linewidth is changed tell sel object*/
#define UM_CLRHASCHANGED WM_USER+27 /* SHOW in status line the color setting!*/
#define UM_LNJOINCHANGE  WM_USER+28 /*The linejoin is changed */
#define UM_ROTATING      WM_USER+29 /*Object rotation show degrees in statusline*/
#define UM_FORMSIZE      WM_USER+30 /*Show formsize when clicked on it */
#define UM_IMGLOADED     WM_USER+31 /*Tell that the image is loaded.   */
#define UM_JPGERROR      WM_USER+32 /* JPEG LIBRARY USES THIS !!!!!    */
#define UM_FORECOLORCHANGED WM_USER + 33
#define UM_BACKCOLORCHANGED WM_USER + 34
#define UM_READWRITE     WM_USER + 35 /* Show user that program is loading image or printing*/
#define UM_LOADMETAFILE  WM_USER + 36
#define UM_LOADBITMAPFILE WM_USER + 37
#define UM_ANGLEHASCHANGED WM_USER + 38
#define UM_PATCHED         WM_USER + 39 /* Patched program, see drwprn!*/
#define UM_LOADWMFFILE     WM_USER + 40 /* Handle dropped WMF file     */
#define UM_WMFERROR        WM_USER + 41 /* Handle error in WMF file    */
#define UM_JSPERROR        WM_USER + 42 /* Report load error on JSPFILE*/
