/*------------------------------------------------------------------------*/
/*  Name: drwview.c      [ serves the view menu ?]                        */
/*                                                                        */
/*  Description : Handles all the canvas tuff like scrolling and position */
/*                of the drawing window.                                  */
/*                All code in this module is serving the drawing area:    */
/*                Zoom,UnZoom and Grid including the necessary dialog     */
/*                procedures.                                             */
/*                                                                        */
/* Public:                                                                */
/*  DrawGrid    : Draw the grid in the drawing area.                      */
/*  GridSizeDlgProc: Serves the gridsize dialog.                          */
/*  SnapToGrid  : Snaps the mouse pointer to the grid.                    */
/*  PaperSetsize: Sets the size of our drawing window.                    */
/*  CalcPrtPosition : Calculates the poiter pos etc for in the statusline */
/*  fileGetForm : Get form dimensions from file.                          */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwcanv.h"
#include "drwutl.h"
#include "resource.h"

#define MINGRIDSIZE   10  /* gridsize here defined in 0.1 mm */
#define MAXGRIDSIZE   150 /* gridsize here defined in 0.1 mm */

#define MININCHGRIDSIZE   5   /* gridsize here defined in 0.1 inch */
#define MAXINCHGRIDSIZE   60  /* gridsize here defined in 0.1 inch */


#define MINGRIDDISP   1   /* display interval of grid */
#define MAXGRIDDISP   5


#define ZOOM_NORMAL   0 /* 100% window gets page size  */
#define ZOOM_HIGH     1 /* Page twice as big as normal */
#define ZOOM_FULLPAGE 2 /* show full page              */


static char szCanvasClass[]    = "ViewPortWindow";
static CANVAS  view;
static WINDOWINFO *pwiCanv;
static BOOL bSetScrollbars;

/*
** This function is for the zoom button.
*/
void CanvZoom(WINDOWINFO *pwi)
{
   POINTL ptl;
   LONG x,y;
   LONG cx,cy;
   HWND hwndframe;

   hwndframe = WinQueryWindow(pwi->hwndClient,QW_PARENT);
   x = 0;
   y = 0;

   if (view.iZoom)
   {
      /*
      ** Restore to default
      */
      pwi->usFormWidth   = pwi->usWidth;
      pwi->usFormHeight  = pwi->usHeight;
      view.iZoom = 0;
   }
   else
   {
      /*
      ** Full page view!
      */
      pwi->usFormWidth   = pwi->usWidth /2;
      pwi->usFormHeight  = pwi->usHeight/2;
      view.iZoom = 2;
   }

   ptl.x = pwi->usFormWidth;
   ptl.y = pwi->usFormHeight;
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,(POINTL *)&ptl);

   cx = ptl.x;
   cy = ptl.y;

   if (cy > view.cyCanvas )
      y = view.cyCanvas - cy;
   else if (view.cyCanvas > cy ) /* if possible center drawing window */
      y = (view.cyCanvas - cy)/2;

   if (view.cxCanvas > cx ) /* if possible center drawing window */
      x = (view.cxCanvas - cx)/2;
   else
      x = 0;
   /*
   ** Reset scrollbar pos..
   */
   view.sVscrollPos = 0;


   WinSetWindowPos(hwndframe,HWND_TOP,
                   x,y,
                   cx,cy,
                   SWP_SHOW | SWP_SIZE | SWP_MOVE | SWP_ZORDER );

   /*
   ** For those objects which do use the DCTL_BOUNDARY... stuff
   ** to calculate the outline/invarea. We must do it here since
   ** otherwise the WinIntersectRect call fails when the object
   ** needs to be drawn.
   */
   ObjSetInvArea(pwi);

   WinInvalidateRect(pwi->hwndClient,(RECTL *)0,TRUE);
}
/*
** Zoom with a percentage
*/
void PercentageZoom(WINDOWINFO *pwi, int iZoom)
{
   LONG x,y;
   LONG cx,cy;
   HWND hwndframe;

   hwndframe = WinQueryWindow(pwi->hwndClient,QW_PARENT);
   x = 0;
   y = 0;

   view.iZoom = iZoom;

   if (iZoom == 100)
   {
      /*
      ** Restore to default
      */
      cx = pwi->cxClient = pwi->cxClnt;
      cy = pwi->cyClient = pwi->cyClnt;
      pwi->usFormWidth   = pwi->usWidth;
      pwi->usFormHeight  = pwi->usHeight;
   }
   else if (iZoom )
   {
      ULONG ulcx; /* to avoid overflow!!! */
      ULONG ulcy;
      /*
      ** Full page view!
      */
      cy = pwi->cyClnt * iZoom;
      cx = pwi->cxClnt * iZoom;

      cy /= 100;
      cx /= 100;
      ulcx = (ULONG)pwi->usWidth;
      ulcy = (ULONG)pwi->usHeight;
      ulcx *= iZoom;
      ulcy *= iZoom;
      ulcx /= 100;
      ulcy /= 100;
      pwi->usFormWidth  = ulcx;
      pwi->usFormHeight = ulcy;
   }

   if (cy > view.cyCanvas )
      y = view.cyCanvas - cy;
   else if (view.cyCanvas > cy ) /* if possible center drawing window */
      y = (view.cyCanvas - cy)/2;

   if (view.cxCanvas > cx ) /* if possible center drawing window */
      x = (view.cxCanvas - cx)/2;
   else
      x = 0;
   /*
   ** Reset scrollbar pos..
   */
   view.sVscrollPos = 0;
   view.sHscrollPos = 0;

   WinSetWindowPos(hwndframe,HWND_TOP,
                   x,y,
                   cx,cy,
                   SWP_SHOW | SWP_SIZE | SWP_MOVE | SWP_ZORDER );

   /*
   ** For those objects which do use the DCTL_BOUNDARY... stuff
   ** to calculate the outline/invarea. We must do it here since
   ** otherwise the WinIntersectRect call fails when the object
   ** needs to be drawn.
   */
   ObjSetInvArea(pwi);

   WinInvalidateRect(pwi->hwndClient,(RECTL *)0,TRUE);
}
/*-----------------------------------------------[ private ]--------------*/
/*  Name: positionWindowInViewPort.                                       */
/*                                                                        */
/*  Description : Positions the window correctly within the viewport.     */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void positionWindowInViewPort(WINDOWINFO *pwi )
{
   HWND hFrame = WinQueryWindow(pwi->hwndClient,QW_PARENT);

   LONG x,y;

   x = y = 0;

   if (pwi->cxClient < view.cxCanvas)
   {
      x = (view.cxCanvas - pwi->cxClient)/2;
      x -= view.sHscrollPos;
   }
   else
   {
      x -= view.sHscrollPos;
   }

   if (pwi->cyClient < view.cyCanvas)
   {
      y = (view.cyCanvas - pwi->cyClient)/2;
      y += view.sVscrollPos;
   }
   else
   {
      y  = view.cyCanvas - pwi->cyClient;
      y += view.sVscrollPos;
   }
   WinSetWindowPos(hFrame,(HWND)0,x,y,0,0,SWP_MOVE);
}
/*------------------------------------------------------------------------*/
/*  Name: CanvScrollBar                                                   */
/*                                                                        */
/*  Description : Sets the scollbar position and range on a windowsize    */
/*                change. Called from the WM_SIZE of the canvas wndproc   */
/*                and the drawing area.                                   */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void CanvScrollBar(WINDOWINFO *pwi, BOOL bSetForm)
{

   if (bSetScrollbars)
      return;

   bSetScrollbars = TRUE;

   view.sVscrollMax = max (0, pwi->cyClient - view.cyCanvas);
   view.sVscrollPos = min (view.sVscrollPos, view.sVscrollMax);

   WinSendMsg (view.hwndVscroll, SBM_SETSCROLLBAR,
               MPFROM2SHORT (view.sVscrollPos,0),
               MPFROM2SHORT (0,view.sVscrollMax));

   WinSendMsg(view.hwndVscroll, SBM_SETTHUMBSIZE,
              MPFROM2SHORT(view.cyCanvas,pwi->cyClient), 0);

   view.sHscrollMax  = max ( 0, pwi->cxClient - view.cxCanvas);
   view.sHscrollPos  = min (view.sHscrollPos, view.sHscrollMax);

   WinSendMsg (view.hwndHscroll, SBM_SETSCROLLBAR,
               MPFROM2SHORT (view.sHscrollPos,0),
               MPFROM2SHORT (0,view.sHscrollMax));


   WinSendMsg(view.hwndHscroll, SBM_SETTHUMBSIZE,
              MPFROM2SHORT(view.cxCanvas,pwi->cxClient), 0);

   WinEnableWindow (view.hwndHscroll, view.sHscrollMax ? TRUE : FALSE);

   if (bSetForm )
   {
      positionWindowInViewPort(pwi);
   }
   bSetScrollbars = FALSE;
}
/*------------------------------------------------------------------------*/
/*  Name: CanvasWndproc.                                                  */
/*                                                                        */
/*  Description : Ourcanvas wnd procedure. The parent of our drawingpaper.*/
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY CanvasWndproc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   HPS   hps;
   RECTL rcl;
   SHORT  sVscrollInc;               /* Vertical scrollbar incr val    */
   SHORT  sHscrollInc;               /* Horizontal scrollbar incr val  */


   switch (msg)
   {
      case WM_CREATE:
         view.hwndVscroll = WinWindowFromID(WinQueryWindow(hwnd, QW_PARENT),
                                                      FID_VERTSCROLL);
         view.hwndHscroll = WinWindowFromID(WinQueryWindow(hwnd, QW_PARENT),
                                                      FID_HORZSCROLL);
         view.iZoom = ZOOM_NORMAL;
         view.hwndClient = hwnd;
         return 0;

      case WM_SIZE:
         view.cxCanvas = (LONG)SHORT1FROMMP (mp2);
         view.cyCanvas = (LONG)SHORT2FROMMP (mp2);
         CanvScrollBar(pwiCanv, TRUE);
         return 0;

      case WM_CHAR:
         if (SHORT1FROMMP(mp1) & KC_KEYUP )
            return 0;
         if (SHORT1FROMMP(mp1) & KC_INVALIDCHAR )
            return 0;
         if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
         {
             switch (SHORT2FROMMP(mp2))
             {
                case VK_ESC: return KeybEsc(); /* see drwmain */
                case VK_PAGEUP:
                case VK_PAGEDOWN:
                   return WinSendMsg(view.hwndVscroll,msg,mp1,mp2);

             }
          }
          return (MRESULT)0;

      case WM_PAINT:
         hps = WinBeginPaint(hwnd,(HPS)0,NULL);
         WinQueryWindowRect(hwnd, &rcl);
         WinFillRect(hps,&rcl,CLR_DARKGRAY);
         WinEndPaint(hps);
         return 0;

/*========================Vertical=Scrolling================================*/
      case WM_VSCROLL:
         switch (SHORT2FROMMP (mp2))
         {
            case SB_LINEUP:
               sVscrollInc = -pwiCanv->fattrs.lMaxBaselineExt;
               break;
            case SB_LINEDOWN:
               sVscrollInc = pwiCanv->fattrs.lMaxBaselineExt;
               break;
            case SB_PAGEUP:
               sVscrollInc = -view.cyCanvas;
               break;
            case SB_PAGEDOWN:
               sVscrollInc = view.cyCanvas;
               break;
            case SB_SLIDERPOSITION:
               sVscrollInc = SHORT1FROMMP (mp2) - view.sVscrollPos;
               break;
            default:
               sVscrollInc = 0;
               break;
         }
         sVscrollInc = max(-view.sVscrollPos,
                           min(sVscrollInc,view.sVscrollMax - view.sVscrollPos));
         if ( sVscrollInc != 0 )
         {
            view.sVscrollPos += sVscrollInc;
            WinScrollWindow (hwnd,0,sVscrollInc,(PRECTL)NULL,
                             (PRECTL)NULL,(HRGN)NULLHANDLE,
                             (PRECTL)NULL,SW_INVALIDATERGN | SW_SCROLLCHILDREN );

            WinSendMsg (view.hwndVscroll, SBM_SETPOS,
                         MPFROMSHORT (view.sVscrollPos), NULL);

            WinUpdateWindow (hwnd);
         }
         return 0;
/*========================Horizontal=Scrolling==============================*/
    case WM_HSCROLL:
         switch(SHORT2FROMMP (mp2))
         {
            case SB_LINELEFT:
               sHscrollInc = -pwiCanv->fattrs.lAveCharWidth;
               break;
            case SB_LINERIGHT:
               sHscrollInc = pwiCanv->fattrs.lAveCharWidth;
               break;
            case SB_PAGELEFT:
               sHscrollInc = -view.cxCanvas;
               break;
            case SB_PAGERIGHT:
               sHscrollInc = view.cxCanvas;
               break;
            case SB_SLIDERPOSITION:
               sHscrollInc = SHORT1FROMMP (mp2) - view.sHscrollPos;
               break;
            default:
               sHscrollInc = 0;
               break;
         }
         sHscrollInc = max ( -view.sHscrollPos,
                       min (sHscrollInc, view.sHscrollMax - view.sHscrollPos));

         if ( sHscrollInc !=0)
         {
            view.sHscrollPos += sHscrollInc;
            WinScrollWindow(hwnd,-sHscrollInc,0L,
                            (PRECTL)NULL,(PRECTL)NULL,
                            (HRGN)NULLHANDLE,(PRECTL)NULL,
                            SW_INVALIDATERGN | SW_SCROLLCHILDREN);

            WinSendMsg (view.hwndHscroll, SBM_SETPOS,
                        MPFROMSHORT (view.sHscrollPos), NULL);
            WinUpdateWindow (hwnd);
         }
         return 0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/*  Regview.                                                              */
/*                                                                        */
/*  Description : Register the windowprocedures.                          */
/*------------------------------------------------------------------------*/
void RegCanv(HAB hab, WINDOWINFO *pwi)
{
   WinRegisterClass (hab,(PSZ)szCanvasClass,(PFNWP)CanvasWndproc,
                     0L,0L);

   pwiCanv = pwi; /* Get a ref.. */
}


HWND CreateCanvas(HWND hParent, HWND *hwnd )
{

  static ULONG flFrameFlags = FCF_BORDER |
                              FCF_HORZSCROLL | FCF_VERTSCROLL;

  return WinCreateStdWindow (hParent,
                             WS_CLIPCHILDREN,&flFrameFlags,
                             (PSZ)szCanvasClass,NULL,
                             0L,(HMODULE)0,0L,
                             hwnd);
}
/*------------------------------------------------------------------------*/
/* DrawGrid..                                                             */
/*------------------------------------------------------------------------*/
VOID DrawGrid(WINDOWINFO *pwi, RECTL *rcl)
{
   POINTL ptl;
   LONG   lForeColor;
   LONG   lBackColor;
   ULONG  ulcx = pwi->ulgridcx;
   ULONG  ulcy = pwi->ulgridcy;


   lForeColor = GpiQueryColor(pwi->hps);
   lBackColor = GpiQueryBackColor(pwi->hps);

   if (view.iZoom == 2)
   {
      ulcx /=2;
      ulcy /=2;
   }
   else if (view.iZoom && view.iZoom != 2)
   {
      ulcx *= view.iZoom;
      ulcy *= view.iZoom;
      ulcx /= 100;
      ulcy /= 100;
   }
   GpiSetColor(pwi->hps,lBackColor);
   GpiSetMix(pwi->hps,FM_INVERT);

   for (ptl.x= rcl->xLeft; ptl.x <  rcl->xRight; ptl.x += (ulcx * pwi->ulgriddisp))
      for (ptl.y=rcl->yBottom; ptl.y <  rcl->yTop; ptl.y += (ulcy * pwi->ulgriddisp))
      {
         GpiMove(pwi->hps,&ptl);
         GpiLine(pwi->hps,&ptl);
      }
   GpiSetColor(pwi->hps,lForeColor);
}
/*------------------------------------------------------------------------*/
/*  GridSizeDlgProc.                                                      */
/*                                                                        */
/*  Description : Serves the dialog which shows three spinbuttons.        */
/*                1: Gidsize in the x - direction.                        */
/*                2: Gidsize in the y - direction.                        */
/*                3: Display interval of the grid.                        */
/*                Gets at the initialisation a pointer to the             */
/*                WINDOWINFO structure and used this to save the val's    */
/*                after pressing the OK button.                           */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY GridSizeDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static WINDOWINFO *pwi;
   ULONG  ulStorage[2];    /* To get the vals out of the spins  */
   PVOID  pStorage;        /* idem spinbutton.                  */

   SWP   swp;

   switch(msg)
   {
      case WM_INITDLG:
         pwi  = (WINDOWINFO *)mp2;
         /* Centre dialog on the screen        */

         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
                       ((WinQuerySysValue(HWND_DESKTOP,        SV_CXSCREEN) - swp.cx) / 2),
                       ((WinQuerySysValue(HWND_DESKTOP,        SV_CYSCREEN) - swp.cy) / 2),
                       0, 0, SWP_MOVE);

         /*
         ** Display interval spinbutton.
         */
         if (pwi->paper == IDM_MM)
         {
            /*
            ** 0.1 mm precision.
            */
            WinSendDlgItemMsg( hwnd, ID_GRIDSPIN, SPBM_SETLIMITS,
                               MPFROMLONG(MAXGRIDDISP),MPFROMLONG(MINGRIDDISP));

            WinSendDlgItemMsg( hwnd, ID_GRIDSPIN, SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)pwi->ulgriddisp), NULL);
            /*
            ** Gridsize in x direction.
            */
            WinSendDlgItemMsg( hwnd, ID_SPINGRIDX, SPBM_SETLIMITS,
                               MPFROMLONG(MAXGRIDSIZE),MPFROMLONG(MINGRIDSIZE));

            WinSendDlgItemMsg( hwnd, ID_SPINGRIDX,SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)pwi->ulgridcx), NULL);
            /*
            ** Gridsize in y direction.
            */
            WinSendDlgItemMsg( hwnd, ID_SPINGRIDY, SPBM_SETLIMITS,
                               MPFROMLONG(MAXGRIDSIZE),MPFROMLONG(MINGRIDSIZE));

            WinSendDlgItemMsg( hwnd, ID_SPINGRIDY,SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)pwi->ulgridcy), NULL);

         }
         else
         {
            LONG Inch;
            /*
            ** 0.1 inch precision.
            */
            WinSetDlgItemText (hwnd,ID_GRPGRID,"Value's * 0.1 Inch");

            WinSendDlgItemMsg( hwnd, ID_GRIDSPIN, SPBM_SETLIMITS,MPFROMLONG(MAXGRIDDISP),
                               MPFROMLONG(MINGRIDDISP));

            Inch = pwi->ulgriddisp / 2.54;

            WinSendDlgItemMsg( hwnd, ID_GRIDSPIN, SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)Inch), NULL);
            /*
            ** Gridsize in x direction.
            */
            WinSendDlgItemMsg( hwnd, ID_SPINGRIDX, SPBM_SETLIMITS,
                               MPFROMLONG(MAXINCHGRIDSIZE),
                               MPFROMLONG(MININCHGRIDSIZE));

            Inch = pwi->ulgridcx / 2.54;
            WinSendDlgItemMsg( hwnd, ID_SPINGRIDX,SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)Inch), NULL);
            /*
            ** Gridsize in y direction.
            */
            WinSendDlgItemMsg( hwnd, ID_SPINGRIDY, SPBM_SETLIMITS,
                               MPFROMLONG(MAXINCHGRIDSIZE),
                               MPFROMLONG(MININCHGRIDSIZE));

            Inch = pwi->ulgridcy / 2.54;

            WinSendDlgItemMsg( hwnd, ID_SPINGRIDY,SPBM_SETCURRENTVALUE,
                               MPFROMLONG((LONG)Inch), NULL);
         }
         return (MRESULT)0;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               pStorage = (PVOID)ulStorage;
               /* display interval */

               WinSendDlgItemMsg(hwnd,ID_GRIDSPIN,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));
               if (ulStorage[0] >= MINGRIDDISP && ulStorage[0] <= MAXGRIDDISP )
               {
                  if (pwi->paper == IDM_MM)
                     pwi->ulgriddisp = ulStorage[0];
                  else
                     pwi->ulgriddisp = ulStorage[0] * 2.54; /*back to 0.1 mm */
               }
               /* x - direction */
               WinSendDlgItemMsg(hwnd,ID_SPINGRIDX,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));
               if (ulStorage[0] >= MINGRIDSIZE && ulStorage[0] <= MAXGRIDSIZE )
               {
                  if (pwi->paper == IDM_MM)
                     pwi->ulgridcx = ulStorage[0];
                  else
                     pwi->ulgridcx = ulStorage[0] * 2.54;
               }
               /* y - direction */
               WinSendDlgItemMsg(hwnd,ID_SPINGRIDY,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));
               if (ulStorage[0] >= MINGRIDSIZE && ulStorage[0] <= MAXGRIDSIZE )
               {
                  if (pwi->paper == IDM_MM)
                     pwi->ulgridcy = ulStorage[0];
                  else
                     pwi->ulgridcy = ulStorage[0] * 2.54;
               }
               WinPostMsg(pwi->hwndClient,UM_ENDDIALOG,(MPARAM)0,(MPARAM)0);               WinDismissDlg(hwnd,DID_OK);
               break;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               break;
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  Snap2Grid                                                             */
/*                                                                        */
/*  Description : Snaps the mouse position to the grid.                   */
/*                of objects.                                             */
/*                                                                        */
/*------------------------------------------------------------------------*/
void Snap2Grid(WINDOWINFO *pwi, PPOINTL pptl)
{
   LONG s;

   s = pptl->x % pwi->ulgridcx;
   pptl->x -= s;
   s = pptl->y % pwi->ulgridcy;
   pptl->y -= s;
   return;
}
/*------------------------------------------------------------------------*/
/*  Name: CanvSetSize.  [ called from drwform.c ]                         */
/*                                                                        */
/*  Description  : Called whenever the user selects another form size.    */
/*------------------------------------------------------------------------*/
void PaperSetSize(WINDOWINFO *pwi,USHORT usWidth, USHORT usHeight)
{
   HWND   hwndframe;
   HWND   hParent;
   POINTL ptl;
   RECTL  rcl;
   LONG   x,y,cx,cy;

   hwndframe = WinQueryWindow(pwi->hwndClient,QW_PARENT);
   hParent   = WinQueryWindow(hwndframe,QW_PARENT); /* canvas */

   WinQueryWindowRect(hParent,&rcl);

   view.iZoom = 0;

   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;
   pwi->usWidth       = usWidth;
   pwi->usHeight      = usHeight;

   ptl.x = usWidth;
   ptl.y = usHeight;
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,(POINTL *)&ptl);

   cx = ptl.x;
   cy = ptl.y;
   pwi->cxClient = cx;
   pwi->cyClient = cy;
   pwi->cxClnt   = cx;
   pwi->cyClnt   = cy;

   /*
   ** If paperwidth is smaller than the window wherein it is drawn
   ** than try to center it in the x-direction.
   ** Else simply begin at zero.
   */
   if (cx < rcl.xRight)
   {
      x = (rcl.xRight - cx)/2;
   }
   else
      x = 0;

   if (cy < rcl.yTop)
   {
      y = (rcl.yTop - cy)/2;
   }
   else
   {
      y = rcl.yTop - cy;
   }
   view.sVscrollPos = 0;
   view.sHscrollPos = 0;
   CanvScrollBar(pwi,FALSE);
   WinSetWindowPos(hwndframe,(HWND)0,x,y,cx,cy,SWP_MOVE | SWP_SIZE);

   return;
}
/*------------------------------------------------------------------------*/
/*  CalcPtrPosition.                                                      */
/*                                                                        */
/*  Description : Calculates the form position.                           */
/*                                                                        */
/*                                                                        */
/*  Parameters  : Mouse postion in 0.1 mm.                                */
/*                or object size in 0.1 mm.                               */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void CalcPtrPosition(WINDOWINFO *pwi, PPOINTL ptl)
{
   if (view.iZoom == 2)
   {
      ptl->x *= 2;
      ptl->y *= 2;
   }
   else if (view.iZoom && view.iZoom!= 2)
   {
      ptl->x *= 100;
      ptl->y *= 100;

      ptl->x /=  view.iZoom;
      ptl->y /=  view.iZoom;
   }
   if (pwi->paper == IDM_INCH)
   {
      ptl->x /= 2.54;
      ptl->y /= 2.54;
   }
}
/*------------------------------------------------------------------------*/
/*  fileGetForm.                                                          */
/*                                                                        */
/*  Description : Load the form dimensions from the header of the file.   */
/*                If there are already objects on the screen which are    */
/*                drawn on a different format the program will warn the   */
/*                user and if the user agrees the objects from the file   */
/*                will be put on the paper as it is. If there are no      */
/*                objects drawn the program will change the form size and */
/*                continue the loading of the file.                       */
/*                                                                        */
/*  Parameters  : pLoadinfo pli - pointer to the loadinfo struct.         */
/*                with a valid filepointer.                               */
/*                WINDOWINFO    - pointer to the windowinfo.              */
/*                BOOL bDrawing - Is there a drawing active at this       */
/*                moment, e.g are there objects based on the current form */
/*                                                                        */
/* Returns      : BOOL TRUE = Continue loading objects. FALSE = STOP      */
/*------------------------------------------------------------------------*/
int fileGetForm(pLoadinfo pli, WINDOWINFO *pwi,BOOL bDrawing)
{
   FORMINFO fi;
   char   buf[350];
   USHORT usRes;
   int i;

   i = read(pli->handle,(PVOID)&fi,sizeof(FORMINFO));

   if (i < 0)
      return FALSE;

   if (bDrawing)
   {
      if (fi.cxForm != pwi->usWidth || fi.cyForm != pwi->usHeight)
      {
          sprintf(buf,"%s\n%s\n%s\n%s\n",
                  "Form size differs from current",
                  "form size",
                  "Objects will appear different",
                  "Continue loading?");


          usRes = (USHORT)WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                                        buf,
                                        "Different form size",
                                        0,
                                        MB_YESNO | MB_APPLMODAL | MB_MOVEABLE |
                                        MB_ICONEXCLAMATION);
         if (usRes == MBID_YES)
            return TRUE;
      }
      else
         return TRUE;
   }
   else
   {
      /*
      ** Nothing drawn so update the wininfo structure.
      */
      PaperSetSize(pwi,fi.cxForm,fi.cyForm);
      return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/*  fileWrtForm.                                                          */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void fileWrtForm(pLoadinfo pli, WINDOWINFO *pwi)
{
   FORMINFO fi;

   fi.cb = sizeof(FORMINFO);
   fi.cxForm = pwi->usWidth;
   fi.cyForm = pwi->usHeight;

   _write(pli->handle,(PVOID)&fi,sizeof(FORMINFO));
}

void checkScrollbars(WINDOWINFO *pwi, POINTL ptl)
{
   RECTL rcl;

   WinQueryWindowRect(view.hwndClient,&rcl);

   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptl);
   WinMapWindowPoints(pwi->hwndClient,view.hwndClient,&ptl,1);

   if (ptl.y < 0)
      WinSendMsg(view.hwndClient,WM_VSCROLL,(MPARAM)0,
                 MPFROM2SHORT(0,SB_LINEDOWN));
   if (ptl.x > rcl.xRight)
      WinSendMsg(view.hwndClient,WM_HSCROLL,(MPARAM)0,
                 MPFROM2SHORT(0,SB_LINERIGHT));
}

