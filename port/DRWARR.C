/*------------------------------------------------------------------------*/
/*  Name: drwarr.c                                                        */
/*                                                                        */
/*  Description : Functions for handling arrow on lines ends.             */
/*                                                                        */
/* Private functions:  [ line end functions ]                             */
/*   arrowRight        : Draws a right arrow closed or open.              */
/*   arrowLeft         : Draws a left  arrow closed or open.              */
/*   circle            : Draws a circle closed or open.                   */
/*   drawLine          : Draws a normal line end in the dialog window.    */
/*   drawDiamond       : Draws a diamond shape at the line end.           */
/*   drawDiamondFilled : Draws a filled diamond and uses drawDiamond.     */
/*   drawArrowOpenRight: Draws an open right arrow. Uses arrowRight.      */
/*   drawArrowRight    : Draws an open right arrow Uses arrowRight.       */
/*   drawArrowFilledRight: Draws a filled/closed right arrow. arrowRight  */
/*   drawArrowLeft     : Draws an open left arrow. Uses arrowLeft         */
/*   drawArrowFilledLeft: Draws a closed filled left arrow. Uses arrowLeft*/
/*   drawArrowOpenLeft : Draws a non filled closed left arrow. arrowLeft  */
/*   drawOpenCircle    : Draws an open circle as line end. Uses circle.   */
/*   drawCircle        : Draws a filled circle. Uses circle.              */
/*   angleFromPoints   : Used to get the drawing angle for drawing the    */
/*                       line ends.                                       */
/*                                                                        */
/* Public functions    [ line end functions ]                             */
/*   ArrWndProc        : Window procedure to handle line end previed wnd. */
/*   ArrowDlgProc      : Dialog procedure for handling the arrow def.     */
/*   RegisterArrow     : Register the arrow preview window.               */
/*   drwEndPoints      : Draws the end points based on the arrow struct...*/
/*   lineSetLineEnd    : Sets the line end or changes it after selecting  */
/*                       the line end menu item while the dialog runs.    */
/*   arrowDetail       : Starts the arrow dialog window.                  */
/*   drwStartPt        : Draws only the start of the line. Used by polylns*/
/*   drwEndPt          : Draws only the end of a line. Used by polylns    */
/*   arrowAreaExtra    : Adds the arrow size to the given rectangle.      */
/*------------------------------------------------------------------------*/

#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_lin.h"
#include "dlg_hlp.h"
#include "resource.h"
#include "drwutl.h"
#include "dlg_sqr.h"
#define LINESELSIZE     15

#define CONVERT_FACTOR  1.5   /* Constante of Jasper..... */
#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#define MARGIN         4
#define MAX_ARROWPROCS 11 /* Max number of functions in drawArray */

typedef long ( * drawproc )(HPS,RECTL);

static ARROW arrow;   /* Local copy of original */
static BOOL  bDialogActive;
/*---------------------------------------------------------------------------*/
static long arrowRight(HPS hps, RECTL rcl, BOOL bClose )
{
   POINTL    ptl;
   /*
   ** Arrow point at the right
   */
   ptl.x = (rcl.xLeft + ( rcl.xRight - rcl.xLeft )) - MARGIN;
   ptl.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiMove(hps,&ptl);
   /*
   ** Move up to upper Left corner
   */
   ptl.x -= (rcl.xRight - rcl.xLeft);
   ptl.x += MARGIN * 2;
   ptl.y += (rcl.yTop - rcl.yBottom)/2;
   ptl.y -= MARGIN;
   GpiLine(hps,&ptl);
   /*
   ** Go down to lower left corner.
   */
   ptl.y = rcl.yBottom + MARGIN;
   if (bClose)
      GpiLine(hps,&ptl);
   else
      GpiMove(hps,&ptl);
   /*
   ** Back to point
   */
   ptl.x = (rcl.xLeft + ( rcl.xRight - rcl.xLeft )) - MARGIN;
   ptl.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiLine(hps,&ptl);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long arrowLeft(HPS hps, RECTL rcl, BOOL bClose )
{
   POINTL    ptl;
   /*
   ** Arrow point at the left
   */
   ptl.x = rcl.xLeft + MARGIN;
   ptl.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiMove(hps,&ptl);
   /*
   ** Move up to upper right corner
   */
   ptl.x += (rcl.xRight - rcl.xLeft);
   ptl.x -= MARGIN * 2;
   ptl.y += (rcl.yTop - rcl.yBottom)/2;
   ptl.y -= MARGIN;
   GpiLine(hps,&ptl);
   /*
   ** Go down to lower right corner.
   */
   ptl.y = rcl.yBottom + MARGIN;
   if (bClose)
      GpiLine(hps,&ptl);
   else
      GpiMove(hps,&ptl);
   /*
   ** Back to point
   */
   ptl.x = rcl.xLeft + MARGIN;
   ptl.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiLine(hps,&ptl);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long circle(HPS hps, RECTL rcl, LONG lControl)
{
   ARCPARAMS arcp;
   POINTL    ptlCenter;

   arcp.lR = 0L;
   arcp.lS = 0L;

   arcp.lP = (rcl.xRight - rcl.xLeft)/2;
   arcp.lQ = (rcl.yTop - rcl.yBottom)/2;

   arcp.lP -= MARGIN;
   arcp.lQ -= MARGIN;

   if (arcp.lQ < arcp.lP)
     arcp.lP = arcp.lQ;
   else
     arcp.lQ = arcp.lP;

   ptlCenter.x = rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
   ptlCenter.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiSetCurrentPosition(hps,&ptlCenter);
   GpiSetArcParams(hps,&arcp);
   GpiFullArc(hps,lControl,MAKEFIXED(1,0));
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawLine(HPS hps, RECTL rcl )
{
   POINTL ptl;

   ptl.x = rcl.xLeft + MARGIN;
   ptl.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiMove(hps,&ptl);
   ptl.x = rcl.xRight - MARGIN;
   GpiLine(hps,&ptl);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawDiamond(HPS hps, RECTL rcl)
{
   POINTL ptl;
   POINTL ptlMid;

   ptlMid.x = rcl.xLeft   + MARGIN;
   ptlMid.y = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   GpiMove(hps,&ptlMid);
   /*
   ** Go to top of diamond. Mid top...
   */
   ptl.x = rcl.xLeft + ((rcl.xRight - rcl.xLeft )/2);
   ptl.y = rcl.yTop - ( MARGIN + 2);
   GpiLine(hps,&ptl);
   /*
   ** Goto mid right
   */
   ptl.x = rcl.xRight - MARGIN;
   ptl.y = ptlMid.y;
   GpiLine(hps,&ptl);
   /*
   ** Goto mid bottom
   */
   ptl.x = rcl.xLeft + ((rcl.xRight - rcl.xLeft )/2);
   ptl.y = rcl.yBottom + ( MARGIN + 2);
   GpiLine(hps,&ptl);
   /*
   ** Back to start
   */
   GpiLine(hps,&ptlMid);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawDiamondFilled(HPS hps, RECTL rcl )
{
   GpiSetPattern(hps, PATSYM_SOLID);
   GpiBeginArea(hps, BA_NOBOUNDARY | BA_ALTERNATE);
   drawDiamond(hps,rcl);
   GpiCloseFigure(hps);
   GpiEndArea(hps);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawArrowOpenRight(HPS hps, RECTL rcl )
{
   return arrowRight(hps,rcl,FALSE);
}
/*------------------------------------------------------------------------*/
static long drawArrowRight(HPS hps, RECTL rcl )
{
   return arrowRight(hps,rcl,TRUE);
}
/*------------------------------------------------------------------------*/
static long drawArrowFilledRight(HPS hps, RECTL rcl )
{
   GpiSetPattern(hps, PATSYM_SOLID);
   GpiBeginArea(hps, BA_NOBOUNDARY | BA_ALTERNATE);
   arrowRight(hps,rcl,TRUE);
   GpiCloseFigure(hps);
   GpiEndArea(hps);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawArrowLeft(HPS hps, RECTL rcl )
{
   return arrowLeft(hps,rcl,TRUE);
}
/*------------------------------------------------------------------------*/
static long drawArrowFilledLeft(HPS hps, RECTL rcl )
{
   GpiSetPattern(hps, PATSYM_SOLID);
   GpiBeginArea(hps,BA_NOBOUNDARY | BA_ALTERNATE);
   arrowLeft(hps,rcl,TRUE);
   GpiCloseFigure(hps);
   GpiEndArea(hps);
   return 0L;
}
/*------------------------------------------------------------------------*/
static long drawArrowOpenLeft(HPS hps, RECTL rcl )
{
   return arrowLeft(hps,rcl,FALSE);
}
/*------------------------------------------------------------------------*/
static long drawOpenCircle(HPS hps, RECTL rcl )
{
   return circle(hps,rcl,DRO_OUTLINE);
}
/*------------------------------------------------------------------------*/
static long drawCircle(HPS hps, RECTL rcl )
{
   return circle(hps,rcl,DRO_FILL);
}
/*------------------------------------------------------------------------*/
static drawproc drawArray[] = { drawLine, drawCircle, drawOpenCircle, 
                                drawArrowLeft,drawArrowOpenLeft,
                                drawArrowFilledLeft,drawArrowRight,
                                drawArrowOpenRight,
                                drawArrowFilledRight,drawDiamond,
                                drawDiamondFilled,drawCircle };
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ArrWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   HPS   hps;
   RECTL rcl;
   RECTL rclArrow,rclPix;
   SIZEL sizel;
   POINTL ptl;

   switch(msg)
   {
      case WM_PAINT:
         WinQueryWindowRect(hwnd,&rcl);
         rclPix = rcl;
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiErase(hps);

         sizel.cx = 0;
         sizel.cy = 0;
         GpiSetPS(hps,&sizel,PU_LOMETRIC | GPIF_DEFAULT);
         
         GpiConvert(hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,2,(POINTL *)&rcl);

         /*
         ** Draw traight line from left to right
         */
         ptl.x = arrow.lSize + MARGIN;
         ptl.y = (rcl.yTop - rcl.yBottom)/2;
         GpiMove(hps,&ptl);
         ptl.x = rcl.xRight - ( arrow.lSize + MARGIN);
         GpiLine(hps,&ptl);
         /*
         ** Add starting arrow
         */
         rclArrow.xLeft   = MARGIN;
         rclArrow.xRight  = arrow.lSize + MARGIN;
         rclArrow.yBottom = ((rcl.yTop - rcl.yBottom) - arrow.lSize)/2;
         rclArrow.yTop    = rcl.yTop - ((rcl.yTop - rcl.yBottom) - arrow.lSize)/2;
         drawArray[arrow.lStart-1](hps,rclArrow);
         /*
         ** Add ending arrow
         */
         rclArrow.xLeft   = rcl.xRight - ( arrow.lSize + MARGIN );
         rclArrow.xRight  = rcl.xRight - MARGIN;
         rclArrow.yBottom = ((rcl.yTop - rcl.yBottom) - arrow.lSize)/2;
         rclArrow.yTop    = rcl.yTop - ((rcl.yTop - rcl.yBottom) - arrow.lSize)/2;
         drawArray[arrow.lEnd-1](hps,rclArrow);
         /*
         ** 3D border around the window.
         */
         WinDrawBorder(hps, &rclPix,2L,2L,0L, 0L, DB_DEPRESSED);
         WinEndPaint(hps);
         return 0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ArrowDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   static     ARROW *pArrow; /* reference to caller    */
   static     BOOL  bInit;
   static     HWND  hArrow;
   SWP        swp;
   int        idxRow;
   POWNERITEM pOwner;
   USHORT     usRow;

   switch(msg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         if (mp2)
         {
            /*
            ** Make local copy and remember reference to copy stuff
            ** back on OK.
            */
            pArrow = (ARROW *)mp2;
            memcpy(&arrow,pArrow,sizeof(ARROW));
         }
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx)),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);
         /*
         ** Initialize our spinbutton, min and max arrow size
         */
         WinSendDlgItemMsg(hwnd,ID_SPINARROWSIZE, SPBM_SETLIMITS,
                           MPFROMLONG(MAX_ARROWSIZE), 
                           MPFROMLONG(MIN_ARROWSIZE));

         WinSendDlgItemMsg(hwnd,ID_SPINARROWSIZE,SPBM_SETCURRENTVALUE,
                           MPFROMLONG(arrow.lSize), NULL);
         /*
         ** Tell both valuesets that we want to draw the contents of the
         ** items.
         */
         for ( idxRow = 1; idxRow <= MAX_ARROWPROCS; idxRow++)
         {
            WinSendDlgItemMsg(hwnd,ID_LINESTART,VM_SETITEMATTR,
                              MPFROM2SHORT(idxRow,1),MPFROM2SHORT(VIA_OWNERDRAW,TRUE));
            WinSendDlgItemMsg(hwnd,ID_LINEEND,VM_SETITEMATTR,
                              MPFROM2SHORT(idxRow,1),MPFROM2SHORT(VIA_OWNERDRAW,TRUE));
         }
         /*
         ** Setup selection in value sets.
         */
         WinSendDlgItemMsg(hwnd,ID_LINESTART,
                           VM_SELECTITEM,MPFROM2SHORT(arrow.lStart,1),
                           NULL);
         WinSendDlgItemMsg(hwnd,ID_LINEEND,
                           VM_SELECTITEM,MPFROM2SHORT(arrow.lEnd,1),
                           NULL);
         hArrow = WinWindowFromID(hwnd, ID_ARROWWND);
         bInit = FALSE;     
         return (MRESULT)0;

      case WM_DRAWITEM:
         pOwner = (POWNERITEM)mp2;
         WinFillRect(pOwner->hps,&pOwner->rclItem,CLR_WHITE);
         if (pOwner->idItem == VDA_ITEM)
         {
            GpiSetColor(pOwner->hps,CLR_BLACK);
            usRow = SHORT1FROMMP((MPARAM)pOwner->hItem);
            if (usRow-1 < MAX_ARROWPROCS )
               drawArray[usRow-1](pOwner->hps,pOwner->rclItem);
            return (MPARAM)TRUE; /* Done, so don't do the default stuff...*/
         }
         break;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_HELP:
               ShowDlgHelp(hwnd);
               return 0;
            case DID_CANCEL:
               WinDismissDlg( hwnd, DID_OK);
               bDialogActive = FALSE;
               break;
            case DID_APPLY:
               WinSendDlgItemMsg(hwnd,ID_SPINARROWSIZE,SPBM_QUERYVALUE,
                                 (MPARAM)(&arrow.lSize),MPFROM2SHORT(0,0));

               pArrow->lStart = arrow.lStart;
               pArrow->lEnd   = arrow.lEnd;
               pArrow->lSize  = arrow.lSize;
	       break;
	    case DID_DEFAULT:
               arrow.lStart     = DEF_LINESTART;
               arrow.lEnd       = DEF_LINEEND;
               arrow.lSize      = DEF_ARROWSIZE;
               pArrow->lStart   = DEF_LINESTART;
               pArrow->lEnd     = DEF_LINEEND;
               pArrow->lSize    = DEF_ARROWSIZE;
               WinSendDlgItemMsg(hwnd,ID_SPINARROWSIZE,SPBM_SETCURRENTVALUE,
                                 MPFROMLONG(arrow.lSize), NULL);
               WinSendDlgItemMsg(hwnd,ID_LINESTART,
                                 VM_SELECTITEM,MPFROM2SHORT(arrow.lStart,1),
                                 NULL);
               WinSendDlgItemMsg(hwnd,ID_LINEEND,
                                 VM_SELECTITEM,MPFROM2SHORT(arrow.lEnd,1),
                                 NULL);
	       break;
         }
         return (MRESULT)0;

      case WM_CONTROL:
         if (bInit)
            break;
         switch (LOUSHORT(mp1))
         {
            case ID_LINESTART:
               if (HIUSHORT(mp1) == VN_SELECT)
               {
                  /*
                  ** Get selected row. Not the value...
                  */
	          arrow.lStart = (long)WinSendDlgItemMsg(hwnd,ID_LINESTART,
                                                         VM_QUERYSELECTEDITEM, NULL,NULL);
                  arrow.lStart = (long)SHORT1FROMMP((MPARAM)arrow.lStart);
               }
               break;
            case ID_LINEEND:
               if (HIUSHORT(mp1) == VN_SELECT)
               {
                  /*
                  ** Get selected row. Not the value...
                  */
	          arrow.lEnd = (long)WinSendDlgItemMsg(hwnd,ID_LINEEND,
                                                       VM_QUERYSELECTEDITEM, NULL,NULL);
                  arrow.lEnd = (long)SHORT1FROMMP((MPARAM)arrow.lEnd);
               }
               break;
            case ID_SPINARROWSIZE:
               if (HIUSHORT(mp1) == SPBN_CHANGE )
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINARROWSIZE,SPBM_QUERYVALUE,
                                    (MPARAM)&arrow.lSize,MPFROM2SHORT(0,0));
                  if (arrow.lSize <= MIN_ARROWSIZE)
                     arrow.lSize   = MIN_ARROWSIZE;
                  arrow.lSize = min(arrow.lSize,MAX_ARROWSIZE);
               }            
               break;
         }
         pArrow->lStart = arrow.lStart;
         pArrow->lEnd   = arrow.lEnd;
         pArrow->lSize  = arrow.lSize;
         WinInvalidateRect (hArrow,NULL, FALSE);
         return (MRESULT)0;
   }
   return WinDefDlgProc(hwnd,msg,mp1,mp2);
}
/*-----------------------------------------------[ public ]---------------*/
/*  RegisterArrow.                                                        */
/*                                                                        */
/*  Description : Register the windowprocedures for the windows living    */
/*                in the Arrow definition dialog.                         */
/*------------------------------------------------------------------------*/
void RegisterArrow(HAB hab)
{
   WinRegisterClass(hab,                   // Another block handle
                    (PSZ)"ARROWPREVCLASS", // Name of class being registered
                    (PFNWP)ArrWndProc,	   // Window procedure for class
                    CS_SIZEREDRAW,         // Class style
                    0L);                   // Extra bytes to reserve
   return;
}
/*------------------------------------------------------------------------*/
long angleFromPoints( const POINTL ptl1, const POINTL ptl2 )
{
  double angle;
  double x,y;

  x = (double)(ptl1.x - ptl2.x);
  y = (double)(ptl1.y - ptl2.y);

  angle = atan2(y,x);
  angle *= 57.295779;

  if ( angle < 0.0 )
    angle += 360.0;

  return (long)angle;
}
/*---------------------------------------------------------------------------*/
void arrowDetail( HWND hOwner, WINDOWINFO *pwi)
{
   if (!bDialogActive)
   {
      WinLoadDlg(HWND_DESKTOP,hOwner,(PFNWP)ArrowDlgProc,(HMODULE)0,
                 DLG_ARROWS,(PVOID)&pwi->arrow);
      bDialogActive = TRUE;
   }
   return;
}
/*---------------------------------------------------------------------------*/
void drwEndPoints(WINDOWINFO *pwi,ARROW Arrow,POINTL ptl1,POINTL ptl2)
{
   POINTL   ptl;
   RECTL    rcl;
   LONG     lAngle;
   LONG     lHeight;
   MATRIXLF matlf,matOrg;

   if (pwi->ulUnits == PU_PELS)
   {
      Arrow.lSize = (Arrow.lSize * pwi->xPixels)/10000;
      Arrow.lSize *= CONVERT_FACTOR;
   }

   lAngle = angleFromPoints( ptl2, ptl1 );
   /*
   ** The arrows get a kind of geometric size. This means that during
   ** zooming they are not resized. usWidth & usHeight of the 
   ** windowinfo structs are used.
   ** This enables the program to show them arrow etc smaller in the preview
   ** screen and in the print preview screen.
   */
   if (Arrow.lEnd != DEF_LINEEND)
   {
      ptl = ptl2; /* pointl is endpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);

      rcl.xLeft  = ptl2.x;
      rcl.xRight = rcl.xLeft + Arrow.lSize;
      lHeight    = Arrow.lSize;

      rcl.yBottom  = ptl2.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;

      drawArray[Arrow.lEnd-1](pwi->hps,rcl);

      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }

   if (Arrow.lStart != DEF_LINESTART)
   {
      ptl = ptl1; /* pointl is startpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);

      rcl.xRight  = ptl1.x;
      rcl.xLeft   = rcl.xRight - Arrow.lSize;
      lHeight     = Arrow.lSize;

      rcl.yBottom  = ptl1.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;

      drawArray[Arrow.lStart-1](pwi->hps,rcl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }
}
/*-----------------------------------------------[ public ]---------------*/
/* drwEndPt.                                                              */
/*                                                                        */
/* Description : Used when a polyline wants to draw an arrow at the end.  */
/*               Uses two point the calculate the angle of the line end.  */
/*------------------------------------------------------------------------*/
void drwEndPt(WINDOWINFO *pwi,ARROW Arrow,POINTL ptl1,POINTL ptl2)
{
   POINTL   ptl;
   RECTL    rcl;
   LONG     lAngle;
   LONG     lHeight;
   MATRIXLF matlf,matOrg;

   if (pwi->ulUnits == PU_PELS)
   {
      Arrow.lSize = (Arrow.lSize * pwi->xPixels)/10000;
      Arrow.lSize *= CONVERT_FACTOR;
   }
   lAngle = angleFromPoints( ptl2, ptl1 );
   /*
   ** The arrows get a kind of geometric size. This means that during
   ** zooming they are not resized.
   */
   if (Arrow.lEnd != DEF_LINEEND)
   {
      ptl = ptl2; /* pointl is endpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
      rcl.xLeft  = ptl2.x;
      rcl.xRight = rcl.xLeft + Arrow.lSize;
      lHeight    = Arrow.lSize;

      rcl.yBottom  = ptl2.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;

      drawArray[Arrow.lEnd-1](pwi->hps,rcl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }
}
/*-----------------------------------------------[ public ]---------------*/
/* drwStartPt.                                                            */
/*                                                                        */
/* Description : Used when a polyline wants to draw an arrow at the start.*/
/*               Uses two point the calculate the angle of the line end.  */
/*------------------------------------------------------------------------*/
void drwStartPt(WINDOWINFO *pwi,ARROW Arrow,POINTL ptl1,POINTL ptl2)
{
   POINTL   ptl;
   RECTL    rcl;
   LONG     lAngle;
   LONG     lHeight;
   MATRIXLF matlf,matOrg;

   if (pwi->ulUnits == PU_PELS)
   {
      Arrow.lSize = (Arrow.lSize * pwi->xPixels)/10000;    
      Arrow.lSize *= CONVERT_FACTOR;
   }
   lAngle = angleFromPoints( ptl2, ptl1 );

   if (Arrow.lStart != DEF_LINESTART)
   {
      ptl = ptl1; /* pointl is startpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
      rcl.xRight  = ptl1.x;
      rcl.xLeft   = rcl.xRight - Arrow.lSize;
      lHeight     = Arrow.lSize;

      rcl.yBottom  = ptl1.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;

      drawArray[Arrow.lStart-1](pwi->hps,rcl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }
}
/*------------------------------------------------------------------------*/
void drwEndPtAtAngle(WINDOWINFO *pwi,ARROW Arrow,LONG lAngle,POINTL ptl2)
{
   POINTL   ptl;
   RECTL    rcl;
   LONG     lHeight;
   MATRIXLF matlf,matOrg;

   if (pwi->ulUnits == PU_PELS)
   {
      Arrow.lSize = (Arrow.lSize * pwi->xPixels)/10000;    
      Arrow.lSize *= CONVERT_FACTOR;
   }
   /*
   ** The arrows get a kind of geometric size. This means that during
   ** zooming they are not resized.
   */
   if (Arrow.lEnd != DEF_LINEEND)
   {
      ptl = ptl2; /* pointl is endpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
      rcl.xLeft  = ptl2.x;
      rcl.xRight = rcl.xLeft + Arrow.lSize;
      lHeight    = Arrow.lSize;

      rcl.yBottom  = ptl2.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;
      drawArray[Arrow.lEnd-1](pwi->hps,rcl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }
}
/*------------------------------------------------------------------------*/
void drwStartPtAtAngle(WINDOWINFO *pwi,ARROW Arrow,LONG lAngle,POINTL ptl1)
{
   POINTL   ptl;
   RECTL    rcl;
   LONG     lHeight;
   MATRIXLF matlf,matOrg;

   if (pwi->ulUnits == PU_PELS)
   {
      Arrow.lSize = (Arrow.lSize * pwi->xPixels)/10000;    
      Arrow.lSize *= CONVERT_FACTOR;
   }
   if (Arrow.lStart != DEF_LINESTART)
   {
      ptl = ptl1; /* pointl is startpoint....*/

      GpiQueryModelTransformMatrix(pwi->hps,9L,&matOrg);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(lAngle,0), &ptl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
      rcl.xRight  = ptl1.x;
      rcl.xLeft   = rcl.xRight - Arrow.lSize;
      lHeight     = Arrow.lSize;

      rcl.yBottom  = ptl1.y - (lHeight/2);
      rcl.yTop     = rcl.yBottom + lHeight;

      drawArray[Arrow.lStart-1](pwi->hps,rcl);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matOrg,TRANSFORM_REPLACE);
   }
}
/*-----------------------------------------------[ public ]---------------*/
/* arrowAreaExtra.                                                        */
/*                                                                        */
/* description  : Called by object during the calculation of the outline  */
/*                size.                                                   */
/*                                                                        */
/* parameters   : ARROW Arrow - Arrow description.                        */
/*                RECTL *     - Reference to rectangle.                   */
/*                                                                        */
/* On return    : The rectangle including the arrow size.                 */
/*                                                                        */
/* Returns      : NONE.                                                   */
/*------------------------------------------------------------------------*/
void arrowAreaExtra(ARROW Arrow, RECTL *prcl )
{
    if (Arrow.lStart != DEF_LINESTART)
    {
       prcl->xLeft   -= Arrow.lSize;
       prcl->yTop    += Arrow.lSize;
       prcl->xRight  += Arrow.lSize;
       prcl->yBottom -= Arrow.lSize;
    }

    if (Arrow.lEnd != DEF_LINEEND)
    {
       prcl->xLeft   -= Arrow.lSize;
       prcl->yTop    += Arrow.lSize;
       prcl->xRight  += Arrow.lSize;
       prcl->yBottom -= Arrow.lSize;
    }
    return;
}
