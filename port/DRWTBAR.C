/*---------------------------------------------------------------------------*/
/*  Name: drwtbar.c                                                          */
/*                                                                           */
/*  Description : Toolbar                                                    */
/*                                                                           */
/*                                                                           */
/*-sc---date-----------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include "dlg_btn.h"
#include "drwtbar.h"
#include "resource.h"  /* resource id's            */

#define MAXBUTTONS    15    /* Number of buttons in main bar.              */
#define CIRCBUTTONS   15    /* Offset in array where the circle btns start */
#define NR_CIRBTNS     9    /* Number of circle buttons.                   */
#define TEXTBUTTONS   25    /* Here start the text buttons*/

#define TBAR_SPACE    5
static char pszButtonClass[] = "ImageBtn";
static HWND hTbar;             /* Main toolbar window handle           */
static HWND hTbarCircle;       /* Group of buttons for circular shapes */
static HWND hTbarText;         /* Group of buttons for text & block txt*/
/*
**---------------------------- Main toolbar-------------------------------
*/
struct                      /* ibtn */
{                           /* Size:  16        Bytes                     */
   LONG        cb;          /* Structure        Size                      */
   LONG        idBitmap;    /* Bitmap ID        : Normal or Up Position   */
   LONG        idBitmapDown;/* Bitmap ID        : Down Position           */
   LONG        idBitmapDisabled; /* Bitmap ID        : Disabled           */
   LONG        idButton;    /* WINDOW ID used in the WM_COMMAND.          */
   HWND        hButton;     /* WINDOW handle of the Button.               */
} imgbtn[]=
{
{16,IDB_SELECT,    IDB_SELECT,  IDB_SELECT,  IDBTN_SELECT,  NULLHANDLE}, /* SELECT    */
{16,IDB_TEXT,      IDB_TEXT,    IDB_TEXT,    IDBTN_TEXT,    NULLHANDLE}, /* TEXT      */
{16,IDB_SQUARE,    IDB_SQUARE,  IDB_SQUARE,  IDBTN_SQUARE,  NULLHANDLE}, /* SQUARE    */
{16,IDB_CIRCLE,    IDB_CIRCLE,  IDB_CIRCLE,  IDBTN_TBCIRCLE,NULLHANDLE}, /* CIRCLE BAR*/
{16,IDB_LINE  ,    IDB_LINE,    IDB_LINE,    IDBTN_LINE,    NULLHANDLE}, /* LINE      */
{16,IDB_FLINE ,    IDB_FLINE,   IDB_FLINE,   IDBTN_FLINE,   NULLHANDLE}, /* POLYFILLET*/
{16,IDB_TRIANGLE,  IDB_TRIANGLE,IDB_TRIANGLE,IDBTN_TRIANGLE,NULLHANDLE}, /* TRIANGLE  */
{16,IDB_SPLINE,    IDB_SPLINE,  IDB_SPLINE,  IDBTN_SPLINE,  NULLHANDLE}, /* POLYLINE  */
{16,IDB_FILL  ,    IDB_FILL,    IDB_FILL,    IDBTN_FILL,    NULLHANDLE}, /* COLORFILL */
{16,IDB_FONT  ,    IDB_FONT,    IDB_FONT,    IDBTN_FONT,    NULLHANDLE}, /* FONT PAL  */
{16,IDB_LAYER ,    IDB_LAYER,   IDB_LAYER,   IDBTN_LAYER,   NULLHANDLE}, /* LAYER     */
{16,IDB_FCHANGE,   IDB_FCHANGE, IDB_FCHANGE, IDBTN_FCHANGE, NULLHANDLE}, /* POINT EDIT*/
{16,IDB_CHGOBJ,    IDB_CHGOBJ,  IDB_CHGOBJ,  IDBTN_CHGOBJ,  NULLHANDLE}, /* BRING TO TOP */
{16,IDB_ZOOM,      IDB_ZOOM,    IDB_ZOOM,    IDBTN_ZOOM,    NULLHANDLE}, /* ZOOM      */
{16,IDB_ROTATE,    IDB_ROTATE,  IDB_ROTATE,  IDBTN_ROTATE,  NULLHANDLE}, /* ROTATE    */
/*
**----------------------------Toolbar for circles-----------------------------
*/
{16,IDB_CIRCLE,    IDB_CIRCLE,  IDB_PCIRCLE,  IDBTN_CIRCLE,  NULLHANDLE}, /* CIRCLE    */
{16,IDB_CLTOP,     IDB_CLTOP,   IDB_CLTOP,    IDBTN_CLTOP,   NULLHANDLE},
{16,IDB_CLBOT,     IDB_CLBOT,   IDB_CLBOT,    IDBTN_CLBOT,   NULLHANDLE},
{16,IDB_CRTOP,     IDB_CRTOP,   IDB_CRTOP,    IDBTN_CRTOP,   NULLHANDLE},
{16,IDB_CRBOT,     IDB_CRBOT,   IDB_CRBOT,    IDBTN_CRBOT,   NULLHANDLE},
{16,IDB_CLFTOP,    IDB_CLFTOP,  IDB_CLFTOP,   IDBTN_CLFTOP,  NULLHANDLE},
{16,IDB_CLFBOT,    IDB_CLFBOT,  IDB_CLFBOT,   IDBTN_CLFBOT,  NULLHANDLE},
{16,IDB_CRFTOP,    IDB_CRFTOP,  IDB_CRFTOP,   IDBTN_CRFTOP,  NULLHANDLE},
{16,IDB_CRFBOT,    IDB_CRFBOT,  IDB_CRFBOT,   IDBTN_CRFBOT,  NULLHANDLE},
{0,0,0,0,0,NULLHANDLE},
/*
**----------------------------Toolbar for text and blocktext------------------
*/
{16,IDB_TEXT,      IDB_TEXT,    IDB_TEXT,    IDBTN_SNTEXT,    NULLHANDLE}, /* SINGLETEXT*/
{16,IDB_BLCKTEXT,  IDB_BLCKTEXT,IDB_BLCKTEXT,IDBTN_BLCKTEXT,  NULLHANDLE}, /* BLOCKTEXT */
{0,0,0,0,0,NULLHANDLE},
};
/*---------------------------------------------------------------------------*/
/* createToolBar.                                                            */
/*---------------------------------------------------------------------------*/
BOOL createToolBar(HWND hOwner, HWND hParent )
{
   int i,index;
   long cyBar = CYBUTTON * MAXBUTTONS;
   long yPos;
   long xPos;
   /*
   ** Create the main toolbar
   */
   hTbar =  WinCreateWindow(hParent,WC_STATIC, "",
                            WS_VISIBLE,
                            0,0,
                            CXBUTTON,
                            cyBar,
                            hOwner,               /* owner window      */
                            HWND_BOTTOM,
                            ID_TBAR,
                            (void *)0,            /* user data         */
                            NULL);                /* press params      */

   if (!hTbar)
      return FALSE;

   yPos = cyBar - CYBUTTON;

   for (i = 0; i < MAXBUTTONS; i++)
   {
      imgbtn[i].hButton = WinCreateWindow(hTbar,pszButtonClass,"",
                                          WS_VISIBLE,
                                          0,
                                          yPos,
                                          CXBUTTON,
                                          CYBUTTON,
                                          hOwner,
                                          HWND_BOTTOM,
                                          imgbtn[i].idButton,
                                          (void *)&imgbtn[i],
                                          NULL); /* press params */
      yPos -= CYBUTTON;
   }
   /*
   ** Create the invisible toolbar window for the circle shapes.
   ** Make it big enough to contain 9 buttons.
   */
   hTbarCircle =  WinCreateWindow(HWND_DESKTOP,WC_STATIC, "",
                            0,
                            CXBUTTON,0,
                            CXBUTTON * 3,
                            CYBUTTON * 3,
                            hOwner,               /* owner window      */
                            HWND_BOTTOM,
                            ID_CIRTBAR,
                            (void *)0,            /* user data         */
                            NULL);                /* press params      */

   xPos = 0;
   yPos = 0;
   for (index = CIRCBUTTONS, i = 0; i < NR_CIRBTNS && imgbtn[index].idBitmap;
        i++,index++, xPos += CXBUTTON)
   {
      if (i && !(i % 3))
      {
         xPos  = 0;
         yPos += CYBUTTON;
      }
      imgbtn[index].hButton = WinCreateWindow(hTbarCircle,
                                              pszButtonClass,"",WS_VISIBLE,
                                              xPos,yPos,
                                              CXBUTTON,
                                              CYBUTTON,
                                              hOwner,    /* owner window */
                                              HWND_BOTTOM,
                                              imgbtn[index].idButton,
                                              (void *)&imgbtn[index],                      /* user data    */
                                              NULL);                                      /* press params */
   }
   /*
   ** Create the invisible toolbar window for the text buttons.
   ** Make it big enough to contain 2 buttons.
   */
   hTbarText   =  WinCreateWindow(HWND_DESKTOP,WC_STATIC, "",
                            0,            /* Invisible */
                            CXBUTTON,0,
                            CXBUTTON * 2, /* Two buttons width */
                            CYBUTTON * 1, /* One button height.*/
                            hOwner,       /* owner window      */
                            HWND_BOTTOM,
                            ID_TEXTTBAR,
                            (void *)0,    /* user data         */
                            NULL);        /* press params      */

   xPos = 0;
   yPos = 0;

   for (index = TEXTBUTTONS, i = 0; i < 2 && imgbtn[index].idBitmap;
        i++,index++, xPos += CXBUTTON)
   {
      imgbtn[index].hButton = WinCreateWindow(hTbarText,
                                              pszButtonClass,"",WS_VISIBLE,
                                              xPos,yPos,
                                              CXBUTTON,
                                              CYBUTTON,
                                              hOwner,    /* owner window */
                                              HWND_BOTTOM,
                                              imgbtn[index].idButton,
                                              (void *)&imgbtn[index],                      /* user data    */
                                              NULL);                                      /* press params */
   }
   return (BOOL)(hTbar != NULLHANDLE);
}
/*---------------------------------------------------------------------------*/
/* destroyToolBar                                                            */
/*---------------------------------------------------------------------------*/
BOOL destroyToolBar(void)
{
   int i;

   for (i = 0; imgbtn[i].idBitmap; i++)
   {
      if (imgbtn[i].hButton)
         WinDestroyWindow(imgbtn[i].hButton);
   }
   WinDestroyWindow(hTbar);
   WinDestroyWindow(hTbarCircle);
   WinDestroyWindow(hTbarText);
   return TRUE;
}
/*-----------------------------------------------[ private ]-----------------*/
/* showCirTBar   - Query the possible position for the circle toolbar.       */
/*                                                                           */
/* Description  : When the user selects the circle button, which brings up   */
/*                toolbar for circle, this function is used to set the pos   */
/*                correct. And shows the toolbar?                            */
/*---------------------------------------------------------------------------*/
void showSubTBar(HWND hButtonParent, ULONG idButtonBar)
{
   SWP swp,swpTbar;
   POINTL ptl;
   HWND hT; /* Button which activates sub toolbar */
   HWND hClient;

   hClient = WinQueryWindow(hTbar,QW_PARENT);

   if (hButtonParent && hClient)
   {
      /*
      ** I tried to map the points of IBTN_TBCRICLE to the
      ** HWND_DESKTOP, but WinMapWindowPoints fails..
      ** So here I use hT.
      ** Get in hT the parent of the buttons. Parent of circle buttons
      ** What is the position of the window.
      ** Get the position of the main buttonbar.
      */
      hT = WinWindowFromID(hTbar,idButtonBar); 
      WinQueryWindowPos(hT,&swp);
      WinQueryWindowPos(hTbar,&swpTbar);
      ptl.x = swp.x + swpTbar.x;
      ptl.y = swp.y + swpTbar.y;
      ptl.x += (CXBUTTON + TBAR_SPACE);
      WinMapWindowPoints(hClient,HWND_DESKTOP,&ptl,1);
      WinSetWindowPos(hButtonParent,HWND_TOP,ptl.x,ptl.y,0,0,
                      SWP_MOVE | SWP_ZORDER | SWP_SHOW);
   }
}
/*---------------------------------------------------------------------------*/
/* posToolBar                                                                */
/*---------------------------------------------------------------------------*/
void posToolBar(long x, long y)
{
   if (hTbar)
      WinSetWindowPos(hTbar,HWND_TOP,x,y,0,0,SWP_MOVE | SWP_ZORDER);
}
/*---------------------------------------------------------------------------*/
/* getToolBarHeight                                                          */
/*---------------------------------------------------------------------------*/
long getToolBarHeight( void )
{
 long cyBar = CYBUTTON * MAXBUTTONS;
 return cyBar;
}
#if 0
/*-----------------------------------------------[ private ]-----------------*/
/* enableButtons..                                                           */
/*---------------------------------------------------------------------------*/
static void enableButtons( void )
{
   int i;

   for (i = 0; i < MAXBUTTONS; i++)
      WinEnableWindow(imgbtn[i].hButton,TRUE);
}
#endif
/*---------------------------------------------------------------------------*/
/* commandToolBar.                                                           */
/*---------------------------------------------------------------------------*/
void commandToolBar(long lCommand)
{
   WinShowWindow(hTbarCircle,FALSE);
   WinShowWindow(hTbarText,FALSE);
   if (lCommand == IDBTN_TBCIRCLE )
         showSubTBar(hTbarCircle,IDBTN_TBCIRCLE); 
   else if (lCommand == IDBTN_TEXT)
         showSubTBar(hTbarText,IDBTN_TEXT); 
}
/*---------------------------------------------------------------------------*/
/* registerToolBar                                                           */
/*---------------------------------------------------------------------------*/
BOOL registerToolBar(HAB hab)
{
  return WinRegisterClass(hab,(PSZ)pszButtonClass,
                          (PFNWP)ImageBtnWndProc,
                          CS_PARENTCLIP |
                          CS_SYNCPAINT  |
                          CS_SIZEREDRAW,
                          (ULONG)USER_RESERVED);
}

