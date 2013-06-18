/*---------------------------------------------------------------------------*/
/*  Name: drawit.c       contains    tempcode     search for 'tempcode'      */
/*                                                                           */
/*  Description : Main module of the drawit program!                         */
/*                                                                           */
/*                                                                           */
/*-sc---date-----------------------------------------------------------------*/
/* 1    140597   JdK       Multiselect box should show its size while drawing*/
/* 2    020698   JdK       Removed the partial arc stuff.                    */
/* 3    250898   JdK       Leave selection on after attribute change.        */
/* 4    231298   JdK       No dragbox needed for multimove operation.        */
/*---------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#define INCL_DOSPROCESS      /* Process and thread values     */
#define INCL_DOSSEMAPHORES   /* Semaphore values              */
#define INCL_DOSFILEMGR
#define SELECT        103
#define MAKE_SQUARE   104
#define DRAWMODE      106
#define CYSTATUSLINE  28
#define CBARHEIGHT    20 /* colorbar height */

#define ABOVECENTER    6
#define ID_TIMER       1    /*Timer ID used for freestyle lines*/

#define DB_RAISED    0x0400	/* the undocumented value's */
#define DB_DEPRESSED 0x0800     /* drawbox.                 */

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_sqr.h"
#include "drwprn.h"
#include "dlg_sel.h"
#include "dlg_cir.h"
#include "dlg_file.h"
#include "dlg_img.h"
#include "dlg_txt.h"
#include "dlg_fnt.h"
#include "dlg_hlp.h"
#include "dlg_lin.h"
#include "dlg_btn.h"
#include "dlg_clr.h"
#include "drwtrian.h"  /* triangle stuff           */
#include "drwsplin.h"  /* PolyLines and splines    */
#include "drwmenu.h"   /* Menu handling stuff      */
#include "drwutl.h"
#include "drwtxtin.h"  /*TextInput                 */
#include "drwmeta.h"   /*Metafile support          */
#include "drwcanv.h"
#include "drwform.h"
#include "drwpic.h"    /* Lotus 123 picture loader */
#include "drwfount.h"  /* fountain fill prototypes */
#include "drwgrp.h"
#include "drwcbar.h"
#include "drwtbar.h"   /* Toolbar ..               */
#include "drwarr.h"    /* Arrows or lineends       */
#include "resource.h"  /* resource id's            */
#include "drwbtext.h"  /* func createblocktext.    */
#include "drwwmf.h"
static BOOL   fHelpEnabled=TRUE;

void PolyDump(POBJECT pObj, WINDOWINFO *pwi); /* tempcode */
POBJECT splArrow(WINDOWINFO *wi, POINTL *ptlMouse, long lFigure);

                                    /* interior?                      */
static BOOL   button1down  =FALSE;  /* For moving a selected object   */
static BOOL   linestretch  = FALSE;
static BOOL   objectstretch= FALSE;
static WINDOWINFO wi;                /* clientwindow size     */
static  GRADIENTL agradl = {100,0};  /* 0 degrees for reset   */

POBJECT pObj;
POBJECT pCreate;

MRESULT EXPENTRY DrawWndProc  (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY CanvasWndproc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY MainWndProc  (HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY AboutDlgProc (HWND,ULONG,MPARAM,MPARAM);
VOID    EXPENTRY PaintThreadProc(LONG arg);
BOOL    Drawing(void);

extern void Splash(HAB hab,HWND hwnd);

HWND hwndFrame, hwndClient,hwndMFrame,hwndMain;
HWND hwndFcanv, hwndCcanv;
HWND HwndMenu;           /* Menubar windowhandle */

/* Key global variables */

HAB    hab2;       /* Anchor block for the main thread             */
HAB    hab;        /* Anchor block for the second thread           */
HMQ    hmq;        /* Message queue handle for the main thread     */
HMQ    hmq2;       /* Message queue handle for the second thread   */
TID    tid;        /* Thread id of the second thread               */
HEV    hevQEvent;  /* Event semaphore, well see the code.          */

HPOINTER hptrCrop;       /* Image crop pointer.                       */
HPOINTER hptrCircle;     /* Used when drawing cicles and ellipses.    */
HPOINTER hptrSquare;     /* Used when drawing squares.                */
HPOINTER hptrPline;      /* Used when drawing polylines.              */
HPOINTER hptrParc;       /* Used when drawing partial arcs.           */
HPOINTER hptrText;       /* Text pointer.                             */

HWND hwndColor;          /* Color ValueSet                            */
POINTL  ptlStart,ptlEnd; /* defines the outline of drawn select box   */


static pCircle pCir;     /* used for selection and filling of circles */
static pLines  pLin;     /* used for selection                        */
static char pszButtonClass[] = "ImageBtn";
static char szDrawClass[]    = "DrawWindow";
static char szMainClass[]    = "MainWindow";
/*------------------------------------------------------------------------*/
/* Handle the ESC key. Reset the application back to its default select   */
/* state by resetting the globals.                                        */
/*------------------------------------------------------------------------*/
MRESULT KeybEsc(void)
{
   RECTL rcl;

   commandToolBar((long)IDBTN_SELECT); /* Make the select button white */

   if (wi.op_mode == MULTISELECT)
   {
      ObjBoundingRect(&wi,&rcl,TRUE);
      WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
   }
   else if (wi.op_mode == SPLINEDRAW  || wi.op_mode == FREESTYLE)
   {
      CloseSpline(&wi);
      changeMode(&wi,NOSELECT,0L);
   }
   else if (wi.op_mode == OBJFORMCHANGE)
   {
      changeMode(&wi,NOSELECT,0L);
      ObjRefresh(wi.pvCurrent,&wi);
   }
   else if (wi.op_mode == OBJROTATION)
   {
      StartupSnapRotDlg(&wi,FALSE); /*dismissDlg..*/
      ObjRefresh(wi.pvCurrent,&wi);
      ObjRefreshRotCenter(&wi,wi.pvCurrent);
   }
   /* unselect all selected elements*/
   changeMode(&wi,NOSELECT,0L);
   return (MRESULT)0;
}

/*------------------------------------------------------------------------*/
/* name:        showProperties.                                          -*/
/*------------------------------------------------------------------------*/
MRESULT showProperties(HWND hwnd, POBJECT pObj)
{
    if (!pObj)
       return (MRESULT)0;

    wi.pvCurrent = pObj;
    switch (pObj->usClass)
    {
       case CLS_CIR:
          circleDetail(hwnd,&wi);
          break;
       case CLS_BLOCKTEXT:
          WinLoadDlg(HWND_DESKTOP,hwnd,(PFNWP)ObjectDlgProc,(HMODULE)0,
                     850,(PVOID)&wi);
          break;
       case CLS_GROUP:
          if (canPackGroup(wi.pvCurrent))
             WinLoadDlg(HWND_DESKTOP,hwnd,(PFNWP)ObjectDlgProc,(HMODULE)0,
                                     850,(PVOID)&wi);
          break;
       case CLS_TXT:
          textDetail(hwnd,&wi);
          break;
       case CLS_IMG:
          ImgShowPalette(hwnd,(pImage)pObj);
          break;
       case CLS_SPLINE:
          wi.pvCurrent = (POBJECT)pObj;
          WinLoadDlg(HWND_DESKTOP,hwnd,
                     (PFNWP)PolyLineDlgProc,
                     (HMODULE)0,
                     ID_SPLINE,
                     (PVOID)&wi);
          break;
       case CLS_META:
          MetaDetail(hwnd,pObj);
          break;

    }
    return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/* name:        handleCursorKeys                                         -*/
/*------------------------------------------------------------------------*/
MRESULT handleCursorKeys(WINDOWINFO *pwi, USHORT usKey)
{
   LONG ldx,ldy;

   switch (usKey)
   {
   case VK_LEFT:
      if (pwi->bGrid)
      {
         ldx = (LONG)pwi->ulgridcx;
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,-ldx,0);
         else
            MoveObject(pwi->pvCurrent,-ldx,0,pwi);
      }
      else
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,-1,0);
         else
            MoveObject(pwi->pvCurrent,-1,0,pwi);
      }
      return (MRESULT)1;

   case VK_RIGHT:
      if (pwi->bGrid)
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,(LONG)pwi->ulgridcx,0);
         else
            MoveObject(pwi->pvCurrent,(LONG)pwi->ulgridcx,0,pwi);
      }
      else
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,1,0);
         else
            MoveObject(pwi->pvCurrent,1,0,pwi);
      }
      return (MRESULT)1;

   case VK_UP:
      if (pwi->bGrid)
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,0,(LONG)pwi->ulgridcy);
         else
            MoveObject(pwi->pvCurrent,0,(LONG)pwi->ulgridcy,pwi);
      }
      else
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,0,1);
         else
            MoveObject(pwi->pvCurrent,0,1,pwi);
      }
      return (MRESULT)1;

   case VK_DOWN:
      if (pwi->bGrid)
      {
         ldy = (LONG)pwi->ulgridcy;

         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,0,-ldy);
         else
            MoveObject(pwi->pvCurrent,0,-ldy,pwi);
      }
      else
      {
         if (pwi->op_mode == OBJFORMCHANGE)
            ObjMoveHandle(pwi->pvCurrent,pwi,0,-1);
         else
            MoveObject(pwi->pvCurrent,0,-1,pwi);
      }
      return (MRESULT)1;
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/* Set the mouse pointer according to the operation.                      */
/*------------------------------------------------------------------------*/
static void drwSetPointer(USHORT operation)
{
   if (!wi.bCompat)
   {
      /*
      ** Most VGA drivers can show fancy pointers
      ** So then we are in this part.
      */
      switch (operation)
      {
         case SPLINEDRAW:
            WinSetPointer(HWND_DESKTOP,hptrPline);
            break;
         case SQUAREDRAW:      /* SQUARE DRAWING MODE       */
            WinSetPointer(HWND_DESKTOP,hptrSquare);
            break;
         case CIRCLEDRAW:      /* CIRCLE DRAWING MODE       */
            WinSetPointer(HWND_DESKTOP,hptrCircle);
            break;
         case TEXTINPUT:
            WinSetPointer(HWND_DESKTOP,hptrText);
            break;
         case LINEDRAW:
         case FREELINEDRAW:
         case FREESTYLE:
         case SPECLINEDRAW:
         case REGPOLYDRAW:
         case REGPOLYSTAR:
         case IMGCROP:
         case FINDCOLOR:
         case INSERTPICTURE:
            WinSetPointer(HWND_DESKTOP,hptrCrop);
            break;
         default:
            WinSetPointer(HWND_DESKTOP,WinQuerySysPointer(HWND_DESKTOP,
                          SPTR_ARROW,FALSE));
            break;
      }
   }
   else
   {
      /*
      ** Trident and Cirrus logic driver cannot show fancy pointers
      ** when in 16 colormode. So just show a simple cross.
      */
      if (operation != NOSELECT)
         WinSetPointer(HWND_DESKTOP,hptrCrop);
   }
}
/*-------------------------------main----------------------------------*/
int main(int argc, char *argv[])
{
   static ULONG flMainFlags = FCF_TITLEBAR      | FCF_SYSMENU |
                              FCF_SIZEBORDER    | FCF_MINMAX  |
                                FCF_SHELLPOSITION | FCF_TASKLIST|
                              FCF_MENU          | FCF_ICON    |
                              FCF_ACCELTABLE;

   static ULONG flFrameFlags = FCF_BORDER |
                               FCF_HORZSCROLL    | FCF_VERTSCROLL;

   QMSG qmsg;
   HOBJECT hoColorPal;     /* Default colorpallette                 */
   HOBJECT hoFontPal;

   ULONG ulTimeout;  /* Number of milliseconds to wait */

   DosCreateEventSem((PSZ)"\\sem32\\qevent",      /* Named-shared semaphore */
                     &hevQEvent,(ULONG)0,FALSE);  /* Initially reset        */

   /*
   ** Create our worker thread. Once created the thread will sit waiting
   ** for messages to be posted to it. Create the second thread before
   ** we create our windows so that it is ready to start servicing paint
   ** requests when the windows are created.
   */

   DosCreateThread( &tid,
                    (PFNTHREAD)PaintThreadProc,
                    0L,
                    0L,
                    28192L);
   /*
   ** Wait for the other thread until it is running
   */
   ulTimeout = 30000;  /* Wait for a maximum of 1/2 minute */

   DosWaitEventSem(hevQEvent, ulTimeout);

   /*
   ** Say bye bye to event semaphore...
   */
   DosCloseEventSem(hevQEvent);
   hab2 = WinInitialize (0);
   hmq = WinCreateMsgQueue(hab2,0);

   ObjNotify();
   RegCanv(hab2,&wi);

   WinRegisterClass(hab2,
                    (PSZ)pszButtonClass,
                    (PFNWP)ImageBtnWndProc,
                    CS_PARENTCLIP |
                    CS_SYNCPAINT  |
                    CS_SIZEREDRAW,
                    (ULONG)USER_RESERVED);

   WinRegisterClass (hab2,(PSZ)szDrawClass,(PFNWP)DrawWndProc,0L,0L);

   WinRegisterClass (hab2,(PSZ)szMainClass,(PFNWP)MainWndProc,0L,0L);

   RegisterColorBar(hab2);   /* see drwcbar.c */
   RegisterTxt(hab2);        /* see dlg_txt.c */
   RegisterFountain(hab2);   /* see drwfount.c*/
   registerToolBar(hab2);    /* see drwtbar.c */
   registerCircle(hab2);     /* see dlg_cir.c */
   RegisterArrow(hab2);      /* see dlg_lin.c */


   WinRegisterClass(hab2,                    // Another block handle
                    (PSZ)"GRADIENTCLASS",    // Name of class being registered
                    (PFNWP)GradientWndProc,  // Window procedure for class
                    CS_SIZEREDRAW,           // Class style
                    0L);                     // Extra bytes to reserve
   /*
   ** See drwlayer.c
   */
   WinRegisterClass(hab2,                    // Another block handle
                    (PSZ)"LayerClass",       // Name of class being registered
                    (PFNWP)LayerWndProc,     // Window procedure for class
                    CS_SIZEREDRAW,           // Class style
                    0L);                     // Extra bytes to reserve

   hwndMFrame = WinCreateStdWindow (HWND_DESKTOP,WS_CLIPCHILDREN,
                                    &flMainFlags,(PSZ)szMainClass,
                                    (PSZ)"DrawIt",0L,
                                    (HMODULE)0,ID_RESOURCE,
                                    &wi.hwndMain);

   hwndFcanv = CreateCanvas(wi.hwndMain, &hwndCcanv);

   flFrameFlags = FCF_BORDER;

   hwndFrame = WinCreateStdWindow (hwndCcanv,0,&flFrameFlags,
                                   (PSZ)szDrawClass,NULL,0L,(HMODULE)0,0L,
                                   &wi.hwndClient);
   fHelpEnabled = InitHelp(hwndMFrame);

   if (!getenv("NOLOGO") && !argv[1])
      Splash(hab2,wi.hwndClient);

   if (getenv("NOPOINTER"))
      wi.bCompat = TRUE;
   else
      wi.bCompat = FALSE;

   CreateBitmapHdcHps( &hdcBitmapFile,&wi.hpsMem);

   WinSetWindowPos(hwndMFrame,(HWND)0,
                   0,0,
                   0,0,
                   SWP_SHOW | SWP_MAXIMIZE);

   /*Load the pointer which shows you can move the object etc*/

   hptrCrop    = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_CROP);
   hptrCircle  = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_CIRCLE);
   hptrSquare  = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_SQUARE);
   hptrPline   = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_POLYLINE);
   hptrParc    = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_PARTARC);
   hptrText    = WinLoadPointer(HWND_DESKTOP,(HMODULE)0,IDP_TEXT);

   if (argv[1])
   {
      char *p;
      char *pszFilename;
      /*
      ** If no extension given: assume JSP file to be loaded
      */
      if ( !(p = strchr(argv[1],'.')) )
         ReadDrwfile(argv[1],&wi);
      else
      {
        pszFilename = strupr(strdup(argv[1]));
        p = strchr(pszFilename,'.');
        p++; /* jump over dot */
        if (*p == 'J' && *(p+1) == 'S'&& *(p+2) == 'P')
           ReadDrwfile(argv[1],&wi);
        else if (*p == 'M' && *(p+1) == 'E'&& *(p+2) == 'T')
        {
           wi.bFileHasChanged = TRUE;
           WinPostMsg(wi.hwndClient,UM_LOADMETAFILE,
                      (MPARAM)pszFilename,(MPARAM)0);
        }
        else
        {
           wi.bFileHasChanged = TRUE;
           WinPostMsg(wi.hwndClient,UM_LOADBITMAPFILE,
                     (MPARAM)pszFilename,(MPARAM)0);
        }


      }

   }

   showForm(&wi,hwndFrame,hwndFcanv);

   UpdateTitleText(hwndMFrame);
   WinSetFocus(HWND_DESKTOP,wi.hwndClient);

   while (TRUE)
   {
      while (WinGetMsg (hab2, &qmsg, 0L,0,0))
         WinDispatchMsg (hab2,&qmsg);

      if ( wi.bFileHasChanged)
      {
         if (FileNew(wi.hwndMain,&wi))
            break;
         WinCancelShutdown(hmq,FALSE);
      }
      else
         break;
   }

   GpiDeleteSetId(wi.hps,2L);
   GpiSetCharSet (wi.hps,LCID_DEFAULT) ;     // Clean up

   hoColorPal = WinQueryObject((PSZ)"<DRWCOLOR>");
   if (hoColorPal)
      WinSetObjectData(hoColorPal,(PSZ)"CLOSE=YES");

   hoFontPal = WinQueryObject((PSZ)"<DRWFONT>");
   if (hoFontPal)
      WinSetObjectData(hoFontPal,(PSZ)"CLOSE=YES");


   iniSaveSettings(&wi);            /* Save settings (drwcreat.c)*/

   ObjDeleteAll();                  /* clear drawing from memory */
   ClosePrinter();
   CloseImgHdcHps(&wi);
   DestroyHelpInstance();
   GpiDestroyPS(wi.hps);
   WinStopTimer(hab,wi.hwndClient,ID_TIMER);
   WinDestroyPointer(hptrCrop);
   WinDestroyPointer(hptrCircle);
   WinDestroyPointer(hptrSquare);
   WinDestroyPointer(hptrPline);
   WinDestroyPointer(hptrParc);
   WinDestroyPointer(hptrText);
   WinDestroyWindow (hwndFrame);
   WinDestroyWindow (wi.hwndImgMenu);   /* Image    popupmenu */
   WinDestroyWindow (wi.hwndOptMenu);   /* Default  popupmenu */
   WinDestroyWindow (wi.hwndTxtMenu);   /* Text     popupmenu */
   WinDestroyWindow (wi.hwndClipMenu);  /* ClipPath popupmenu */
   WinDestroyWindow (wi.hwndBlockMenu); /* Blocktextpopupmenu */
   WinDestroyWindow (wi.hwndMetaMenu);  /* Metafile popupmenu */
   destroyToolBar();
   WinDestroyMsgQueue ( hmq);
   WinTerminate (hab2);
   return 0;
}
//--------------------------------------------------------------------------
//
//  PaintThreadProc() & printing!
//
// --------------------------------------------------------------------------
VOID PaintThreadProc( LONG arg )
{
   QMSG  qmsg2;
   RECTL rcl,rclWnd;
   POINTL ptl;
   TID         ThreadID;       /* New thread ID (returned)                   */
   ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
   ULONG       StackSize;      /* Size in bytes of new thread's stack        */

   hab = WinInitialize( 0UL );

   hmq2 = WinCreateMsgQueue( hab, 0UL );

   DosPostEventSem(hevQEvent);  /* Posts the event*/

   WinCancelShutdown( hmq2, TRUE );

   qmsg2.msg = 0;

   while( qmsg2.msg != UM_EXIT )
   {
      switch( qmsg2.msg )
      {
         case UM_PRINT:
            ObjPreparePrinting(&wi);
            ThreadFlags = 0;        /* Indicate that the thread is to */
                                    /* be started immediately         */
            StackSize = 14096;      /* Set the size for the new       */
                                    /* thread's stack                 */

            DosCreateThread(&ThreadID,
                            (PFNTHREAD)PrintWrite,
                            (ULONG)&wi,
                            ThreadFlags,
                            StackSize);
            break;

         case UM_PAINT:
            GpiSetMix(wi.hps,FM_DEFAULT);
            GpiSetColor(wi.hps,wi.ulColor);
            GpiSetLineType(wi.hps,LINETYPE_SOLID);
            GpiSetPattern(wi.hps, PATSYM_SOLID);

            if (wi.bGrid)
            {
               WinQueryWindowRect(wi.hwndClient, &rclWnd);
               GpiConvert(wi.hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,2,(POINTL *)&rclWnd);
            }
            /*
            ** Het zijn 0.1 mm units...
            */
            rcl.xLeft  = (LONG)SHORT1FROMMP (qmsg2.mp1);
            rcl.xRight = (LONG)SHORT2FROMMP (qmsg2.mp1);
            rcl.yBottom= (LONG)SHORT1FROMMP (qmsg2.mp2);
            rcl.yTop   = (LONG)SHORT2FROMMP (qmsg2.mp2);

            ptl.x = rcl.xLeft;
            ptl.y = rcl.yTop;

            GpiBeginPath(wi.hps, 1L);  /* define a clip path    */
            GpiMove(wi.hps,&ptl);
            ptl.x = rcl.xRight;
            ptl.y = rcl.yBottom;
            GpiBox(wi.hps,DRO_OUTLINE,&ptl,0,0);
            GpiEndPath(wi.hps);
            GpiSetClipPath(wi.hps,1L,SCP_AND);
            if (wi.bGrid)
               DrawGrid(&wi,&rclWnd); /* in 0.1 mm ps... */
            if ( wi.bActiveLayer )
            {
               wi.usdrawlayer = wi.uslayer;
               ObjDrawSegment(wi.hps,&wi,(POBJECT)0,&rcl);
            }
            else
            {
               for ( wi.usdrawlayer = MINLAYER; wi.usdrawlayer <= MAXLAYER; wi.usdrawlayer++)
                  ObjDrawSegment(wi.hps,&wi,(POBJECT)0,&rcl);
            }
            GpiSetClipPath(wi.hps, 0L, SCP_RESET);  /* clear the clip path   */
            GpiSetPattern(wi.hps, PATSYM_SOLID);
            if (wi.pvCurrent && wi.op_mode != MULTISELECT && wi.op_mode != OBJROTATION)
               showSelectHandles((POBJECT)wi.pvCurrent,&wi);
            else if ( wi.op_mode == MULTISELECT )
               ObjMultiPaintHandles(&wi);

            GpiSetMix(wi.hps,FM_DEFAULT);
            GpiSetColor(wi.hps,wi.ulColor);
             GpiSetLineType(wi.hps,LINETYPE_SOLID);
            GpiSetPattern(wi.hps, PATSYM_SOLID);
            GpiSetCharAngle(wi.hps,&agradl) ;     // Reset!!
            setFont(&wi,&wi.fattrs,wi.sizfx);

            if (wi.pvCurrent && wi.op_mode == OBJFORMCHANGE)
               ObjShowMovePoints((POBJECT)wi.pvCurrent,&wi);
            else if (wi.pvCurrent && wi.op_mode == OBJROTATION)
               ObjDrawRotCenter(&wi,(POBJECT)wi.pvCurrent,FALSE);

            WinPostMsg(wi.hwndMain,UM_BUSINESS,
                       MPFROMLONG(IDS_WAIT),(MPARAM)0);

            break;
      }
      WinGetMsg( hab, &qmsg2, 0L, 0L, 0L );
   }

   if ( hmq2 )
   {
      WinDestroyMsgQueue( hmq2 );
   }

   if ( hab )
   {
      WinTerminate( hab );
   }
   DosExit( EXIT_THREAD, 0L );
}
/*------------------------------------------------------------------------*/
static VOID MultiSelectBox(HPS hps, POINTL *pptlStart, POINTL *pptlEnd, USHORT mode)
{
   POINTL ptls,ptle;
   LONG lSaveType;

   ptls.x = pptlStart->x;
   ptls.y = pptlStart->y;

   ptle.x = pptlEnd->x;
   ptle.y = pptlEnd->y;


   if ( mode == SELECT)
   {
      lSaveType = GpiQueryLineType(hps);
      GpiSetLineType(hps, LINETYPE_DOT);
      GpiSetMix(hps,FM_INVERT);
      GpiMove(hps, &ptls);
      GpiBox(hps,DRO_OUTLINE,&ptle,0L,0L);
      GpiSetLineType(hps, lSaveType);
   }
   else if (mode == MAKE_SQUARE)
   {
      GpiSetMix(hps,FM_INVERT);
      GpiSetColor(hps,wi.ulColor);
      GpiSetLineType(wi.hps,wi.lLntype);
      GpiMove(hps, &ptls);
      GpiBox(hps,DRO_OUTLINE,&ptle,0L,0L);
      WinPostMsg(wi.hwndMain,
                 UM_SIZEHASCHANGED,
                 (MPARAM)0,
                 MPFROM2SHORT((USHORT)abs(ptls.x - ptle.x),
                              (USHORT)abs(ptls.y - ptle.y)));
   }
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY DrawWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static POINTL  ptlPosn; /* MOUSE point at button1 & button2 down  & dblclick !! */
   POINTL ptlMove;         /* Mouse position during the mouse move        */
   static HPS     hps;
   static RECTL   rcl;
   static USHORT  corner;               /* on which corner of the image are you?*/
   int    fnd;                          /* presparam */
   CHAR   buffer[100];
   SHORT  sRep;                    /* Keyboard Repeat count + layer count*/
   char   *p; /*pointer in the presparam buffer when fonthandle comes in */


   switch (msg)
   {
     case WM_HELP:
        DisplayHelpPanel(PANEL_HELP);
        return (MRESULT)0;
#ifdef DEMOVERSION
     case UM_PATCHED: /* Someone has patched the executable! */
        WinMessageBox(HWND_DESKTOP,hwnd,
                      (PSZ)"Program exec has been modified\n"\
                      "Thread has been stopped",
                      (PSZ)"ERROR",0,
                      MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                      MB_ICONEXCLAMATION);
        return (MRESULT)0;
#endif
     case UM_LOADBITMAPFILE:
        if (!mp1)
           return (MRESULT)0;
        ImgLoad(&wi,(char *)mp1);
        free((void *)mp1);
        return (MRESULT)0;

     case UM_LOADMETAFILE:
        {
           POINTL ptl;

           ptl.x = 0;
           ptl.y = 0;
           if (!(char *)mp1)
              return (MRESULT)0;

           pCreate = OpenMetaSegment(ptl,(char *)mp1,&wi,(HMF)0);
           if (pCreate)
              ObjRefresh(pCreate,&wi);
           free((void *)mp1);
        }
        return (MRESULT)0;

     case UM_LOADWMFFILE:
        {
           if (!(char *)mp1)
              return (MRESULT)0;
           wmfLoadFile(&wi,(char *)mp1,mp2);
           free((void *)mp1);
        }
        return (MRESULT)0;
        
     case UM_WMFERROR:
        {
           if (!(char *)mp1)
              return (MRESULT)0;
           WinMessageBox(HWND_DESKTOP,hwnd,
                         (PSZ)mp1,
                         (PSZ)"Error",0,
                         MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                         MB_ICONEXCLAMATION);
        }
        return (MRESULT)0;

     case UM_IMGLOADED:
        if (mp1)
           ObjRefresh((POBJECT)mp1,&wi);
        else
        {
           WinMessageBox(HWND_DESKTOP,hwnd,
                         (PSZ)"I don't understand the bitmap format",
                         (PSZ)"Sorry",0,
                         MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                         MB_ICONEXCLAMATION);
        }
        WinPostMsg(wi.hwndMain, UM_READWRITE,(MPARAM)0,(MPARAM)0);
        return (MRESULT)0;

     case UM_JPGERROR:
        WinMessageBox(HWND_DESKTOP,hwnd,
                     (PSZ)mp1,(PSZ)"JPEG_ERROR", 0,
                                       MB_OK | MB_APPLMODAL |
                                       MB_MOVEABLE |
                                       MB_ICONEXCLAMATION);
        free((void *)mp1);
        return (MRESULT)0;

     case UM_JSPERROR:
        WinMessageBox(HWND_DESKTOP,hwnd,
                     (PSZ)mp1,(PSZ)"JSP FILE LOAD ERROR", 0,
                                       MB_OK | MB_APPLMODAL |
                                       MB_MOVEABLE |
                                       MB_ICONEXCLAMATION);
        free((void *)mp1);
        delTitle(hwndMFrame,&wi); /* Cleanup applic titlebar */
        return (MRESULT)0;
 
     case UM_ENDDIALOG:
        /*
        ** A dialog procedure posted us a message of completeness
        ** So invalidate the rect occupied by the object since we
        ** should redraw the object, it is changed!!!
        ** The address is given in the mp1 parameter.
        */
        if (mp1)
        {
           WinInvalidateRect(hwnd,&rcl,TRUE);
           ObjRefresh((POBJECT)mp1,&wi);
        }
        else
           WinInvalidateRect(hwnd,(RECTL *)0,TRUE);
        return 0;
     case WM_BUTTON1DBLCLK:
           ptlPosn.x = MOUSEMSG(&msg)->x;
           ptlPosn.y = MOUSEMSG(&msg)->y;
           GpiConvert(wi.hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlPosn);
           if (!Drawing())
           {
              if (wi.op_mode == TEXTINPUT)
              {
                 GpiSetCurrentPosition(wi.hps,&ptlPosn);
                 WinCreateCursor(hwnd,ptlPosn.x,ptlPosn.y,0,0,CURSOR_SETPOS,NULL);
                 return 0;
              }

              if ((pObj = ObjSelect(ptlPosn,&wi))!=NULL)
              {
                 ObjInvArea(pObj,&rcl,&wi,TRUE);
                 showProperties(hwnd,pObj);
              }
           }
           else           /* YES we are in drawing mode */
           {
              WinAlarm(HWND_DESKTOP, WA_WARNING);
           }
           return 0;

      case WM_BUTTON2DOWN:
           ptlPosn.x = SHORT1FROMMP(mp1);
           ptlPosn.y = SHORT2FROMMP(mp1);
           WinMapWindowPoints(wi.hwndClient,wi.hwndMain,&ptlPosn,1);

           if (wi.hwndClient !=WinQueryFocus(HWND_DESKTOP))
           {
              WinSetFocus(HWND_DESKTOP,wi.hwndMain);
              WinSetFocus(HWND_DESKTOP,hwnd);
           }
           return ShowPopupMenu(&wi,wi.hwndMain,WM_BUTTON2DOWN,ptlPosn);
/*------------------------------------------------------------------------*/
/* First click is getting focus if we don't have it. The next one drawing */
/* etc can be done.                                                       */
/*------------------------------------------------------------------------*/
      case WM_BUTTON1DOWN:
           WinSetFocus(HWND_DESKTOP,hwnd);
           ptlPosn.x = SHORT1FROMMP(mp1);
           ptlPosn.y = SHORT2FROMMP(mp1);


           GpiConvert(wi.hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlPosn);

           ptlStart.x = ptlEnd.x = ptlPosn.x;
           ptlStart.y = ptlEnd.y = ptlPosn.y;

           if (wi.op_mode == FINDCOLOR)
           {
              wi.ulColor = GpiQueryPel(wi.hps,&ptlPosn);


              WinPostMsg(wi.hwndMain,UM_CLRHASCHANGED,(MPARAM)0,(MPARAM)0);
              wi.op_mode = NOSELECT;
              WinSetPointer(HWND_DESKTOP,WinQuerySysPointer(HWND_DESKTOP,
                            SPTR_ARROW,FALSE));
              return setCurBarColor(hwndColor,wi.ulColor);
           }

           if (wi.op_mode ==  OBJFORMCHANGE)
           {
              if (ObjDragHandle(wi.pvCurrent,&wi,ptlPosn,msg,mp2))
              {
                 ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                 button1down=TRUE;
                 WinSetCapture(HWND_DESKTOP,hwnd);
                 return (MRESULT)0;
              }
              else
                 WinAlarm(HWND_DESKTOP, WA_WARNING);
              return (MRESULT)0;
           }

           if (wi.op_mode == OBJROTATION )
           {
              if (corner == ABOVECENTER)
              {
                 button1down=TRUE;
                 WinSetCapture(HWND_DESKTOP,hwnd);
                 wi.op_mode = MOVECENTER;
                 return ObjMoveCenter(&wi,wi.pvCurrent,ptlPosn,msg);
              }

              if (ObjDrawRotLine(&wi,wi.pvCurrent,ptlPosn,msg))
              {
                 button1down=TRUE;
                 WinSetCapture(HWND_DESKTOP,hwnd);
                 if (SHORT2FROMMP(mp2) == KC_CTRL)
                    wi.pvCurrent = ObjectCopy(wi.pvCurrent);
                 ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
              }
              return (MRESULT)0;
           }

           if (wi.bGrid)
           {
              Snap2Grid(&wi,&ptlStart);
              ptlEnd.x =  ptlStart.x;
              ptlEnd.y =  ptlStart.y;
           }

           if (wi.op_mode == TEXTINPUT)
           {
              SetCursorStartPosition(hwnd,&wi,&ptlStart);
              return 0;
           }

           if ( wi.op_mode == INSERTPICTURE )
           {
              splArrow(&wi, &ptlStart, wi.lFigure);
              return KeybEsc();
           }
           button1down = TRUE;
           WinSetCapture(HWND_DESKTOP,hwnd);

           if (wi.op_mode == IMGCROP)
           {
              pObj = ObjSelect(ptlPosn,&wi);

              if (pObj && pObj->usClass == CLS_IMG)
              {
                 wi.pvCurrent = pObj;
                 MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
                 return 0;
              }
              else
              {
                 /* You clicked outside the image */
                 WinAlarm(HWND_DESKTOP, WA_ERROR);
                 WinSetPointer(HWND_DESKTOP,
                               WinQuerySysPointer(HWND_DESKTOP,
                                                  SPTR_ARROW,FALSE));
              }
           }
           /*
           ** must be done here
           ** else on the mousemove the
           ** text outline is moved and nothing is drawn!
           */
           if (wi.pvCurrent && !linestretch && !objectstretch)
           {
              /*
              ** So if we are here, there is something selected
              ** so unselect and overwrite the selectbox..
              ** Further in the WM_BUTTON1DOWN message we
              ** look were you clicked.....
              */
              showSelectHandles(wi.pvCurrent,&wi);
              wi.pvCurrent  = NULL;
              if (!Drawing())
                 wi.op_mode = NOSELECT;
           }

           if (!wi.pvCurrent && !Drawing() && (SHORT2FROMMP(mp2) == KC_SHIFT))
           {
              /*
              ** Do not set the button1down bool...
              ** Avoid continues bounding rect recalcs.
              ** And here we do a multiselect with mouse plus shift key.
              */
              rcl.xLeft = rcl.xRight = rcl.yTop = rcl.yBottom = 0;
              ObjShiftSelect(ptlPosn,&wi);
              wi.op_mode = MULTISELECT;
              return (MPARAM)0;
           }

           if (wi.op_mode == LINEDRAW && !linestretch)
           {
              pLin = OpenLineSegment(ptlStart,&wi);
              DrawLine(&wi,ptlStart,ptlEnd,CREATEMODE,(POBJECT)pLin);
              return 0;
           }
           else if (wi.op_mode == MULTISELECT)
           {
              remSelectHandles(&wi); /* Remove first all selection handles */
              wi.op_mode = MULTIMOVE;

              if (SHORT2FROMMP(mp2) == KC_CTRL)
              {
                 ObjMoveMultiOutline(&wi,0,0);
                 wi.op_mode = MULTICOPY;
              }
              WinSetPointer(HWND_DESKTOP,
                            WinQuerySysPointer(HWND_DESKTOP,SPTR_MOVE,FALSE));
              return 0;
           }
           else if (wi.op_mode == SPECLINEDRAW)
           {
              pLin = OpenLineSegment(ptlStart,&wi);
              DrawLine(&wi,ptlStart,ptlEnd,DRAWMODE,(POBJECT)pLin);
              return 0;
           }
           else if (wi.op_mode == FREELINEDRAW)
           {
              pLin = OpenLineSegment(ptlStart,&wi);
              DrawLine(&wi,ptlStart,ptlEnd,DRAWMODE,(POBJECT)pLin);
              return 0;
           }
           else if (wi.op_mode == FREESTYLE)
              return CreateSpline(&wi,&ptlStart,msg);
           else if (wi.op_mode == REGPOLYDRAW || wi.op_mode == REGPOLYSTAR)
           {
              RegularPolyCreate(msg,ptlStart,&wi,wi.op_mode);
              return 0;
           }
           else if (wi.op_mode == SPLINEDRAW)
           {
              return CreateSpline(&wi,&ptlStart,msg);
           }
           else if (wi.op_mode == SQUAREDRAW)
           {
              pCreate = openSqrSegment(ptlStart,&wi);
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,MAKE_SQUARE);
              return 0;
           }

           if (linestretch)
           {
              if (corner == 1) /* So point one moves which is ptlEnd */
              {
                 ptlStart.x = (LONG)(pLin->ptl2.x * wi.usFormWidth);
                 ptlStart.y = (LONG)(pLin->ptl2.y * wi.usFormHeight);
                 ptlEnd.x =   (LONG)(pLin->ptl1.x * wi.usFormWidth);
                 ptlEnd.y =   (LONG)(pLin->ptl1.y * wi.usFormHeight);

              }
              else if (corner == 2)
              {
                 ptlStart.x = (LONG)(pLin->ptl1.x * wi.usFormWidth);
                 ptlStart.y = (LONG)(pLin->ptl1.y * wi.usFormHeight);
                 ptlEnd.x =   (LONG)(pLin->ptl2.x * wi.usFormWidth);
                 ptlEnd.y =   (LONG)(pLin->ptl2.y * wi.usFormHeight);
              }
              LineInvArea((POBJECT)pLin,&rcl,&wi,TRUE); /* remember the inv area */
              if (SHORT2FROMMP(mp2) == KC_CTRL)
              {
                 pLin = (pLines)ObjectCopy((POBJECT)pLin);
                 wi.pvCurrent = (POBJECT)pLin;
              }
              DrawLine(&wi,ptlStart,ptlEnd,CREATEMODE,(POBJECT)pLin);
              return 0;
           }
           else if (objectstretch)
           {
              if (wi.op_mode == CIRSELECT || wi.op_mode == SPLINESELECT ||
                  wi.op_mode == METAPICSELECT || wi.op_mode == IMGSELECT||
                  wi.op_mode == SQRSELECT || wi.op_mode == TEXTSELECT   ||
                  wi.op_mode == GROUPSELECT || wi.op_mode == BOXSELECT  ||
                  wi.op_mode == BLOCKTEXTSEL)
              {
                 showSelectHandles(wi.pvCurrent,&wi);
              }
              else    /* default, so no select and no handles!!! */
              {
                 showSelectHandles(wi.pvCurrent,&wi);
                 objectstretch = FALSE;
                 wi.pvCurrent     = NULL;
                 wi.op_mode       = NOSELECT;
              }
           }
           else if (wi.op_mode == CIRCLEDRAW)
           {
              pCir = OpenCircleSegment(&wi);
              if (wi.op_mode == CIRCLEDRAW)
              {
                 ptlEnd = ptlStart;
                 wi.fxArc = MAKEFIXED(360,0);
                 wi.pvCurrent = (POBJECT)pCir;
                 createCircle(&wi,ptlStart,msg,0);
              }
              return 0;
           }
/*
**------------------CHECK IF WE SELECTED AN OBJECT !!-----------------------
*/
           if (objectstretch || (pObj = ObjSelect(ptlPosn,&wi))!=NULL)
           {
              if (objectstretch)
                 pObj = wi.pvCurrent;

              wi.pvCurrent   = pObj;

              if (!objectstretch)
                 WinSetPointer(HWND_DESKTOP,
                               WinQuerySysPointer(HWND_DESKTOP,SPTR_MOVE,FALSE));

              if (pObj->usClass == CLS_TXT)
                 showTxtFont(pObj,&wi); /* Set font name in status line */
              else if (pObj->usClass == CLS_BLOCKTEXT)
                 showBlockTextFont(pObj,&wi); /* Set font name in status line */
              ObjOutLine(pObj,&rcl,&wi,FALSE);
              ptlEnd.x   = ptlPosn.x;
              ptlEnd.y   = ptlPosn.y;

              WinPostMsg(wi.hwndMain,UM_SIZEHASCHANGED,(MPARAM)0,
                         MPFROM2SHORT((USHORT)abs(rcl.xRight - rcl.xLeft),
                                      (USHORT)abs(rcl.yTop - rcl.yBottom)));

              ObjInvArea(pObj,&rcl,&wi,TRUE);
              /*
              ** Map the selection to an operation mode.
              */
              wi.op_mode = mapSelTypeOpmode(pObj->usClass); /*dlg_sel.c*/

              switch(pObj->usClass)
              {
/*
** CIRCLE SELECTED ????
*/
                 case CLS_CIR:
                    pCir = (pCircle)wi.pvCurrent;
                    /*
                    ** STRETCHING????
                    */
                    if (SHORT2FROMMP(mp2) == KC_CTRL )
                    {
                       pCir = (pCircle)ObjectCopy((POBJECT)pCir);
                       wi.pvCurrent = (POBJECT)pCir;
                    }

                    if (objectstretch)
                    {
                       ObjGetBoxCorners(wi.pvCurrent,&ptlStart,corner,&wi);
                       ObjStretch(wi.pvCurrent,&ptlStart,&ptlEnd,&wi,msg);
                       /*
                       ** if we are a partial arc e.g less than 360 degrees
                       ** then setup the wi.usarc which is used at the
                       ** close circle.
                       */
                       wi.fxArc = pCir->SweepAngle;
                    }
                    else
                    {
                       ObjOutLine(pObj,&rcl,&wi,FALSE);
                       ptlStart.x = rcl.xLeft;
                       ptlStart.y = rcl.yTop;
                       ptlEnd.x   = rcl.xRight;
                       ptlEnd.y   = rcl.yBottom;
                       SelectBox(wi.hps,&ptlStart,&ptlEnd,(SHORT)0,(SHORT)0,DRAG_START);
                       ObjInvArea(pObj,&rcl,&wi,TRUE);
                       ptlEnd.x=ptlPosn.x;
                       ptlEnd.y=ptlPosn.y;
                       ptlStart.x=ptlPosn.x;
                       ptlStart.y=ptlPosn.y;
                    }
                    return 0;
/*
** LINE SELECTED ????
*/
                 case CLS_LIN:
                    pLin = (pLines)wi.pvCurrent;
                    if (SHORT2FROMMP(mp2) == KC_CTRL)
                    {
                       pLin = (pLines)ObjectCopy((POBJECT)pLin);
                       wi.pvCurrent = (POBJECT)pLin;
                    }
                    showSelectHandles(wi.pvCurrent,&wi);
                    return 0;
/*
** An image selected??? or a metafile? or group.............
*/
                 default:
                    if (SHORT2FROMMP(mp2) == KC_CTRL )
                       wi.pvCurrent = ObjectCopy(wi.pvCurrent);

                    if (objectstretch)
                    {
                       /*
                       ** Here we overwrite ptlStart!!!
                       ** To start the stretch box
                       */
                       ObjGetBoxCorners(wi.pvCurrent,&ptlStart,corner,&wi);
                       ObjStretch(wi.pvCurrent,&ptlStart,&ptlEnd,&wi,msg);
                    }
                    else
                    {
                       ObjOutLine(wi.pvCurrent,&rcl,&wi,FALSE);
                       ptlStart.x = rcl.xLeft;
                       ptlStart.y = rcl.yTop;
                       ptlEnd.x   = rcl.xRight;
                       ptlEnd.y   = rcl.yBottom;
                       SelectBox(wi.hps,&ptlStart,&ptlEnd,(SHORT)0,(SHORT)0,DRAG_START);
                       ObjInvArea(pObj,&rcl,&wi,TRUE);
                       ptlEnd.x=ptlPosn.x;
                       ptlEnd.y=ptlPosn.y;
                       ptlStart.x=ptlPosn.x;
                       ptlStart.y=ptlPosn.y;
                    }
                    return 0;
              } /* switch usClass */
           } /* endof if (pobj) */

           WinPostMsg(wi.hwndMain,UM_FORMSIZE,
                      (MPARAM)0,MPFROM2SHORT((USHORT)wi.usFormWidth,(USHORT)wi.usFormHeight));

           WinPostMsg(wi.hwndMain, UM_FNTHASCHANGED,
                      (MPARAM)&wi.fontname,(MPARAM)0);

           if (wi.op_mode != MULTISELECT)
           {
              /* if there is nothing selected make a start
              ** for a multiselect box.
              */
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
              wi.op_mode = MULTISELECT;
              wi.pvCurrent = NULL;
           }
           return 0;
/*------------------------------------------------------------------------*/
      case WM_BUTTON1UP:
           if (!button1down)
              return (MRESULT)0;

           button1down= FALSE;
           WinSetCapture(HWND_DESKTOP,(HWND)0);

           if (wi.op_mode == OBJROTATION && wi.pvCurrent)
           {
              if (ObjDrawRotLine(&wi,wi.pvCurrent,ptlEnd,msg))
              {

                 if ( wi.pvCurrent->usClass == CLS_GROUP)
                 {
                    GroupCalcOutline(wi.pvCurrent,&wi);
                 }

                 WinInvalidateRect(hwnd,&rcl,TRUE); /*our old rect !*/
                 ObjRefresh(wi.pvCurrent,&wi);
              }
              return (MRESULT)0;
           }

           if (wi.op_mode == MOVECENTER && wi.pvCurrent)
           {
              wi.op_mode = OBJROTATION; /* Back to rotation mode !!*/
              return ObjMoveCenter(&wi,wi.pvCurrent,ptlEnd,msg);
           }

           if (wi.op_mode == MULTISELECT)
           {
              /*
              ** Remove multiselect box
              */
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
              if (!ObjQuerySelect())
              {
                 /*
                 ** There is no object found which is selected
                 ** so quickly disappear...
                 */
                 wi.pvCurrent = NULL;
                 wi.op_mode = NOSELECT;
              }
              else
              {
                 /*
                 ** Get quickly the size of the select box and
                 ** remember it as the first inv area.
                 ** For multicopy or for changing the colors etc!
                 */
                 ObjBoundingRect(&wi,&rcl,TRUE);
              }
           }
           else if (wi.op_mode == OBJFORMCHANGE)
           {
              WinInvalidateRect(wi.hwndClient,&rcl,TRUE); /*our old rect !*/
              ObjDragHandle((POBJECT)wi.pvCurrent,&wi,ptlEnd,msg,mp2);
              ObjRefresh(wi.pvCurrent,&wi);
           }
           else if (wi.op_mode == MULTIMOVE  || wi.op_mode == MULTICOPY)
           {
              ObjBoundingRect(&wi,&rcl,TRUE);
              ObjMultiMoveCopy(&wi,(ptlEnd.x-ptlStart.x),(ptlEnd.y-ptlStart.y), wi.op_mode);
//@ch4        DragBox(wi.hps,
//@ch4               (POINTL *)0,
//@ch4               (POINTL *)0,
//@ch4               (SHORT)(ptlEnd.x-ptlStart.x),
//@ch4               (SHORT)(ptlEnd.y-ptlStart.y),DRAG_MOVE);
//@ch4        if (wi.op_mode == MULTIMOVE)
              WinInvalidateRect(hwnd,&rcl,TRUE); /*our old rect !*/
              /*
              ** Objboundingect fails here since the new created objects
              ** (MULTICOPY) are not selected.
              */
              GpiConvert(wi.hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlStart);
              GpiConvert(wi.hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlEnd);
              rcl.xLeft   += (ptlEnd.x-ptlStart.x);
              rcl.xRight  += (ptlEnd.x-ptlStart.x);
              rcl.yTop    += (ptlEnd.y-ptlStart.y);
              rcl.yBottom += (ptlEnd.y-ptlStart.y);
              WinInvalidateRect(hwnd,&rcl,TRUE); /*our new rect !*/
              changeMode(&wi,NOSELECT,0L);
           }
           else if (wi.op_mode == LINEDRAW  || linestretch)
           {
              if (!linestretch)
              {
                 CloseLineSegment(pLin, ptlEnd,&wi);
                 ObjRefresh((POBJECT)pLin,&wi);
              }
              else
              {
                 if (corner == 1) /* So point one moves which is ptlEnd */
                 {
                    pLin->ptl1.x  =  (float)ptlEnd.x;
                    pLin->ptl1.y  =  (float)ptlEnd.y;
                    pLin->ptl1.x  /= (float)wi.usFormWidth;
                    pLin->ptl1.y  /= (float)wi.usFormHeight;
                 }
                 else if (corner == 2)
                 {
                    pLin->ptl2.x  =  (float)ptlEnd.x;
                    pLin->ptl2.y  =  (float)ptlEnd.y;
                    pLin->ptl2.x  /= (float)wi.usFormWidth;
                    pLin->ptl2.y  /= (float)wi.usFormHeight;
                 }
                 WinInvalidateRect(hwnd,&rcl,TRUE);   /* Delete old line!! */
                 ObjRefresh((POBJECT)pLin,&wi);
              }
              if (!linestretch)
                 DrawLine(&wi,ptlStart,ptlEnd,DRAWMODE,(POBJECT)pLin);
              linestretch = FALSE;
              return 0; /* to keep the drawing pointer */
           }
           else if (wi.op_mode == SPECLINEDRAW)
           {
              CloseLineSegment(pLin,ptlEnd,&wi);
              return 0; /* to keep the drawing pointer */
           }
           else if (wi.op_mode == REGPOLYDRAW || wi.op_mode == REGPOLYSTAR)
           {
              RegularPolyCreate(msg,ptlEnd,&wi,wi.op_mode);
              return 0;
           }
           else if (wi.op_mode == FREELINEDRAW)
           {
              CloseLineSegment(pLin, ptlEnd,&wi);
              return 0; /* to keep the drawing pointer */
           }
           else if (wi.op_mode == FREESTYLE)
              return CreateSpline(&wi,&ptlStart,msg);
           else if (wi.op_mode == SPLINEDRAW)
           {
              return CreateSpline(&wi,&ptlStart,msg);
           }
           else if (wi.op_mode == SQUAREDRAW)
           {
              pCreate = closeSqrSegment(pCreate,ptlEnd,&wi);
              return 0; /* to keep the drawing pointer */
           }
           else if (wi.op_mode == CIRCLEDRAW )
           {
              createCircle(&wi,ptlEnd,msg,SHORT2FROMMP(mp2));
              ObjRefresh((POBJECT)pCir,&wi);
              wi.pvCurrent = NULL; /* No current selection at this point */
              return 0; /* to keep the drawing pointer */
           }
           else if (wi.op_mode == IMGSELECT  || wi.op_mode== METAPICSELECT ||
                    wi.op_mode == SQRSELECT  || wi.op_mode== TEXTSELECT    ||
                    wi.op_mode == GROUPSELECT || wi.op_mode == BOXSELECT   ||
                    wi.op_mode == CIRSELECT || wi.op_mode == SPLINESELECT  ||
                    wi.op_mode == BLOCKTEXTSEL)
           {
              /*
              ** do not invalidate the rect if there was no shift!
              ** This gives better performance.
              */
              if (objectstretch)
              {
                 WinInvalidateRect(hwnd,&rcl,TRUE);    /* our old rectl... */

                 ObjStretch(wi.pvCurrent,&ptlStart,&ptlEnd,&wi,msg);
                 rcl.xLeft    = min(ptlStart.x,ptlEnd.x) - HANDLESIZE * 2;
                 rcl.xRight   = max(ptlStart.x,ptlEnd.x) + HANDLESIZE * 2;
                 rcl.yBottom  = min(ptlStart.y,ptlEnd.y) - HANDLESIZE * 2;
                 rcl.yTop     = max(ptlStart.y,ptlEnd.y) + HANDLESIZE * 2;
                 /*
                 ** convert to pixels....
                 */
                 GpiConvert(wi.hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,
                            (POINTL *)&rcl);
                 WinInvalidateRect(hwnd,&rcl,TRUE);    /* our new rectl... */
                 return 0;
              }
              else if (ptlEnd.x != ptlPosn.x || ptlEnd.y != ptlPosn.y)
              {
                 MoveObject(wi.pvCurrent,
                             (SHORT)(ptlEnd.x - ptlStart.x),
                             (SHORT)(ptlEnd.y - ptlStart.y),
                             &wi);
                 return 0;
              }
           }
           else if (wi.op_mode == LINESELECT && !linestretch)
           {
              if (!pLin)
                 return 0;
              ObjMove((POBJECT )pLin,
                      (SHORT)(ptlEnd.x-ptlStart.x),
                      (SHORT)(ptlEnd.y-ptlStart.y),
                      &wi);

              WinInvalidateRect(hwnd,&rcl,TRUE);    /* our old rectl... */
              ObjRefresh((POBJECT)pLin,&wi);
           }
           else if (wi.op_mode == IMGCROP)
           {
              /*
              ** quickly determine the size of the full
              ** image to set the invalidated area
              */
              ImgInvArea(wi.pvCurrent, &rcl, &wi, TRUE);
              ImageCrop(ptlStart,ptlEnd,(pImage)wi.pvCurrent,&wi);
              wi.op_mode = IMGSELECT;
              WinInvalidateRect(hwnd,&rcl,TRUE);
           }
           WinSetPointer(HWND_DESKTOP,
                         WinQuerySysPointer(HWND_DESKTOP,
                                            SPTR_ARROW,FALSE));

           return 0;
/*------------------------------------------------------------------------*/
/* MOUSEMOVE checks first what is selected and decides then how to handle */
/*           at the moment only text selection is possible.               */
/*------------------------------------------------------------------------*/
      case WM_MOUSEMOVE:
           ptlMove.x = MOUSEMSG(&msg)->x;
           ptlMove.y = MOUSEMSG(&msg)->y;

           GpiConvert(wi.hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlMove);
           if (wi.bGrid && button1down)
              Snap2Grid(&wi,&ptlMove);

           /*
           ** Show in pos status line.
           */
           WinPostMsg(wi.hwndMain,UM_POSHASCHANGED,(MPARAM)0,
                      MPFROM2SHORT((SHORT)ptlMove.x,(SHORT)ptlMove.y));

           if (button1down)
           {

           if (wi.op_mode == OBJROTATION && wi.pvCurrent)
           {
              GpiSetMix(hps,FM_INVERT);
              ObjDrawRotLine(&wi,wi.pvCurrent,ptlEnd,msg);
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              GpiSetMix(hps,FM_INVERT);
              ObjDrawRotLine(&wi,wi.pvCurrent,ptlEnd,msg);
              return (MRESULT)0;
           }

           if (wi.op_mode == MOVECENTER && wi.pvCurrent)
           {
              GpiSetMix(wi.hps,FM_INVERT);
              ObjMoveCenter(&wi,wi.pvCurrent,ptlEnd,msg);
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              GpiSetMix(wi.hps,FM_INVERT);
              ObjMoveCenter(&wi,wi.pvCurrent,ptlEnd,msg);
              return (MRESULT)0;
           }

           if (wi.op_mode == MULTISELECT)
           {
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
              ObjMultiSelect(ptlStart,ptlEnd,&wi);
              WinPostMsg(wi.hwndMain,UM_SIZEHASCHANGED,
                 (MPARAM)0,MPFROM2SHORT((USHORT)abs(ptlStart.x - ptlEnd.x),
                              (USHORT)abs(ptlStart.y - ptlEnd.y)));
              return (MRESULT)0;
           }
           else if (wi.op_mode ==  OBJFORMCHANGE)
           {
              GpiSetMix(hps,FM_INVERT);
              ObjDragHandle(wi.pvCurrent,&wi,ptlEnd,msg,mp2);
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              GpiSetMix(hps,FM_INVERT);
              ObjDragHandle(wi.pvCurrent,&wi,ptlEnd,msg,mp2);
           }
           else if (wi.op_mode == MULTIMOVE  || wi.op_mode == MULTICOPY )
           {
              GpiSetMix(hps,FM_INVERT);
              ObjMoveMultiOutline(&wi,(SHORT)(ptlEnd.x-ptlStart.x),
                                  (SHORT)(ptlEnd.y-ptlStart.y));
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              GpiSetMix(hps,FM_INVERT);
              ObjMoveMultiOutline(&wi,(SHORT)(ptlEnd.x-ptlStart.x),
                                  (SHORT)(ptlEnd.y-ptlStart.y));
           }
           else if (wi.op_mode == LINEDRAW || linestretch)
           {
              DrawLine(&wi,ptlStart,ptlEnd,CREATEMODE,(POBJECT)pLin);
              if (SHORT2FROMMP(mp2) == KC_SHIFT)
              {
                 if (abs(ptlMove.x - ptlPosn.x) < abs(ptlMove.y - ptlPosn.y))
                    ptlMove.x = ptlPosn.x;
                 else 
                    ptlMove.y = ptlPosn.y;
              }
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              DrawLine(&wi,ptlStart,ptlEnd,CREATEMODE,(POBJECT)pLin);
           }
           else if (wi.op_mode == SPECLINEDRAW)
           {
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              DrawLine(&wi,ptlStart,ptlEnd,DRAWMODE,(POBJECT)pLin);
              CloseLineSegment(pLin,ptlEnd,&wi);
              pLin = OpenLineSegment(ptlStart,&wi);
           }
           else if (wi.op_mode == REGPOLYDRAW || wi.op_mode == REGPOLYSTAR)
           {
              GpiSetMix(wi.hps,FM_INVERT);
              RegularPolyCreate(msg,ptlEnd,&wi,wi.op_mode);
              ptlEnd.x = ptlMove.x;
              ptlEnd.y = ptlMove.y;
              GpiSetMix(wi.hps,FM_INVERT);
              RegularPolyCreate(msg,ptlEnd,&wi,wi.op_mode);
              return 0;
           }
           else if (wi.op_mode == FREELINEDRAW)
           {
              ptlStart.x = ptlEnd.x;
              ptlStart.y = ptlEnd.y;
              ptlEnd.x   = ptlMove.x;
              ptlEnd.y   = ptlMove.y;
              DrawLine(&wi,ptlStart,ptlEnd,DRAWMODE,(POBJECT)pLin);
              CloseLineSegment(pLin,ptlEnd,&wi);
              pLin = OpenLineSegment(ptlEnd,&wi);
           }
           else if (wi.op_mode == SPLINEDRAW)
           {
              return CreateSpline(&wi,&ptlMove,msg);
           }
           else if (wi.op_mode == FREESTYLE)
           {
              CreateSpline(&wi,&ptlMove,msg);
              ptlStart.x = ptlMove.x;
              ptlStart.y = ptlMove.y;
              return (MRESULT)0;
           }
           else if (wi.op_mode == SQUAREDRAW )
           {
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,MAKE_SQUARE);
              if (SHORT2FROMMP(mp2) == KC_SHIFT)
              {
                 ptlEnd.x = ptlMove.x;
                 ptlEnd.y = ptlStart.y - (ptlEnd.x - ptlStart.x ); /* Make dy == dx */
              }
              else
              {
                 ptlEnd.x =ptlMove.x;
                 ptlEnd.y =ptlMove.y;
              }
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,MAKE_SQUARE);
           }
           else if (wi.op_mode == CIRCLEDRAW)
           {
             createCircle(&wi,ptlEnd,msg,SHORT2FROMMP(mp2));
             ptlEnd.x = ptlMove.x;
             ptlEnd.y = ptlMove.y;
             createCircle(&wi,ptlEnd,msg,SHORT2FROMMP(mp2));
             WinPostMsg(wi.hwndMain,UM_SIZEHASCHANGED,(MPARAM)0,
                 MPFROM2SHORT((USHORT)abs(ptlStart.x - ptlEnd.x),
                              (USHORT)abs(ptlStart.y - ptlEnd.y)));
             return (MRESULT)0;
           }
           else if (wi.op_mode == METAPICSELECT || wi.op_mode == IMGSELECT ||
                    wi.op_mode == SQRSELECT     || wi.op_mode == TEXTSELECT||
                    wi.op_mode == GROUPSELECT   || wi.op_mode == CIRSELECT ||
                    wi.op_mode == LINESELECT    || wi.op_mode == SPLINESELECT ||
                    wi.op_mode == BLOCKTEXTSEL)
           {
              if (objectstretch)
              {
                 GpiSetMix(wi.hps,FM_INVERT);
                 ObjStretch(wi.pvCurrent,&ptlStart,&ptlEnd,&wi,msg);

                 ptlEnd.x =ptlMove.x;
                 ptlEnd.y =ptlMove.y;
                 WinSendMsg(wi.hwndMain,UM_SIZEHASCHANGED,(MPARAM)0,
                            MPFROM2SHORT((USHORT)abs(ptlEnd.x - ptlStart.x),
                                         (USHORT)abs(ptlEnd.y - ptlStart.y)));
                 GpiSetMix(wi.hps,FM_INVERT);
                 ObjStretch(wi.pvCurrent,&ptlStart,&ptlEnd,&wi,msg);
              }
              else
              {
              ObjMoveOutLine(wi.pvCurrent,&wi,
                             (SHORT)(ptlEnd.x-ptlStart.x),
                             (SHORT)(ptlEnd.y-ptlStart.y));

              if (wi.op_mode != LINESELECT)
                 SelectBox(wi.hps,(POINTL *)0,(POINTL *)0,
                           (SHORT)(ptlEnd.x-ptlStart.x),
                           (SHORT)(ptlEnd.y-ptlStart.y),DRAG_MOVE);
              else
                 GpiSetMix(hps,FM_INVERT);

              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;

              if (wi.op_mode != LINESELECT)
                 SelectBox(wi.hps,(POINTL *)0,(POINTL *)0,
                           (SHORT)(ptlEnd.x-ptlStart.x),
                           (SHORT)(ptlEnd.y-ptlStart.y),DRAG_MOVE);
              else
                 GpiSetMix(hps,FM_INVERT);

              ObjMoveOutLine(wi.pvCurrent,&wi,
                             (SHORT)(ptlEnd.x-ptlStart.x),
                             (SHORT)(ptlEnd.y-ptlStart.y));

              }
           }
           else if (wi.op_mode == IMGCROP)
           {
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
              ptlEnd.x =ptlMove.x;
              ptlEnd.y =ptlMove.y;
              MultiSelectBox(wi.hps, &ptlStart, &ptlEnd,SELECT);
           }

           } /* endof if button1down */
           else if (wi.op_mode == LINESELECT)
           {
              pLin = (pLines)wi.pvCurrent;
              linestretch = IsOnLineEnd(ptlMove, pLin,&wi, &corner);
              if (!linestretch)
                 break;
           }
           else if ( wi.pvCurrent && wi.op_mode == OBJROTATION)
           {
             if (ObjPtrAboveCenter(&wi,wi.pvCurrent,ptlMove))
             {
               corner = ABOVECENTER;  /* YES you selected the center point!!*/
               WinSetPointer(HWND_DESKTOP,
                            WinQuerySysPointer(HWND_DESKTOP,SPTR_MOVE,FALSE));

             }
             else
             {
               WinSetPointer(HWND_DESKTOP,WinQuerySysPointer(HWND_DESKTOP,
                             SPTR_ARROW,FALSE));

               corner = 0;            /* NO you missed it */
             }
           }
           else if ( wi.pvCurrent && wi.op_mode != IMGCROP && wi.op_mode != OBJFORMCHANGE)
           {
              ObjOutLine(wi.pvCurrent,&rcl,&wi,FALSE);
              objectstretch = ObjIsOnCorner(&wi,wi.pvCurrent,ptlMove,&rcl,&corner);
              if (!objectstretch)
                 break;
           }
           else
              drwSetPointer(wi.op_mode);
           return 0;
/*========================================================================*/
/* React on the drag info of the color pallet by changing the background  */
/* if the verfroller hovers over us.                                      */
/*========================================================================*/
      case WM_PRESPARAMCHANGED:
         WinQueryPresParam(hwnd, (int)mp1, 0, (PULONG)&fnd,
                           sizeof(buffer)-1, buffer, 0);

         WinQueryPointerPos(HWND_DESKTOP,&ptlPosn);
         WinMapWindowPoints(HWND_DESKTOP,hwnd,&ptlPosn,1);
         GpiConvert(wi.hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlPosn);

         switch ((long)mp1)
         {
            case PP_FOREGROUNDCOLOR:
               wi.ulColor = *(ULONG*)buffer;
               WinInvalidateRect(hwnd, 0,TRUE);
               WinPostMsg(wi.hwndMain,UM_CLRHASCHANGED,(MPARAM)0,(MPARAM)0);
               break;
            case PP_BACKGROUNDCOLOR:
                  if (wi.op_mode == MULTISELECT)
                  {
                     ObjBoundingRect(&wi,&rcl,TRUE);
                     ObjSetMltFillClr(*(ULONG*)buffer);
                     wi.pvCurrent = NULL;
                     changeMode(&wi,NOSELECT,0L);
                     WinInvalidateRect(hwnd,&rcl,TRUE);
                  }
                  else if ((pObj = (POBJECT)ObjSelect(ptlPosn,&wi))!=NULL)
                  {
                     if (ObjDropFillClr(&wi,*(ULONG*)buffer,pObj))
                        ObjRefresh((POBJECT)pObj,&wi);
                  }
                  else
                  {
                     wi.lBackClr = *(ULONG*)buffer;
                     WinInvalidateRect(hwnd, 0,TRUE);
                  }
               break;
            case 0x0000000f:
               if (wi.op_mode == MULTISELECT)
               {
                  if (strchr(buffer,'.'))
                  {
                     p = strchr(buffer,'.');
                     p++; /* jump over dot */
                     ObjMultiFontChange(p);
                  }
      
                  changeMode(&wi,NOSELECT,0L);
                  WinInvalidateRect(hwnd,&rcl,TRUE);
               }
               else if ((pObj = ObjSelect(ptlPosn,&wi))!=NULL)
               {
                  if (strchr(buffer,'.'))
                  {
                     p = strchr(buffer,'.');
                     p++; /* jump over dot */
                     ObjFontChange(p,pObj);
                     WinInvalidateRect(hwnd, 0,TRUE);
                  }
               }
               else
               {
                  if (strchr(buffer,'.'))
                  {  strcpy(wi.fattrs.szFacename,buffer);
                     p = strchr(buffer,'.');
                     *p = 0;                     /* overwrite dot   */
                     wi.sizfx.cx = MAKEFIXED(atoi(buffer),0); /* Get point value */
                     wi.sizfx.cy = wi.sizfx.cx;
                     p++; /* jump over zero */
                     setFont(&wi,&wi.fattrs,wi.sizfx);
                     if ( wi.op_mode == TEXTINPUT )
                     {
                        WinShowCursor(wi.hwndClient,FALSE);
                        WinDestroyCursor(wi.hwndClient);
                        Createcursor(&wi); /*drwtxtin.c*/
                     }
                     WinPostMsg(wi.hwndMain, UM_FNTHASCHANGED,
                                (MPARAM)&wi.fontname,(MPARAM)0);
                  }
               }
         }
         return 0L;
/*========================================================================*/
      case WM_CHAR:
         if (SHORT1FROMMP(mp1) & KC_KEYUP )
            return 0;
         if (SHORT1FROMMP(mp1) & KC_INVALIDCHAR )
            return 0;

         if (GpiQueryCharSet(wi.hps)!= wi.lcid)
            setFont(&wi,&wi.fattrs,wi.sizfx);

         if (wi.op_mode == TEXTINPUT)
         {
           if (DrwTextInp(hwnd, mp1,mp2, &wi))
              return (MRESULT)0;
         }

         for (sRep = 0 ; sRep < CHAR3FROMMP(mp1); sRep++)
         {
         /*---------------------------
          Process some virtual keys
         ---------------------------*/
            if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
            {
               if (handleCursorKeys(&wi,SHORT2FROMMP(mp2)))
                  return (MRESULT)0;

               switch (SHORT2FROMMP(mp2))
               {
                  case VK_PAGEUP:
                  case VK_PAGEDOWN:
                     return WinSendMsg(hwndCcanv,msg,mp1,mp2);
                  case VK_ENTER:
                  case VK_NEWLINE:
                     if (wi.op_mode == SPLINEDRAW || wi.op_mode == FREESTYLE)
                        if (!button1down)
                           return CreateSpline(&wi,&ptlStart,msg);
                     break;

                  case VK_ESC:    return KeybEsc();
                  case VK_DELETE:
                        if (wi.op_mode == MULTISELECT )
                        {
                           ObjBoundingRect(&wi,&rcl,TRUE);
                           ObjMultiDelete((POBJECT)0);
                           wi.op_mode = NOSELECT;
                           wi.pvCurrent=NULL;
                           WinInvalidateRect(hwnd,&rcl,TRUE);
                           /* see drwutl.c*/
                           wi.bFileHasChanged = ObjectsAvail();
                        }
                        else if (wi.op_mode == OBJFORMCHANGE && wi.pvCurrent)
                        {
                           ObjRefresh(wi.pvCurrent,&wi);
                           ObjDelHandle(wi.pvCurrent);
                           ObjRefresh(wi.pvCurrent,&wi);                           
                        }
                        else if (wi.pvCurrent)
                        {
                          if ( (wi.pvCurrent)->bLocked )
                          {
                             WinAlarm(HWND_DESKTOP, WA_WARNING);
                             return 0;
                          }

                          ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                          ObjDelete(wi.pvCurrent);
                          wi.op_mode = NOSELECT;
                          wi.bFileHasChanged = TRUE;
                          wi.pvCurrent=NULL;
                          WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                        }
                     break;

                  default:
                     break ;
               }
            }
            else if (SHORT1FROMMP(mp1) & KC_CHAR)
            {
               if (wi.op_mode == IMGSELECT && wi.pvCurrent)
               {
                  ImgInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                  if (ImgRotateColor((pImage)wi.pvCurrent,SHORT1FROMMP(mp2)))
                     WinInvalidateRect(hwnd,&rcl,TRUE);
               }
               else if (wi.op_mode == SPLINEDRAW)
               {
                  DelLastPoint(&wi,SHORT1FROMMP(mp2));
               }
            } 
         }
         return 0 ;

      case WM_CREATE:
           return (MRESULT)DrawCreate(hwnd,&wi);
      case WM_SIZE:
           wi.cxClient = (LONG)SHORT1FROMMP (mp2);
           wi.cyClient = (LONG)SHORT2FROMMP (mp2);
           if (!wi.cxClnt && !wi.cyClnt)
           {
             /* remember the original size for restoring zoom mode */
             wi.cxClnt = wi.cxClient;
             wi.cyClnt = wi.cyClient;
           }
           CanvScrollBar(&wi,FALSE);
           return 0;
       case WM_TIMER:
          if (wi.op_mode == FREESTYLE && button1down)
          {
             if (ptlStart.x == ptlEnd.x && ptlStart.y == ptlEnd.y )
                return (MRESULT)0;
             CreateSpline(&wi,&ptlStart,WM_BUTTON1DOWN);
             ptlEnd = ptlStart;
          }
          return (MRESULT)0;

      case UM_OBJCOLORCHANGE:
         if (wi.pvCurrent)
         {
            if (!mp1)
               ObjSetFillClr(wi.ulColor,wi.pvCurrent);
            else
               ObjSetOutLineClr(wi.ulOutLineColor,wi.pvCurrent);
            ObjRefresh(wi.pvCurrent,&wi);
         }
         else  if (wi.op_mode == MULTISELECT)
         {
            ObjBoundingRect(&wi,&rcl,TRUE);
            if (!mp1)
               ObjSetMltFillClr(wi.ulColor);
            else
               ObjSetMltOutLineClr(wi.ulOutLineColor);
            changeMode(&wi,NOSELECT,0L);
            wi.pvCurrent = NULL;
            WinInvalidateRect(hwnd,&rcl,TRUE);
         }
         else if (wi.op_mode == TEXTINPUT)
         {
            UpdateInputString(&wi,TRUE);
         }
         return (MRESULT)0;
      case UM_SELECTTXT:
           if (SHORT2FROMMP(mp2)==IDM_SELTEXT && wi.op_mode == TEXTSELECT)
           {
              ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
              textDetail(hwnd,&wi);
           }
           else if (SHORT2FROMMP(mp2)==IDM_EDITTEXT )
           {
               ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
               ObjEditText(wi.pvCurrent,&wi);
           }
           else if (wi.op_mode == MULTISELECT )
           {
              wi.op_mode = NOSELECT;
              ObjBoundingRect(&wi,&rcl,FALSE);
              if (SHORT2FROMMP(mp2)==IDM_EXPSELMETA)
              {
                 if (ObjQuerySelect())
                 {
                    RecordMetaFile(hab,&wi,&rcl);
                 }
                 else
                    RecordMetaFile(hab,&wi,(RECTL *)0);
              }
              else if (SHORT2FROMMP(mp2)==IDM_OTHERFORM)
              {
                 /*
                 ** The user has selected anotoher format
                 ** to export it's selection via the dialog.
                 */
                 ExportSel2Bmp(&wi,&rcl,".NON",(USHORT)0);
                 ObjMultiUnSelect();
              }
              WinInvalidateRect(hwnd,&rcl,TRUE);
           }
           else if (SHORT2FROMMP(mp2)==IDM_BITMAPEXP)
              Export2Bmp(&wi); /* save total drawing in bmp format */
           return (MRESULT)0;

      case WM_PAINT:
           WinQueryWindowRect(hwnd, &rcl);
           /*
            * Draw color value set in client area like in paint brush.
            */
           hps = WinBeginPaint(hwnd,wi.hps,&rcl);
           GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
           WinFillRect(hps,&rcl,wi.lBackClr);
           GpiConvert(hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,2,(POINTL *)&rcl);

           WinSendMsg(wi.hwndMain,UM_BUSINESS,MPFROMLONG(IDS_DRAWING),
                      (MPARAM)0);

           if (wi.op_mode == TEXTINPUT)  DrawInputString(&wi);

           WinEndPaint(hps);

           WinPostQueueMsg( hmq2,UM_PAINT,
                            MPFROM2SHORT((SHORT)rcl.xLeft,(SHORT)rcl.xRight),
                            MPFROM2SHORT((SHORT)rcl.yBottom,(SHORT)rcl.yTop));
           return 0;

/*------------------------Drag & Drop-------------------------------------*/
   case DM_DRAGOVER:
      return (DragOver (hab, (PDRAGINFO) mp1, (PSZ)wi.szCurrentDir));
   case DM_DROP:
      return (Drop(hab,(PDRAGINFO)mp1,&wi));

   case WM_CLOSE:
      WinPostQueueMsg( hmq2, UM_EXIT, (MPARAM)0L, (MPARAM)0L );
      DosWaitThread( &tid, DCWW_WAIT );
      WinPostMsg( hwnd, WM_QUIT, (MPARAM)0L, (MPARAM)0L );
      return (MRESULT)0;

   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*--------------------------------about--------------------------------*/
MRESULT EXPENTRY AboutDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SWP swp;

   switch (msg)
   {
      case WM_INITDLG:
                       /* Centre dialog        on the screen                        */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
                       ((WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN) - swp.cx) / 2),
                       ((WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - swp.cy) / 2),
                       0, 0, SWP_MOVE);
         return 0;
      case WM_COMMAND:
           WinDismissDlg(hwnd,TRUE);
           break;
      default:
           return(WinDefDlgProc(hwnd, msg, mp1, mp2));
   }
   return (MRESULT)0;
}
/*--------------------------------ErrorBox--------------------------------*/
void ErrorBox(char *Str1, char *Str2)
{
    WinMessageBox(HWND_DESKTOP,
                  HWND_DESKTOP,
                  (PSZ)Str1,
                  (PSZ)Str2,1,
                  MB_CUACRITICAL | MB_SYSTEMMODAL | MB_OK);

    return;
}
/*------------------------------------------------------------------------*/
static void Draw3dBorder(HPS hps, POINTL ptl1, POINTL ptl2)
{
   RECTL  rcl;
   rcl.xLeft   = ptl1.x;
   rcl.yBottom = ptl2.y;
   rcl.xRight  = ptl2.x;
   rcl.yTop    = ptl1.y;
   WinDrawBorder(hps, &rcl,2L,2L,0L, 0L, DB_DEPRESSED);
   return;
}
/*---------------------------------------------------[ private ]----------*/
/*  DrawColorbox                                                          */
/*                                                                        */
/*  Description : Draws in the lowerleft corner a box showing the filling */
/*                and the outlinecolor.                                   */
/*                                                                        */
/*  Functions  :                                                          */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void DrawColorbox(HPS hps)
{
   POINTL ptl,ptl2;

  ptl.x = 1;      /* add a pixel */
  ptl.y = CYSTATUSLINE - 2 ;
  ptl2.y= 2;
  ptl2.x = CXBUTTON;
  Draw3dBorder(hps,ptl,ptl2);
   /*
   ** Top left of small box
   */
   GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
   GpiSetColor(hps,wi.ulOutLineColor);

   ptl.x += 3;
   ptl.y -= 3;
   GpiMove(hps,&ptl);

   ptl2.y += 3;
   ptl2.x -= 3;

   GpiBox(hps,DRO_FILL,&ptl2,0L,0L);

   ptl.x += 2;
   ptl.y -= 2;
   GpiMove(hps,&ptl);

   ptl2.y += 2;
   ptl2.x -= 2;
   GpiSetColor(hps,wi.ulColor);
   GpiBox(hps,DRO_FILL,&ptl2,0L,0L);
}
/*--------------------------------------------------------------------------*/
MRESULT EXPENTRY MainWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   HPS    hps;
   RECTL  rcl;
   static USHORT cxClient,cyClient;
   POINTL ptl;             /* Sets the current pos on text creation */
//   USHORT x,yPos;          /* Index buttonhwnd, yPos button         */
   HOBJECT hoColorPal;     /* Default colorpallette                 */
   HOBJECT hoFontPal;      /* Default fontpallette                  */
   static  char pszPos[25];  /* Displays the x,y coords of the mouse */
   static  char pszSize[25]; /* Displays the size of selected object */
   static  char pszlayr[25]; /* Displays the active layer */
   static  char pszbusy[25]; /* Displays the user what we do, like painting etc */
   static  char pszload[25]; /* displays printing or loading or none */

   static  USHORT cx,cy,x1,y1;
   static  POINTL pos,size,ptlfont,ptllay,ptlbusy,ptlloading;
   static  RECTL  rclSp;   /* Pre calculated inv area for POS & Size*/
   static  RECTL  rclfnt;  /* Pre calculated inv area for font      */
   static  RECTL  rcllay;  /* Pre calculated inv area for layer     */
   static  RECTL  rclbusy; /* Pre calculated inv area for business  */
   static  RECTL  rclload; /* Pre calculated inv area for loading   */
   static  POINTL ptlbox1,ptlbox2;  /* Pre calculated the box to draw */
   static  POINTL ptlbox3,ptlbox4;  /* font status box */
   static  POINTL ptlbox5,ptlbox6;  /* layerstatus box */
   static  POINTL ptlbox7,ptlbox8;  /* business box    */
   static  POINTL ptlbox9,ptlbox10; /* loading box     */
   static  SIZEF  sizfx;
   static  FONTMETRICS fontmetrics; /* used for box calculations    */
   static  char   szFontName[100];
   char        *p;                  /* For font changes.            */

   switch (msg)
   {

     case UM_POSHASCHANGED:
        ptl.x =(LONG)SHORT1FROMMP (mp2);
        ptl.y =(LONG)SHORT2FROMMP (mp2);
        CalcPtrPosition(&wi,&ptl);                    /* see drwcanv.c */
        sprintf(pszPos,"x:%4d y:%4d",ptl.x,ptl.y);
        rcl.xLeft = rclSp.xLeft;
        rcl.xRight= rclSp.xRight /2;
        rcl.yBottom=rclSp.yBottom;
        rcl.yTop   = rclSp.yTop;
        WinInvalidateRect(hwnd,&rcl,TRUE);
        return 0;

     case UM_CLRHASCHANGED: /* reflect the changed color in statusline */
        rcl.xLeft  = 0;
        rcl.xRight = CXBUTTON;
        rcl.yTop   = CYSTATUSLINE;
        rcl.yBottom= 0;
        WinInvalidateRect(hwnd,&rcl,TRUE);
        return (MRESULT)0;

     case UM_READWRITE:
        if ((LONG)mp1)
           WinLoadString(hab,(HMODULE)0,(LONG)mp1,
                         sizeof(pszload),(PSZ)pszload);
        else
           strcpy(pszload,"-----");
        WinInvalidateRect(hwnd,&rclload,TRUE);
        return (MRESULT)0;

     case UM_FNTHASCHANGED:
        if (mp1)
        {
           strcpy(szFontName,(char *)mp1);
        }
        WinInvalidateRect(hwnd,&rclfnt,TRUE);
        return 0;
     case UM_LAYERHASCHANGED:
        sprintf(pszlayr,"Layer:%2d",wi.uslayer);
        WinInvalidateRect(hwnd,&rcllay,TRUE);
        return 0;
           /*
           ** This message is posted from the spline dialog.....
           */
      case UM_OBJINVAREA:
         ObjRefresh(wi.pvCurrent,&wi);
         return (MRESULT)0;

     case UM_BUSINESS:
        WinLoadString(hab,
                     (HMODULE)0, (LONG)mp1,sizeof(pszbusy),(PSZ)pszbusy);
         WinInvalidateRect(hwnd,&rclbusy,TRUE);
         return 0;
     case UM_FORMSIZE:
        ptl.x =(LONG)SHORT1FROMMP (mp2);
        ptl.y =(LONG)SHORT2FROMMP (mp2);
        CalcPtrPosition(&wi,&ptl);                    /* see drwcanv.c */
        sprintf(pszSize,"cx:%4d cy:%4d",ptl.x,ptl.y);
        WinInvalidateRect(hwnd,&rclSp,TRUE);
        return 0;
     case UM_FORECOLORCHANGED:
        wi.ulColor = (ULONG)mp1;
        WinSendMsg(wi.hwndClient,UM_OBJCOLORCHANGE,(MPARAM)0,(MPARAM)0);
        hps = WinGetPS(hwnd);
        DrawColorbox(hps);
        WinReleasePS(hps);
        return (MRESULT)0;
     case UM_BACKCOLORCHANGED:
        wi.ulOutLineColor = (ULONG)mp1;
        WinSendMsg(wi.hwndClient,UM_OBJCOLORCHANGE,(MPARAM)1,(MPARAM)0);
        hps = WinGetPS(hwnd);
        DrawColorbox(hps);
        WinReleasePS(hps);
        return (MRESULT)0;

     case UM_SIZEHASCHANGED:
        ptl.x =(LONG)SHORT1FROMMP (mp2);
        ptl.y =(LONG)SHORT2FROMMP (mp2);
        CalcPtrPosition(&wi,&ptl); /* see drwcanv.c */
        sprintf(pszSize,"cx:%4d cy:%4d",ptl.x,ptl.y);
        WinInvalidateRect(hwnd,&rclSp,TRUE);
        return 0;

     case WM_CREATE:
        hps = WinGetPS(hwnd);
        WinSetPresParam(hwnd,PP_FONTNAMESIZE,(ULONG)strlen("8.Helv")+1,(void *)"8.Helv");
        GpiQueryFontMetrics(hps,(LONG)sizeof(fontmetrics),&fontmetrics);
        WinReleasePS(hps);

        memset(szFontName,0,100);

        sizfx.cx = MAKEFIXED(6,0);
        sizfx.cy = MAKEFIXED(6,0);

        /*
        ** Pre calculate the inv area for the size & pos area.
        */
        rclSp.yBottom = fontmetrics.lMaxDescender ;
        rclSp.yTop    = fontmetrics.lMaxBaselineExt + rclSp.yBottom - 2;
        rclSp.xLeft = CXBUTTON + 10;
        /*
        ** pszPos string + pszSize string is about 40 char's
        */
        rclSp.xRight = (30 * fontmetrics.lAveCharWidth ) + rclSp.xLeft;

        /* precalculate the position of the position string.*/

        pos.x = rclSp.xLeft;
        pos.y = rclSp.yBottom + fontmetrics.lMaxDescender;

        /* precalculate the position of the size string */

        size.x = rclSp.xLeft + 15 * fontmetrics.lAveCharWidth;
        size.y = rclSp.yBottom + fontmetrics.lMaxDescender;

         /*
         ** calculate the boxsize which we draw around the status stuff
         */
         ptlbox1.x = CXBUTTON + 3;      /* add a pixel */
         ptlbox1.y = CYSTATUSLINE - 2 ;
         ptlbox2.y = 2;
         ptlbox2.x = rclSp.xRight + 10;

         /*
         ** calculate the boxes for the font status & layer status
         ** using the box coords ptlbox1 & 2. This to make it easy
         ** to put back the status to the bottom.
         */
         /* fontbox */
         ptlbox3.x = ptlbox2.x + 2;      /* add a pixel */
         ptlbox3.y = ptlbox1.y;
         ptlbox4.y = ptlbox2.y;
         ptlbox4.x = ptlbox2.x + 30 * fontmetrics.lAveCharWidth;

         rclfnt.xLeft  =ptlbox3.x;
         rclfnt.xRight =ptlbox4.x;
         rclfnt.yBottom= ptlbox4.y;
         rclfnt.yTop   = ptlbox3.y;
         ptlfont.x = ptlbox3.x + 10;
         ptlfont.y = pos.y;


         /* layerbox layer:10*/
         ptlbox5.x = ptlbox4.x + 2;      /*top left add a pixel */
         ptlbox5.y = ptlbox3.y;
         ptlbox6.y = ptlbox4.y;         /* bottom right         */
         ptlbox6.x = ptlbox4.x + 10 * fontmetrics.lAveCharWidth;

         rcllay.xLeft  =ptlbox5.x;
         rcllay.xRight =ptlbox6.x;
         rcllay.yBottom= ptlbox6.y;
         rcllay.yTop   = ptlbox5.y;
         ptllay.x = ptlbox5.x + 10;
         ptllay.y = pos.y;

        /* business box */
         ptlbox7.x = ptlbox6.x + 2;      /*top left add a pixel */
         ptlbox7.y = ptlbox5.y;
         ptlbox8.y = ptlbox6.y;         /* bottom right         */
         ptlbox8.x = ptlbox6.x + 12 * fontmetrics.lAveCharWidth;
         /* invarea of the busy box.....*/
         rclbusy.xLeft  = ptlbox7.x;
         rclbusy.xRight = ptlbox8.x;
         rclbusy.yBottom= ptlbox8.y;
         rclbusy.yTop   = ptlbox7.y;
         /* So and this the point we start the busy string */
         ptlbusy.x = ptlbox7.x + 10;
         ptlbusy.y = pos.y;

         /* loading box */
         ptlbox9.x   = ptlbox8.x + 2;      /*top left add a pixel */
         ptlbox9.y   = ptlbox7.y;
         ptlbox10.y  = ptlbox8.y;         /* bottom right         */
         ptlbox10.x  = ptlbox8.x + 12 * fontmetrics.lAveCharWidth;

         /* invarea of the loading box.....*/
         rclload.xLeft  = ptlbox9.x;
         rclload.xRight = ptlbox10.x;
         rclload.yBottom= ptlbox10.y;
         rclload.yTop   = ptlbox9.y;

         /* So and this the point we start the load string */
         ptlloading.x = ptlbox9.x + 10;
         ptlloading.y = pos.y;

        /* endof status line calc.. */

        /* Init the strings to display */

        sprintf(pszPos,"x:%4d y:%4d",x1,y1);
        sprintf(pszSize,"cx:%4d cy:%4d",cx,cy);
        sprintf(pszlayr,"Layer:%2d",wi.uslayer);
        strcpy(pszbusy,"Idle");
        strcpy(pszload,"-----");
        /* Bitmap button creation. */
        createToolBar(hwnd,hwnd);
        /* Create colorbar */
        hwndColor = cbar(hwnd);
        return 0;
     case WM_MEASUREITEM:
        return MeasureItem(mp2);    /* see drwmenu.c */
     case WM_DRAWITEM:
        return DrawMenuItem(mp2);  /* see drwmenu.c  */
     case WM_MENUSELECT:
        Menuhelptext(hwndMFrame,hab,mp1);
        break;

     case WM_INITMENU:
        HwndMenu = HWNDFROMMP(mp2);
        InitMenu(&wi, mp1, mp2,fHelpEnabled,wi.pvCurrent);
        if (wi.op_mode == MULTISELECT )
        {
           EnableMenuItem(HWNDFROMMP(mp2),IDM_EXPSEL,TRUE);
           EnableMenuItem(HWNDFROMMP(mp2),IDM_GROUP,TRUE);
           EnableMenuItem(HWNDFROMMP(mp2),IDM_ALIGN,TRUE);

        }
        else
        {
           EnableMenuItem(HWNDFROMMP(mp2),IDM_EXPSEL,FALSE);
           EnableMenuItem(HWNDFROMMP(mp2),IDM_GROUP,FALSE);
           EnableMenuItem(HWNDFROMMP(mp2),IDM_ALIGN,FALSE);
        }
        return 0;
     case WM_SIZE:
        cxClient = SHORT1FROMMP (mp2);
        cyClient = SHORT2FROMMP (mp2);
        /* Color valueset */
        WinSetWindowPos(hwndColor,hwnd,
                        (CXBUTTON + 2),
                        CYSTATUSLINE, cxClient - (CXBUTTON + 2),
                        CBARHEIGHT ,SWP_SHOW | SWP_MOVE | SWP_SIZE);
        /* viewport */
        WinSetWindowPos(hwndFcanv,hwnd,
                        CXBUTTON + 2,
                        CYSTATUSLINE + CBARHEIGHT,
                        cxClient - (CXBUTTON + 2),
                        cyClient - (CYSTATUSLINE + CBARHEIGHT),
                        SWP_SHOW | SWP_MOVE | SWP_SIZE );

         posToolBar(0,(cyClient - getToolBarHeight()));
         return 0;

       case WM_CHAR:
         if (SHORT1FROMMP(mp1) & KC_KEYUP )         return 0;
         if (SHORT1FROMMP(mp1) & KC_INVALIDCHAR )   return 0;
         if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
         {
            switch (SHORT2FROMMP(mp2))
            {
               case VK_ESC:    return KeybEsc();
            }
         }
         return (MRESULT)0;

      case WM_PAINT:
         hps = WinBeginPaint(hwnd,(HPS)0,NULL);
         WinQueryWindowRect(hwnd, &rcl);
         WinFillRect(hps,&rcl,CLR_PALEGRAY);
         GpiCharStringAt(hps,&pos,(ULONG)strlen(pszPos),(PSZ)pszPos);
         GpiCharStringAt(hps,&size,(ULONG)strlen(pszSize),(PSZ)pszSize);
         if (szFontName[0])
            GpiCharStringAt(hps,&ptlfont,(ULONG)strlen(szFontName),(PSZ)szFontName);

         GpiCharStringAt(hps,&ptllay,(ULONG)strlen(pszlayr),(PSZ)pszlayr);
         GpiCharStringAt(hps,&ptlbusy,(ULONG)strlen(pszbusy),(PSZ)pszbusy);
         GpiCharStringAt(hps,&ptlloading,(ULONG)strlen(pszload),(PSZ)pszload);
           /*
           ** Start drawing a 3d border.....
           */
           Draw3dBorder(hps,ptlbox1,ptlbox2); /*coords*/
           Draw3dBorder(hps,ptlbox3,ptlbox4); /*font*/
           Draw3dBorder(hps,ptlbox5,ptlbox6); /*layer*/
           Draw3dBorder(hps,ptlbox7,ptlbox8); /*business*/
           Draw3dBorder(hps,ptlbox9,ptlbox10);/*file i/o*/
           GpiDeleteSetId (hps, 2L);
           DrawColorbox(hps);
           WinEndPaint(hps);
           return 0;

       case WM_COMMAND:
          commandToolBar((long)LOUSHORT(mp1)); /* enable some buttons */

          if (circleCommand(LOUSHORT(mp1))) /* enable some buttons */
          {        /*CIRCLES*/
             wi.pvCurrent = NULL;
             return changeMode(&wi,CIRCLEDRAW,0L);
          }
/*
** If we are in splinedraw mode open & close the segments.
*/
       if (wi.op_mode == SPLINEDRAW)
       {
          CloseSpline(&wi);
          wi.op_mode = NOSELECT;
       }
       /*
       ** FILLING PATTERNS....
       */
       if (handleMenuCommand(&wi,hwndMFrame,mp1))
       {
          if (wi.op_mode == MULTISELECT)         /* @ch3 */
          {
             changeMode(&wi,NOSELECT,0L);
             wi.pvCurrent = NULL;
          }                                      /* @ch3 */
          return (MRESULT)0;
       }
       switch(LOUSHORT(mp1))
       {
            case IDM_DUMPSPLINE:		/* tempcode */
               PolyDump(wi.pvCurrent,&wi);
               return (MRESULT)0;
/*--------------------------------PAPERCOORS & WINDOWCOORDS---------------*/
            case IDM_MM:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(wi.paper, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));

               wi.paper = IDM_MM;

               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(wi.paper, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
               return 0;

            case IDM_INCH:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(wi.paper, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));

               wi.paper = IDM_INCH;

               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(wi.paper, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));

               return 0;
/*-------------------- SUPPRESS GRADIENT FILL ON/OFF----------------------*/
            case IDM_SUPPRESSON:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_SUPPRESSOFF, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));
               wi.bSuppress = TRUE;
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_SUPPRESSON, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
               WinInvalidateRect(wi.hwndClient,(RECTL *)0,TRUE);
               return 0;

            case IDM_SUPPRESSOFF:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_SUPPRESSON, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));
               wi.bSuppress = FALSE;
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_SUPPRESSOFF, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
               WinInvalidateRect(wi.hwndClient,(RECTL *)0,TRUE);
               return 0;
/**ZOOM***/

            case IDM_ZOOM2:
            case IDM_ZOOM5:
            case IDM_ZOOM10:
            case IDM_ZOOM15:
            case IDM_ZOOM20:
            case IDM_ZOOM25:
            case IDM_ZOOM30:
               return MenuZoom(&wi,WinWindowFromID(hwndMFrame,FID_MENU),
                        LOUSHORT(mp1),TRUE);
/*--------------------------------FILE------------------------------------*/
            case IDM_FILE:
               ReadDrwfile((char *)0,&wi);
               WinInvalidateRect(wi.hwndClient,NULL,TRUE);
               return 0;
/* EXIT ....*/
            case IDM_EXIT:
               WinPostMsg(wi.hwndClient,WM_QUIT,
                         (MPARAM)0,(MPARAM)0);
               return 0;
            
/*SAVE AS ....*/
            case IDM_SAVA:
               WriteDrwfile(&wi);
               return 0;
/*SAVE */
            case IDM_SAVE:
#ifdef DEMOVERSION
              return (MRESULT)demoMessage(&wi);
#endif
               FileSave(&wi);
               return 0;
/*NEW */
            case IDM_FILN:
               if ( wi.bFileHasChanged)
               {
                  if (!FileNew(hwnd,&wi))
                     return (MRESULT)0;
               }
               wi.bFileHasChanged = FALSE;
               ObjDeleteAll();
               delTitle(hwndMFrame,&wi);
               wi.pvCurrent  = NULL;              /* Kill any reference.          */
               wi.op_mode = NOSELECT;             /* Switch to selectmode.        */
               WinInvalidateRect(wi.hwndClient,NULL,TRUE);
               return 0;

/*-------------------------------EDIT MENU--------------------------------*/
            case IDM_GROUP:
               if (wi.op_mode == MULTISELECT)
               {
                  if ((wi.pvCurrent = ObjGroup(&wi))!= NULL)
                     wi.op_mode = GROUPSELECT;
                  else
                     wi.op_mode = NOSELECT;
               }
               return (MRESULT)0;
            case IDM_UNGROUP:
               showSelectHandles(wi.pvCurrent,&wi);   /* delete old selection handles */
               ObjUnGroup(wi.pvCurrent,&wi); /* Ungroup the elements.    */
               wi.pvCurrent  = NULL;                  /* Kill any reference.    */
               changeMode(&wi,NOSELECT,0L);        /* Switch to selectmode.  */
               return (MRESULT)0;
            case IDM_PASTE:
               PasteData(hab2,&wi);
               return (MRESULT)0;
            case IDM_COPY:
               CopyData(hab2,&wi,wi.pvCurrent);
               return (MRESULT)0;
            case IDM_CUT:
               CutBitmap(hab2,(pImage)wi.pvCurrent,&wi);
               return (MRESULT)0;
            case IDM_CLEAR:
               /* Send a delete */
               WinSendMsg(wi.hwndClient,WM_CHAR,MPFROM2SHORT(KC_VIRTUALKEY,1),
                          MPFROM2SHORT(0,VK_DELETE));
               return (MRESULT)0;
/*-------------------------------Switch ouline on/off for text obj--------*/
            case IDM_NOOUTLINE:
               if (wi.pvCurrent && (wi.pvCurrent)->usClass == CLS_TXT)
               {
                  setTxtOutline(wi.pvCurrent,FALSE);
                  ObjRefresh(wi.pvCurrent,&wi);
               }
               return (MRESULT)0;
/*--------------------------------ARROWS----------------------------------*/
            case IDM_ARROWS:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjMultiLineEndChange(&wi);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               else if (wi.pvCurrent)
               {
                  if (ObjLineEndChange(wi.pvCurrent,&wi))
                     ObjRefresh(wi.pvCurrent,&wi);
                  changeMode(&wi,NOSELECT,0L);
               }
               else
                  arrowDetail(hwnd,&wi);
               return (MRESULT)0;
/*--------------------------------ZOOM------------------------------------*/
            case IDBTN_ZOOM:
               CanvZoom(&wi);
               if ( wi.usFormWidth  < wi.usWidth )
                 return MenuZoom(&wi,WinWindowFromID(hwndMFrame,FID_MENU),
                        IDM_ZOOM5,FALSE);
               else
                 return MenuZoom(&wi,WinWindowFromID(hwndMFrame,FID_MENU),
                        IDM_ZOOM10,FALSE);
/*--------------------------------ROTATION--------------------------------*/
            case IDBTN_ROTATE:
               if (wi.pvCurrent && ObjDrawRotCenter(&wi,wi.pvCurrent,TRUE))
               {
                  wi.op_mode = OBJROTATION;
                  /* remove selection handles and show rotation handle */
                  ObjRefresh(wi.pvCurrent,&wi);
                  StartupSnapRotDlg(&wi,TRUE);
               }
               return (MRESULT)0;
/*-------------------------COPY OBJECT ATTRIBUTES TO APPLICATION----------*/
            case IDM_COPYATTR:
               return copyAttributes(&wi,wi.pvCurrent);
            case IDM_COPYATTR2OBJ:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  multiPasteAttribs(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
               }
               else if (wi.pvCurrent)
               {
                  pasteAttributes(&wi,wi.pvCurrent);
                  ObjRefresh(wi.pvCurrent,&wi);
               }
               return (MRESULT)0;
/*-------------------------ALIGNMENT OPTIONS------------------------------*/
            case IDM_ALIGNHORZ:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjHorzAlign(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;
            case IDM_ALNHORZB:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjAlnHorzB(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;
            case IDM_ALNHORZT:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjAlnHorzT(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;

            case IDM_ALIGNVERT:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjVertAlign(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;
            case IDM_ALNVERTL:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjAlnVertL(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;
            case IDM_ALNVERTR:
               if (wi.op_mode == MULTISELECT)
               {
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  ObjAlnVertR(&wi);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  ObjBoundingRect(&wi,&rcl,TRUE);
                  WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                  changeMode(&wi,NOSELECT,0L);
               }
               return (MRESULT)0;

            case IDM_REFRESH:
               WinInvalidateRect(wi.hwndClient,(RECTL *)0,TRUE);
               return 0;
            case IDM_METAEXP:
               ptl.x = 0;
               ptl.y = 0;
               RecordMetaFile(hab,&wi,(RECTL *)0);
               return 0;

/*export complete drawint to PCX and other formats */
            case IDM_BITMAPEXP:
                if ( WinDlgBox(HWND_DESKTOP,wi.hwndClient,(PFNWP)Export2ImgDlgProc,
                              (HMODULE)0,IDB_SAVESEL,(VOID *)&wi) == DID_OK)
               {
                  WinSendMsg(wi.hwndClient,UM_SELECTTXT,
                             (MPARAM)0,MPFROM2SHORT(0,IDM_BITMAPEXP));
               }
               return (MRESULT)0;
/*export selection area to os/2 metafile */
            case IDM_EXPSELMETA:
               WinSendMsg(wi.hwndClient,UM_SELECTTXT,(MPARAM)0,MPFROM2SHORT(0,IDM_EXPSELMETA));
               return 0;

/*export multiple selection to an other format. PCX,GIF,TIF etc...*/

            case IDM_OTHERFORM:
               if ( WinDlgBox(HWND_DESKTOP,wi.hwndClient,(PFNWP)Export2ImgDlgProc,
                              (HMODULE)0,IDB_SAVESELIMG,NULL) == DID_OK)
               {
                  WinSendMsg(wi.hwndClient,UM_SELECTTXT,
                             (MPARAM)0,MPFROM2SHORT(0,IDM_OTHERFORM));
               }
               return (MRESULT)0;
/*------------------------CONVERT IMAGE TO GRAYSCALE ----------------*/
            case IDM_GRAYSCALE:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent && imgGrayScale(wi.pvCurrent))
                        ObjRefresh(wi.pvCurrent,&wi);
               }
               return (MRESULT)0;
/*------------------------CONVERT IMAGE TO BLACK & WHITE----------------*/
            case IDM_BLACKWHITE:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent && imgBlackWhite(wi.pvCurrent))
                        ObjRefresh(wi.pvCurrent,&wi);
               }
               return (MRESULT)0;
/*------------------------UNDO all the palette changes.-----------------*/
            case IDM_UNDOPALETTE:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent && imgUndoPalette(wi.pvCurrent) )
                        ObjRefresh(wi.pvCurrent,&wi);
               }
               return (MRESULT)0;


            case IDM_IMGINVERT:
               if (wi.op_mode == IMGSELECT && wi.pvCurrent)
               {
                  ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                  if (ImgRotateColor((pImage)wi.pvCurrent,(SHORT)'I'))
                     WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
               }
               return (MRESULT)0;
            case IDM_IMGBRIGHT:
               if (wi.op_mode == IMGSELECT && wi.pvCurrent)
               {
                  ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                  if (ImgRotateColor((pImage)wi.pvCurrent,(SHORT)'B'))
                     WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
               }
               return (MRESULT)0;

/*------------------------SAVE Image option in the image menu -----------------*/
           case IDM_IMGOTHERFORMATS:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent)
                  {
                     ImgSaveOtherFormats(wi.hwndClient,wi.pvCurrent,&wi);
                  }
               }
               return 0;
/*
** Rotate image 90 degrees.
*/
            case IDM_IMGROT90:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent)
                  {
                     ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                     if (ImgRotate(&wi,wi.pvCurrent,(int)90))
                     {
                        WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                        ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                        WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                     }
                  }
               }
               return (MRESULT)0;
/*
** Rotate image 270 degrees.
*/
            case IDM_IMGROT270:
               if (wi.op_mode == IMGSELECT)
               {
                  if (wi.pvCurrent)
                  {
                     ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                     if (ImgRotate(&wi,wi.pvCurrent,(int)270))
                     {
                        WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                        ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                        WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                     }
                  }
               }
               return (MRESULT)0;
/* Grid */
            case IDM_GRIDON:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_GRIDOFF, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));

               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_GRIDON, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));

               wi.bGrid=TRUE;
               WinInvalidateRect(wi.hwndClient,(RECTL *)0,TRUE);
               return 0;
            case IDM_GRIDOFF:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_GRIDON, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, 0));

               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_GRIDOFF, TRUE),
                          MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
               WinInvalidateRect(wi.hwndClient,(RECTL *)0,TRUE);
               wi.bGrid=FALSE;
               return 0;
            
            case IDM_GRIDSIZE:
               WinDlgBox(HWND_DESKTOP,hwnd,(PFNWP)GridSizeDlgProc,
                          (HMODULE)0,ID_GRIDSIZE,(void *)&wi);
               return (MRESULT)0;

            case IDM_SHADECLR:  /*--- SHADING ---*/
               setupShading(&wi);
               return (MRESULT)0;


/*FLIPH*/   case IDM_FLIPH:
               if (wi.op_mode == TEXTSELECT)
               {
                  wi.bFileHasChanged = TRUE;
                  TxtFlipHoriz(wi.pvCurrent, &wi);
                  ObjRefresh(wi.pvCurrent,&wi);
               }
               return 0;
            case IDM_FLIPV:
               if (wi.op_mode == TEXTSELECT)
               {
                  wi.bFileHasChanged = TRUE;
                  TxtFlipVert(wi.pvCurrent, &wi);
                  ObjRefresh(wi.pvCurrent,&wi);
               }
               return 0;
/*
** Select all Ctrl + /
*/
            case IDM_SELECTALL:
               changeMode(&wi,NOSELECT,0L);
               if (ObjSelectAll(&wi))
               {
                  changeMode(&wi,MULTISELECT,0L);
                  wi.pvCurrent  = NULL;
               }
               return (MRESULT)0;
/*
** DeSelect all Ctrl + \
*/
            case IDM_DESELECTALL: return KeybEsc();

            case IDM_PRINTAS:
               if (!PrintOpen(&wi))
                  return 0;

               WinPostMsg(wi.hwndMain, UM_READWRITE,
                          (MPARAM)IDS_PRINTING,(MPARAM)0);
               /*
               ** Tell the working thread that is should startup a print
               ** thread after it has finished the objPreparePrint stuff
               */
               WinPostQueueMsg( hmq2,UM_PRINT,(MPARAM)0,(MPARAM)0);
               return 0;

            case IDM_PRINTPREVIEW:
               {
                  ULONG ulResult = DID_CANCEL;

                  LayerConnect(&wi,IDM_PRINTPREVIEW);
                  if (wi.usWidth <  wi.usHeight )
                  {
                     /*
                     ** Show portrait print preview dialog.
                     */
                     ulResult = WinDlgBox(HWND_DESKTOP,hwnd,
                                          (PFNWP)PrintPrevDlgProc,(HMODULE)0,
                                          ID_PREV,(void *)&wi);
                  }
                  else
                  {
                     ulResult = WinDlgBox(HWND_DESKTOP,hwnd,
                                          (PFNWP)PrintPrevDlgProc,(HMODULE)0,
                                          ID_PREVL,(void *)&wi);

                  }

                  if (ulResult == DID_OK)
                  {
                     if (!PrintOpen(&wi))
                        return 0;

                     WinPostQueueMsg( hmq2,UM_PRINT,(MPARAM)0,(MPARAM)0);

                     WinPostMsg(wi.hwndMain, UM_READWRITE,
                               (MPARAM)IDS_PRINTING,(MPARAM)0);
                  }
               }
               return 0;
            case IDM_FNT:
               if ( wi.op_mode == TEXTINPUT )
               {
                  WinShowCursor(wi.hwndClient,FALSE);
                  WinDestroyCursor(wi.hwndClient);
               }
               /*
               ** Try to get fontmetrics of selected object
               ** and show this in the font dialog.
               */
               if ((wi.op_mode == TEXTSELECT || wi.op_mode == BLOCKTEXTSEL) && 
                   pObj )
               {
                  ObjInvArea(pObj,&rcl,&wi,TRUE);  /* Old rectangle     */
                  if (ObjectFontDlg(pObj,&wi))     /* Start font dialog */
                  {
                     WinInvalidateRect(wi.hwndClient,&rcl,TRUE); /* Clear */
                     ObjRefresh(pObj,&wi);                       /* redraw*/
                  }
                  return (MRESULT)0;
               }
               else
               {
                  if (!FontDlg(&wi,&wi.fattrs,FALSE))
                     return (MRESULT)0;
               }

               if (wi.op_mode == MULTISELECT)
               {
                  if (strchr(wi.fontname,'.'))
                  {
                     p = strchr(wi.fontname,'.');
                     p++; /* jump over dot */
                     ObjMultiFontChange(p);
                  }
                  changeMode(&wi,NOSELECT,0L);
                  WinInvalidateRect(hwnd,0,TRUE);
               }

               if ( wi.op_mode == TEXTINPUT )
                   Createcursor(&wi); /*drwtxtin.c*/

               WinPostMsg(wi.hwndMain, UM_FNTHASCHANGED,
                          (MPARAM)&wi.fontname,(MPARAM)0);
               return 0;
            case IDM_SELTEXT:
                 wi.bFileHasChanged = TRUE;
                 GpiSetLineType(wi.hps,LINETYPE_DOT);
                 WinSendMsg(wi.hwndClient,UM_SELECTTXT,
                            (MPARAM)0,MPFROM2SHORT(0,IDM_SELTEXT));
                 return 0;
            case IDM_PROPERTIES:
                 return showProperties(wi.hwndClient,wi.pvCurrent);
            case IDM_EDITTEXT:
                 wi.bFileHasChanged = TRUE;
                 if (wi.pvCurrent)
                    WinSendMsg(wi.hwndClient,UM_SELECTTXT,
                               (MPARAM)0,MPFROM2SHORT(0,IDM_EDITTEXT));
                 return 0;
            case IDM_INSTEXT:
               if (wi.op_mode == TEXTINPUT)
                  breakInputString(&wi); /* see drwtxtin... */
               return 0;
            case IDM_TXTCIRC:
                 wi.bFileHasChanged = TRUE;
                 if (wi.pvCurrent)
                    WinLoadDlg(HWND_DESKTOP,wi.hwndClient,
                               (PFNWP)TxtCirDlgProc,0L,ID_TXTCIRCULAR,
                               (PVOID)&wi);
                 return 0;

            case IDM_GRD:
               WinLoadDlg(HWND_DESKTOP,wi.hwndClient,(PFNWP)GradientDlgProc,
                          0L,ID_GRADIENT,(PVOID)&wi.Gradient);
               return 0;
            case IDM_FTAIN:
               WinLoadDlg(HWND_DESKTOP,hwnd,(PFNWP)FountainDlgProc,0L,
                          ID_DLGFOUNTAIN,(PVOID)&wi.fountain);
               return 0;
/*--------------------------------buttons--------------------------------*/
/*TEXT*/   case IDBTN_SNTEXT:                        /* SINGLE OBJECT */
              return changeMode(&wi,TEXTINPUT,0L);
/*BLOCKTEXT*/
           case IDBTN_BLCKTEXT:
              createBlockText(&wi);
              return 0;
/*LINES*/
             case IDBTN_LINE:
                wi.pvCurrent = NULL;
                return changeMode(&wi,LINEDRAW,0L);                /* dlg_sel.c*/
/*TRIANGLE*/
             case IDBTN_TRIANGLE:
                return changeMode(&wi,REGPOLYDRAW,0L);             /* dlg_sel.c*/
/*FREELINES*/
             case IDBTN_FLINE:
                return changeMode(&wi,SPLINEDRAW,1L);              /* dlg_sel.c*/
/*REFORMAT FORM SIZE AND LOOK ETC*/
             case IDBTN_FCHANGE:
                if (wi.pvCurrent)
                {
                   if (ObjShowMovePoints(wi.pvCurrent,&wi))
                      wi.op_mode = OBJFORMCHANGE;
                   else
                      WinAlarm(HWND_DESKTOP, WA_ERROR);
                }
                return (MRESULT)0;
/* LAST button see doc I mean online help */
             case IDBTN_CHGOBJ:
                if (wi.op_mode == MULTISELECT)
                {
                   ObjInterChange(&wi);
                   wi.op_mode = NOSELECT;
                   wi.pvCurrent = NULL;
                }
                return (MRESULT)0;
/*------------------------------------CHARBOX 1:1----------------------------*/
             case IDM_CHARBOX:
                if (wi.op_mode == TEXTSELECT && wi.pvCurrent)
                {
                   wi.bFileHasChanged = TRUE;
                   setFontSquare(wi.pvCurrent,&wi);
                }
                return 0;
/*SQUARES*/
             case IDBTN_SQUARE:
                wi.pvCurrent = NULL;
                return changeMode(&wi,SQUAREDRAW,0L);

             case IDM_BOX:
                if (wi.op_mode == TEXTINPUT)
                   WinShowCursor(wi.hwndClient,FALSE);
                wi.op_mode = BOXDRAW;
                wi.pvCurrent = NULL;
                wi.bFileHasChanged = TRUE;
                WinSetPointer(HWND_DESKTOP,hptrCrop);
                WinSetFocus(HWND_DESKTOP,wi.hwndClient);
                return 0;
/*BITMAP*/  case IDM_BMP:
                changeMode(&wi,NOSELECT,0L);
                ImgLoad(&wi,(char *)0);
                return 0;
/* --------------------------------IMPORT METAFILE --------------------------*/
            case IDM_METAFILE:
                ptl.x = 0;
                ptl.y = 0;
                pCreate = OpenMetaSegment(ptl,(char *)0,&wi,(HMF)0);
                if (pCreate)
                   ObjRefresh(pCreate,&wi);
                return 0;
             case IDM_LOTUSPIC:
                picLoad(&wi);
                return (MRESULT)0;
             case IDM_WINDOWSWMF:
                return wmfLoad(&wi);
/*FILLING*/
             case IDBTN_FILL:
                hoColorPal = WinQueryObject((PSZ)"<DRWCOLOR>");

                if (!hoColorPal)
                   hoColorPal = WinQueryObject((PSZ)"<WP_LORESCLRPAL>"); /*WARP*/
                WinSetObjectData(hoColorPal,(PSZ)"OPEN=DEFAULT");

                if (wi.op_mode == TEXTINPUT)
                   WinShowCursor(wi.hwndClient,FALSE);
                WinSetFocus(HWND_DESKTOP,wi.hwndClient);
                return 0;
/*LAYER */
             case IDBTN_LAYER:
                return layerDetail(hwnd,&wi);

/* POLYSLINES /SPLINES */
             case IDBTN_SPLINE:
                return changeMode(&wi,SPLINEDRAW,2L);
/*FONT PALLETTE */
             case IDBTN_FONT:
                hoFontPal = WinQueryObject((PSZ)"<DRWFONT>");
                if (hoFontPal)
                {
                   WinSetObjectData(hoFontPal,(PSZ)"OPEN=DEFAULT");
                   return (MRESULT)0;
                }

                hoFontPal = WinQueryObject((PSZ)"<WP_FNTPAL>");
                WinSetObjectData(hoFontPal,(PSZ)"OPEN=DEFAULT");
                if (wi.op_mode == TEXTINPUT)
                   WinShowCursor(wi.hwndClient,FALSE);
                WinSetFocus(HWND_DESKTOP,wi.hwndClient);
                return 0;
/*SELECT*/
             case IDBTN_SELECT:
                WinSendMsg(wi.hwndClient,WM_CHAR,
                           MPFROM2SHORT(KC_VIRTUALKEY,1),
                           MPFROM2SHORT(0,VK_ESC));
                return (MRESULT)0;
/*--------------------------- IMAGE FLIP HORIZONTAL -----------------------*/

             case IDM_FLIPHIMG:
                if (wi.op_mode == IMGSELECT)
                {
                   if (wi.pvCurrent)
                   {
                      wi.bFileHasChanged = TRUE;
                      FlipImgHorz(wi.pvCurrent);
                      ObjRefresh(wi.pvCurrent,&wi);
                   }
                }
               return 0;
/*--------------------------- IMAGE FLIP VERTICAL -------------------------*/
             case IDM_FLIPVIMG:
                if (wi.op_mode == IMGSELECT)
                {
                   if (wi.pvCurrent)
                   {
                      wi.bFileHasChanged = TRUE;
                      FlipImgVert(wi.pvCurrent);
                      ObjRefresh(wi.pvCurrent,&wi);
                   }
                }
               return 0;

/*--------------------- Color rotation ----------------------------------*/

            case IDM_ROTATIONUP:
                if (wi.op_mode == IMGSELECT && wi.pvCurrent)
                {
                   ImgInvArea(wi.pvCurrent,&rcl,&wi, TRUE);
                   ImgRotateColor((pImage)wi.pvCurrent,(SHORT)43);    /* '+' */
                   WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                }
                return 0;
            case IDM_ROTATIONDOWN:
                if (wi.op_mode == IMGSELECT && wi.pvCurrent)
                {
                   ImgInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                   ImgRotateColor((pImage)wi.pvCurrent,(SHORT)45);   /* '-' */
                   WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                }
                return 0;

             case IDM_CIRBMP:     /*Make bitmap circular */
                if (wi.op_mode == IMGSELECT && wi.pvCurrent)
                {
                    wi.bFileHasChanged = TRUE;
                    ImgSetCircular(wi.pvCurrent,TRUE);
                    ObjRefresh(wi.pvCurrent,&wi);
                }
               return 0;
             case IDM_RESTASPECT:
                if (wi.op_mode == IMGSELECT)
                {
                   if (wi.pvCurrent)
                   {
                      ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                      WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                      ImgRestoreAspect(wi.pvCurrent,&rcl,&wi);
                      WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                   }
                }
                return (MRESULT)0;
             case IDM_RESTORESIZE:
                if (wi.op_mode == IMGSELECT)
                {
                   if (wi.pvCurrent)
                   {
                      ObjInvArea(wi.pvCurrent,&rcl,&wi,TRUE);
                      WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                      ImgRestoreSize(wi.pvCurrent,&rcl,&wi);
                      WinInvalidateRect(wi.hwndClient,&rcl,TRUE);
                   }
                }
                return (MRESULT)0;

             case IDM_NORBMP:     /*Make bitmap Square */
                if (wi.op_mode == IMGSELECT && wi.pvCurrent)
                {
                   if (ImgIsCircular(wi.pvCurrent))
                   {
                      wi.bFileHasChanged = TRUE;
                      ImgSetCircular(wi.pvCurrent,FALSE);
                      ObjRefresh(wi.pvCurrent,&wi);
                   }
                }
                return (MRESULT)0;

             case IDM_SELBMP:        /*Details about the bitmap*/
                if (wi.op_mode == IMGSELECT)
                {
                   ImgShowPalette(wi.hwndClient,(pImage)wi.pvCurrent);
                }
                return 0;

             case IDM_FINDCOLOR:
                WinSetPointer(HWND_DESKTOP,hptrCrop);
                wi.op_mode = FINDCOLOR;
                return 0;

             case IDM_CROP:
                if (wi.op_mode == IMGSELECT)
                {
                   WinSetPointer(HWND_DESKTOP,hptrCrop);
                   wi.op_mode = IMGCROP;
                }
                return 0;
             case IDM_SETCLIPPATH:
                ObjMakeImgClipPath( &wi );
                return (MRESULT)0;
             case IDM_DELCLIPPATH:
                if (wi.op_mode == IMGSELECT && wi.pvCurrent)
                {
                   if (imgDelClipPath(wi.pvCurrent))
                      ObjRefresh(wi.pvCurrent,&wi);
                   else
                      WinAlarm(HWND_DESKTOP, WA_WARNING);
                }
                return (MRESULT)0;

/*SPECIAL LINES */
             case IDM_SPECLINES:
                return changeMode(&wi,SPECLINEDRAW,0L);
             case IDM_REGPOLYSTAR:
                return changeMode(&wi,REGPOLYSTAR,0L);
/* Freestyle lines */
             case IDM_FREESTYLEFINE:  /* Freestyle fine granularity */
                changeMode(&wi,FREESTYLE,1L);
                WinStartTimer(hab,wi.hwndClient,ID_TIMER,250);
                return 0;
             case IDM_FREESTYLENORM:  /* Freestyle normal           */
                changeMode(&wi,FREESTYLE,1L);
                WinStartTimer(hab,wi.hwndClient,ID_TIMER,500);
                return 0;
/*-------------------------------- options menu ---------------------------*/
             case IDM_FORMSIZE:
                FormDlg(&wi);      /* see drwform.c */
                return (MRESULT)0;
             case IDM_ONAREA:
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_ONLINE, TRUE),
                          MPFROM2SHORT(MIA_CHECKED,0));
               WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                          MM_SETITEMATTR,
                          MPFROM2SHORT(IDM_ONAREA, TRUE),
                          MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
                wi.bOnArea = TRUE;
                return (MRESULT)0;
             case IDM_ONLINE:
                WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                           MM_SETITEMATTR,
                           MPFROM2SHORT(IDM_ONAREA, TRUE),
                           MPFROM2SHORT(MIA_CHECKED,0));
                WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                           MM_SETITEMATTR,
                           MPFROM2SHORT(IDM_ONLINE, TRUE),
                           MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
                wi.bOnArea = FALSE;
                return (MRESULT)0;
/*-------------------------------- copy/paste palette of img---------------*/
             case IDM_COLORCOPY:
                copyPalette(&wi);
                return (MRESULT)0;
             case IDM_COLORPASTE:
                pastePalette(&wi);
                return (MRESULT)0;
/*-------------------------------- help!!----------------------------------*/
             case IDM_HELPUSINGHELP:
                HelpUsingHelp(mp2);
                return 0;
             case IDM_HELPGENERAL:
                HelpGeneral(mp2);
                return 0;
             case IDM_HELPKEYS:
                HelpKeys(mp2);
                return 0;
             case IDM_HELPINDEX:
                HelpIndex(mp2);
                return 0;
             case IDM_HELPPRODINFO:
                WinLoadDlg(HWND_DESKTOP,wi.hwndClient,(PFNWP)AboutDlgProc,
                          (HMODULE)0,11L,NULL);
                return 0;
           }
           return 0;
     }
     return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/* Name : Drawing. Returns true if we are in drawing mode, else false.    */
/*------------------------------------------------------------------------*/
BOOL Drawing()
{
   switch(wi.op_mode)
   {
      case LINEDRAW:
      case FREELINEDRAW:
      case SPECLINEDRAW:
      case CIRCLEDRAW:
      case SQUAREDRAW:
      case BOXDRAW:
      case ELLIPSEDRAW:
      case SPLINEDRAW:
      case FREESTYLE:
      case REGPOLYDRAW:
      case REGPOLYSTAR:
         return TRUE;
      default:
         return FALSE;
   }
}
/*---------------------------------------------------------------------------*/
void showFontInStatusLine( SIZEF sizfx, char *pszFacename)
{
  static char szBuf[200];
  char        szPoint[10];
  long        lPoint;

  lPoint = (long)(sizfx.cx >> 16);
  lPoint += 1; /* rounding */
  itoa ((int)lPoint,szPoint,10);
  strcpy(szBuf,szPoint);
  lPoint = (long)(sizfx.cy >> 16);
  lPoint += 1; /* rounding */
  itoa ((int)lPoint,szPoint,10);
  strcat(szBuf,"x");
  strcat(szBuf,szPoint);
  strcat(szBuf,".");
  strcat(szBuf,pszFacename);
  WinPostMsg(wi.hwndMain, UM_FNTHASCHANGED,(MPARAM)szBuf,(MPARAM)0);
  return;
}
