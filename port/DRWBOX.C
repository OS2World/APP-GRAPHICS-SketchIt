/*------------------------------------------------------------------------*/
/*  Name: drwbox.c                                                        */
/*                                                                        */
/*  Description : Box functions.                                          */
/*                                                                        */
/*  Functions  :                                                          */
/*  boxCreate     : Create a new box object.                              */
/*  boxMoveOutLine: Move the box over dx,dy and show outline.(mousemove)  */
/*  boxDraw       : Draw the box with all its attributes                  */
/*  boxDrawOutline: Draw only the outline.                                */
/*  boxOutLine    : Get the outline rectangle.                            */
/*  boxInvArea    : Get the complete area incl selection handles in pixels*/
/*  boxPutInGroup : Put the box in a group.                               */
/*  boxRemFromGroup: Remove the box from the group.                       */
/*  boxStretch    : Stretch the box.                                      */
/*  boxSelect     : Was mouse click on our head??                         */
/*  boxGetCenter  : Get the rotation center location of this box          */
/*  boxSetCenter  : Set the rotation center location of this box (groups) */
/*  boxPtrAboveCenter : Did you click on my rotation center?              */
/*  boxNewPattern : Give me a new filling pattern.                        */
/*  boxMake       : Box contruction (Handle mousemove and button1up)      */
/*  boxMoveSegment: Set the box to position + dx,dy                       */
/*  boxDetails    : Show the attributes of the given box.   (dialog)      */
/*  boxRounding   : Show program rounding attribute in dialog             */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "resource.h"
#include "drwtypes.h"
#include "dlg.h"
#include "drwutl.h"
#include "dlg_clr.h"
#include "drwbox.h"
#include "drwfount.h"

#define PI 3.1415926


#define MINROUND 0
#define MAXROUND 200

#define DB_RAISED    0x0400              // frame drawing.
#define DB_DEPRESSED 0x0800              // frame drawing.

static CHAR szBoxClass[]    = "BOXCLASS";

static BOX  locbox;


MRESULT EXPENTRY boxWndProc(HWND hwnd, ULONG ulMsg,MPARAM mp1,MPARAM mp2)
{
   HPS hps;
   RECTL rcl;
   long cxClient,cyClient;
   static POINTL ptl1,ptl2;
   static RECTL  rclWnd;
   static HPS    hpsLometric;
   static LONG   lVertSideWidth;    /* Width of border rectangle vertical sides   */
   static LONG   lHorizSideWidth;   /* Width of border rectangle horizontal sides */
   static LONG   lFrameColor;       /* Color to be used for frames.               */
   static ULONG  flCmd;             /* draw flags                                 */
   HDC     hdc;
   SIZEL   sizl;

   switch (ulMsg)
   {
      case WM_CREATE:
         WinQueryWindowRect(hwnd,&rcl);
         cxClient = rcl.xRight - rcl.xLeft;
         cyClient = rcl.yTop - rcl.yBottom;
         ptl1.x = cxClient / 4;
         ptl1.y = cyClient / 4;
         ptl2.x = cxClient - (cxClient / 4);
         ptl2.y = cyClient - (cyClient / 4);
         rclWnd.xLeft   = 0;
         rclWnd.yBottom = 0;
         rclWnd.yTop    = cyClient;
         rclWnd.xRight  = cxClient;

         sizl.cx = 0;
         sizl.cy = 0;

         hdc = WinOpenWindowDC(hwnd);
         hpsLometric = GpiCreatePS(hab,hdc,&sizl,
			 PU_LOMETRIC  |      /* 0.1 mm precision */
			 GPIF_DEFAULT |
			 GPIT_MICRO   |
			 GPIA_ASSOC);

         GpiConvert(hpsLometric,CVTC_DEVICE,CVTC_WORLD,1,&ptl1);
         GpiConvert(hpsLometric,CVTC_DEVICE,CVTC_WORLD,1,&ptl2);


         lVertSideWidth =2;
         lHorizSideWidth=2;
         flCmd = DB_DEPRESSED;
         lFrameColor = WinQuerySysColor(HWND_DESKTOP,SYSCLR_BUTTONLIGHT,0);

         return (MRESULT)0;

      case WM_PAINT:
         hps = WinBeginPaint(hwnd,hpsLometric,&rcl);
         GpiErase(hps);
         GpiCreateLogColorTable ( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         GpiSetPattern(hps,PATSYM_SOLID);
         GpiSetColor(hps,lFrameColor);
         WinDrawBorder(hps, &rclWnd,lVertSideWidth,lHorizSideWidth,
                       0L, 0L, flCmd);
         GpiSetColor(hps,0x000000FF);
         GpiMove(hps, &ptl1);
         GpiBox(hps,DRO_OUTLINE,&ptl2,locbox.lHRound,locbox.lVRound);
         WinEndPaint(hps);
         return (MRESULT)0;

      case WM_DESTROY:
         GpiDestroyPS(hpsLometric);
         hpsLometric = (HPS)0;
         break;
   }
   return WinDefWindowProc(hwnd,ulMsg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/* roundDlgProc                                                           */
/*                                                                        */
/* Description  : Dialog procedure used to set the rounding program wide  */
/*                Acts on the same dialog as used for changing attr's on  */
/*                a single box object but gets only one long as parametes */
/*                describing the rounding on program level.               */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY roundDlgProc(HWND hwnd, ULONG ulMsg,MPARAM mp1,MPARAM mp2)
{
   static HWND hBox;
   static BOOL bInit;
   static LONG *pRounding; /* The reference to the program wide value */
   SWP    swp;

   switch (ulMsg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         pRounding = (LONG *)mp2;
         /* Centre dialog on the screen   */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
                         ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
                         ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
                         0, 0, SWP_MOVE);


         locbox.lHRound = *pRounding;
         locbox.lVRound = *pRounding;

         /* 
         ** Hide the layer spinbutton, groupbox and text
         */
         WinShowWindow(WinWindowFromID(hwnd, ID_SPINLAYER),FALSE);
         WinShowWindow(WinWindowFromID(hwnd, ID_GRPBOXLAYER),FALSE);
         WinShowWindow(WinWindowFromID(hwnd, ID_TXTLAYER),FALSE);
         /*
         ** Vertical and horizontal rounding.
         */
         WinSendDlgItemMsg( hwnd, ID_SPINROUND, SPBM_SETLIMITS,
                           MPFROMLONG(MAXROUND), MPFROMLONG(MINROUND));

         WinSendDlgItemMsg( hwnd,ID_SPINROUND, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)locbox.lVRound), NULL);

         hBox = WinWindowFromID(hwnd, ID_BOXWND);

         WinInvalidateRect (hBox,NULL, FALSE);
         bInit = FALSE;
         break;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               *pRounding = locbox.lVRound;
               WinDismissDlg(hwnd,DID_OK);
               return (MRESULT)0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               return (MRESULT)0;
            case DID_HELP:
               return (MRESULT)0;
         }
         return (MRESULT)0;

      case WM_CONTROL:
         switch (LOUSHORT(mp1))
         {
            case ID_SPINROUND:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINROUND,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&locbox.lVRound),
                                     MPFROM2SHORT(0,0));
                  locbox.lHRound = locbox.lVRound;
                  WinInvalidateRect (hBox,NULL, FALSE);
               }
               break;
         }
         return (MRESULT)0;
   }
   return WinDefDlgProc(hwnd,ulMsg,mp1,mp2);
}

/*------------------------------------------------------------------------*/
MRESULT EXPENTRY boxDlgProc(HWND hwnd, ULONG ulMsg,MPARAM mp1,MPARAM mp2)
{
   static PBOX pBox;
   static HWND hBox;
   static BOOL bInit;
   SWP    swp;

   switch (ulMsg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         pBox = (PBOX)mp2;
         /* Centre dialog on the screen   */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
                         ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
                         ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
                         0, 0, SWP_MOVE);

         /* Set the layer spinbutton */

         WinSendDlgItemMsg( hwnd, ID_SPINLAYER, SPBM_SETLIMITS,
                           MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

         WinSendDlgItemMsg( hwnd,ID_SPINLAYER, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)pBox->uslayer), NULL);
         /*
         ** Vertical and horizontal rounding.
         */
         WinSendDlgItemMsg( hwnd, ID_SPINROUND, SPBM_SETLIMITS,
                           MPFROMLONG(MAXROUND), MPFROMLONG(MINROUND));

         WinSendDlgItemMsg( hwnd,ID_SPINROUND, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)pBox->lVRound), NULL);

         hBox = WinWindowFromID(hwnd, ID_BOXWND);

         locbox.lHRound = pBox->lHRound;
         locbox.lVRound = pBox->lVRound;
         locbox.uslayer = pBox->uslayer;
         WinInvalidateRect (hBox,NULL, FALSE);
         bInit = FALSE;
         break;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:

               WinSendDlgItemMsg( hwnd,ID_SPINLAYER,SPBM_QUERYVALUE,
                                 (MPARAM)(&pBox->uslayer),
                                 MPFROM2SHORT(0,0));
               pBox->lVRound = locbox.lVRound;
               pBox->lHRound = locbox.lVRound;
               WinDismissDlg(hwnd,DID_OK);
               return (MRESULT)0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               return (MRESULT)0;
            case DID_HELP:
               return (MRESULT)0;
         }
         return (MRESULT)0;

      case WM_CONTROL:
         switch (LOUSHORT(mp1))
         {
            case ID_SPINROUND:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINROUND,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&locbox.lVRound),
                                     MPFROM2SHORT(0,0));
                  locbox.lHRound = locbox.lVRound;
                  WinInvalidateRect (hBox,NULL, FALSE);
               }
               break;
         }
         return (MRESULT)0;
   }
   return WinDefDlgProc(hwnd,ulMsg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
POBJECT boxCreate(WINDOWINFO *pwi, POINTL ptlStart)
{
   PBOX pBox;

   pBox = (PBOX)pObjCreate(CLS_BOX);

   if (!pBox)
   {
       return (POBJECT)0;
   }

   pBox->rclf.xLeft = (float)ptlStart.x;
   pBox->rclf.yBottom = (float)ptlStart.y;


   pBox->line.LineColor = pwi->ulOutLineColor;
   pBox->fColor         = pwi->ulColor;      /* filling color */
   pBox->line.LineType  = pwi->lLntype;
   pBox->line.LineWidth = pwi->lLnWidth;
   pBox->line.LineJoin  = pwi->lLnJoin;
   pBox->line.LineEnd   = pwi->lLnEnd;

   pBox->lPattern   = pwi->ColorPattern;        /* filling pattern */
   pBox->ustype     = CLS_BOX;
   pBox->uslayer    = pwi->uslayer;
   /*
   ** Get the gradient stuff...
   */
   pBox->gradient.ulStart      = pwi->Gradient.ulStart;
   pBox->gradient.ulSweep      = pwi->Gradient.ulSweep;
   pBox->gradient.ulSaturation = pwi->Gradient.ulSaturation;
   pBox->gradient.ulDirection  = pwi->Gradient.ulDirection;
   /*
   ** Fountain fill stuff....
   */
   pBox->fountain.ulStartColor = pwi->fountain.ulStartColor;
   pBox->fountain.ulEndColor   = pwi->fountain.ulEndColor;
   pBox->fountain.lHorzOffset  = pwi->fountain.lHorzOffset;
   pBox->fountain.lVertOffset  = pwi->fountain.lVertOffset;
   pBox->fountain.ulFountType  = pwi->fountain.ulFountType;
   pBox->fountain.lAngle       = pwi->fountain.lAngle;

   pBox->lVRound = pwi->lRound; /* Corner-rounding control */
   pBox->lHRound = pwi->lRound; /* Corner-rounding control */
   pBox->lRotate = 0L;          /* Rotation                */

   return (POBJECT)pBox;
}
/*-------------------------------------------------------------------------*/
void boxMoveOutLine(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy)
{
   POINTL ptl;
   PBOX   pBox = (PBOX)pObj;
   MATRIXLF matlf;


   ptl.y = (USHORT)(pwi->fOffy);
   ptl.x = (USHORT)(pwi->fOffx);

   ptl.x += (LONG)(pBox->rclf.xLeft * pwi->usFormWidth);
   ptl.y += (LONG)(pBox->rclf.yBottom * pwi->usFormHeight);

   ptl.x += dx;
   ptl.y += dy;

   if (pBox->lRotate)
   {
      POINTL ptlCenter;
      ptlCenter.x = (LONG)(pwi->fOffx);
      ptlCenter.y = (LONG)(pwi->fOffy);
      ptlCenter.x += (LONG)(pBox->ptlfCenter.x * pwi->usFormWidth);
      ptlCenter.y += (LONG)(pBox->ptlfCenter.y * pwi->usFormHeight);

      ptlCenter.x += dx;
      ptlCenter.y += dy;

      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(pBox->lRotate,0), &ptlCenter);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
   }


   GpiMove(pwi->hps,&ptl);

   ptl.y = (USHORT)(pwi->fOffy);
   ptl.x = (USHORT)(pwi->fOffx);

   ptl.x += (LONG)(pBox->rclf.xRight * pwi->usFormWidth);
   ptl.y += (LONG)(pBox->rclf.yTop * pwi->usFormHeight);

   ptl.x += dx;
   ptl.y += dy;

   GpiBox(pwi->hps,DRO_OUTLINE, &ptl,pBox->lHRound,pBox->lVRound);

   if (pBox->lRotate)
   {
      /*
      ** Reset ....
      */
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &pwi->matOrg,TRANSFORM_REPLACE);
    }


   return;
}
/*------------------------------------------------------------------------*/
/* Move a box segment.                                                    */
/*------------------------------------------------------------------------*/
void boxMoveSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy;
   float fcx,fcy;
   PBOX  pBox = (PBOX)pObj;

   if (!pBox)
      return;

   fdx = (float)dx;
   fdy = (float)dy;
   fcx = pwi->usFormWidth;
   fcy = pwi->usFormHeight;

   pBox->rclf.xLeft   += (float)( fdx / fcx );
   pBox->rclf.xRight  += (float)( fdx / fcx );
   pBox->rclf.yTop    += (float)( fdy / fcy );
   pBox->rclf.yBottom += (float)( fdy / fcy );
   pBox->ptlfCenter.x += (float)( fdx / fcx );
   pBox->ptlfCenter.y += (float)( fdy / fcy );
   return;
}
/*-------------------------------------------------------------------------*/
void boxDraw(HPS hps, WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl)
{
   PBOX   pBox = (PBOX)pObj;
   POINTL ptl1,ptl2;
   RECTL  rcl,rclDest; 
   MATRIXLF matlf;

   if (pwi->usdrawlayer != pBox->uslayer)
      return;

   if (prcl)
   {
      boxOutLine(pObj,&rcl,pwi,TRUE);
      if (!WinIntersectRect(hab,&rclDest,prcl,&rcl) || rcl.yTop < 0)
         return;
   }
   GpiSetMix(hps,FM_DEFAULT);

   if (pBox->lRotate)
   {
      POINTL ptlCenter;
      ptlCenter.x = (LONG)(pwi->fOffx);
      ptlCenter.y = (LONG)(pwi->fOffy);
      ptlCenter.x += (LONG)(pBox->ptlfCenter.x * pwi->usFormWidth);
      ptlCenter.y += (LONG)(pBox->ptlfCenter.y * pwi->usFormHeight);
      GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                MAKEFIXED(pBox->lRotate,0), &ptlCenter);
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &matlf,TRANSFORM_REPLACE);
   }

   GpiSetLineType(hps,pBox->line.LineType);
   GpiSetPattern(hps, pBox->lPattern);
   GpiSetColor(hps,pBox->fColor);
      
   ptl1.y = (USHORT)(pwi->fOffy);
   ptl1.x = (USHORT)(pwi->fOffx);

   ptl1.x += (LONG)(pBox->rclf.xLeft * pwi->usFormWidth);
   ptl1.y += (LONG)(pBox->rclf.yBottom * pwi->usFormHeight);

   GpiMove(hps,&ptl1);

   ptl2.y = (USHORT)(pwi->fOffy);
   ptl2.x = (USHORT)(pwi->fOffx);

   ptl2.x += (LONG)(pBox->rclf.xRight * pwi->usFormWidth);
   ptl2.y += (LONG)(pBox->rclf.yTop * pwi->usFormHeight);

   if (pBox->lPattern == PATSYM_DEFAULT)
   {
      LONG lLineWidth = pBox->line.LineWidth * pwi->uXfactor;
      /*
      ** No filling
      */
      GpiSetColor(hps,pBox->line.LineColor);

      if (pwi->ulUnits == PU_PELS)
      {
         lLineWidth = (lLineWidth * pwi->xPixels)/10000;
      }

      if (lLineWidth > 1)
      {
         GpiSetLineWidthGeom(hps,lLineWidth);
         GpiBeginPath(hps, 1L);  /* define a clip path    */
      }

      GpiBox(hps,DRO_OUTLINE, 
             &ptl2,pBox->lHRound,pBox->lVRound);

      if (lLineWidth > 1)
      {
         GpiEndPath(hps);
         GpiStrokePath (hps, 1, 0);
      }
   }
   else if ( pBox->lPattern != PATSYM_GRADIENTFILL && 
             pBox->lPattern != PATSYM_FOUNTAINFILL )
   {
      /*
      ** we have a standard OS/2 patternfill.
      */
      if (pBox->line.LineColor == pBox->fColor )
      {
         GpiBox(hps,DRO_OUTLINEFILL, 
                &ptl2,pBox->lHRound,pBox->lVRound);
      }
      else
      {
         LONG lLineWidth = pBox->line.LineWidth * pwi->uXfactor;
         /*
         ** Draw the filling part
         */
         GpiBox(hps,DRO_FILL, 
                &ptl2,pBox->lHRound,pBox->lVRound);
         /*
         ** Draw the outline
         */
         GpiMove(hps,&ptl1);
         GpiSetColor(hps,pBox->line.LineColor);

         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pBox->line.LineJoin);
         GpiSetLineEnd(hps,pBox->line.LineEnd);

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }

         if (lLineWidth > 1)
         {
            GpiSetLineWidthGeom(hps,lLineWidth);
            GpiBeginPath(hps, 1L);  /* define a clip path    */
         }

         GpiBox(hps,DRO_OUTLINE, 
                &ptl2,pBox->lHRound,pBox->lVRound);

         if (lLineWidth > 1)
         {
            GpiEndPath(hps);
            GpiStrokePath (hps, 1, 0);
         }

      }
   }
   else if (pBox->lPattern == PATSYM_GRADIENTFILL || 
            pBox->lPattern == PATSYM_FOUNTAINFILL)
   {
      LONG lLineWidth = pBox->line.LineWidth * pwi->uXfactor;

      GpiBeginPath( hps, 1L);  /* define a clip path    */

      GpiMove(hps,&ptl1);
      GpiBox(hps,DRO_OUTLINE, 
             &ptl2,pBox->lHRound,pBox->lVRound);

      GpiEndPath(hps);
      GpiSetClipPath(hps,1L,SCP_AND);
      GpiSetPattern(hps,PATSYM_SOLID);
      if (pBox->lPattern == PATSYM_GRADIENTFILL)
         GradientFill(pwi,hps,&rcl,&pBox->gradient);
      else
         FountainFill(pwi,hps,&rcl,&pBox->fountain);

       GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */


       if (pBox->line.LineType !=  LINETYPE_INVISIBLE)
       {
         /*
         ** Draw the outline
         */
         GpiMove(hps,&ptl1);
         GpiSetColor(hps,pBox->line.LineColor);
         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pBox->line.LineJoin);
         GpiSetLineEnd(hps,pBox->line.LineEnd);

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }

         if (lLineWidth > 1)
         {
            GpiSetLineWidthGeom(hps,lLineWidth);
            GpiBeginPath(hps, 1L);  /* define a clip path    */
         }

         GpiBox(hps,DRO_OUTLINE, 
                &ptl2,pBox->lHRound,pBox->lVRound);

         if (lLineWidth > 1)
         {
            GpiEndPath(hps);
            GpiStrokePath (hps, 1, 0);
         }
      }
   }

   if (pBox->lRotate)
   {
      /*
      ** Reset ....
      */
      GpiSetModelTransformMatrix(pwi->hps,9L,
                                 &pwi->matOrg,TRANSFORM_REPLACE);
    }

   return;
}
/*-----------------------------------------------[ public ]----------*/
/* boxDrawOutline.                                                   */
/*-------------------------------------------------------------------*/
void boxDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer)
{
   PBOX pBox = (PBOX)pObj;

   if (pwi->usdrawlayer == pBox->uslayer || bIgnorelayer)
      boxMoveOutLine(pObj,pwi,0,0);
   return;
}
/*------------------------------------------------------------------------*/
void boxOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc)
{
   PBOX pBox = (PBOX)pObj;

   if (!pObj)  return;

   if (bCalc)
   {
      GpiResetBoundaryData(pwi->hps);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_ON);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
      boxMoveOutLine(pObj,pwi,0,0);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_OFF);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
      GpiQueryBoundaryData(pwi->hps, &pObj->rclOutline);

      if (pBox->lPattern == PATSYM_GRADIENTFILL ||
          pBox->lPattern == PATSYM_FOUNTAINFILL )
      {
         /*
         ** We only need the normalized rectl when the circle
         ** is used for clipping. Then the normalized rect can
         ** be used when printing is done.
         ** GpiQueryBoundary rect does not function on a printer
         ** PS so this is a workaround.
         */
         pObj->rclf.xLeft    =  (float)pObj->rclOutline.xLeft;
         pObj->rclf.xLeft    /= (float)pwi->usFormWidth;
         pObj->rclf.xRight   =  (float)pObj->rclOutline.xRight;
         pObj->rclf.xRight   /= (float)pwi->usFormWidth;
         pObj->rclf.yBottom  =  (float)pObj->rclOutline.yBottom;
         pObj->rclf.yBottom  /= (float)pwi->usFormHeight;
         pObj->rclf.yTop     =  (float)pObj->rclOutline.yTop;
         pObj->rclf.yTop     /= (float)pwi->usFormHeight;
      }
   }
   rcl->xLeft   = pObj->rclOutline.xLeft;
   rcl->xRight  = pObj->rclOutline.xRight;
   rcl->yTop    = pObj->rclOutline.yTop;
   rcl->yBottom = pObj->rclOutline.yBottom;
   return;  
}
/*------------------------------------------------------------------------*/
void boxInvArea(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi, BOOL bInc)
{
   PBOX pBox = (PBOX)pObj;

   if (!pBox)
      return;

   boxOutLine(pObj,prcl,pwi,TRUE);

   prcl->xLeft    -= pBox->line.LineWidth;
   prcl->xRight   += pBox->line.LineWidth;
   prcl->yBottom  -= pBox->line.LineWidth;
   prcl->yTop     += pBox->line.LineWidth;

   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)prcl);

   /*
   ** Add extra space for selection handles.
   */
   if (bInc)
   {
      prcl->xLeft    -= HANDLESIZE;
      prcl->xRight   += HANDLESIZE;
      prcl->yBottom  -= HANDLESIZE;
      prcl->yTop     += HANDLESIZE;
   }
   else
   {
      prcl->xRight   += 1;
   }
}
/*------------------------------------------------------------------------*/
/*  boxPutInGroup.                                                        */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void boxPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   PBOX    pBox = (PBOX)pObj;
   USHORT  usGrpWidth,usGrpHeight;

   if (!pBox) 
      return;
   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);
   /*
   ** Setup the new rotation center.
   */
   pBox->ptlfCenter.x = (float)(prcl->xLeft  +(prcl->xRight - prcl->xLeft)/2);
   pBox->ptlfCenter.y = (float)(prcl->yBottom+(prcl->yTop - prcl->yBottom)/2);
   pBox->ptlfCenter.x /= (float)usGrpWidth;
   pBox->ptlfCenter.y /= (float)usGrpHeight;

   pBox->rclf.xLeft   *= (float)pwi->usFormWidth ;
   pBox->rclf.yBottom *= (float)pwi->usFormHeight;
   pBox->rclf.xRight  *= (float)pwi->usFormWidth ;
   pBox->rclf.yTop    *= (float)pwi->usFormHeight;

   pBox->rclf.xLeft    -= (float)prcl->xLeft;
   pBox->rclf.yBottom  -= (float)prcl->yBottom;
   pBox->rclf.xRight   -= (float)prcl->xLeft;
   pBox->rclf.yTop     -= (float)prcl->yBottom;

   pBox->rclf.xLeft    /= (float)usGrpWidth;
   pBox->rclf.yBottom  /= (float)usGrpHeight;
   pBox->rclf.xRight   /= (float)usGrpWidth;
   pBox->rclf.yTop     /= (float)usGrpHeight;
   return;
}
/*------------------------------------------------------------------------*/
/*  CirRemFromGroup.                                                      */
/*                                                                        */
/*  Remove the object from the group.                                     */
/*------------------------------------------------------------------------*/
void boxRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   PBOX pBox = (PBOX)pObj;
   USHORT  usGrpWidth,usGrpHeight;

   if (!pBox) return;

   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   pBox->rclf.xLeft   *= (float)usGrpWidth;
   pBox->rclf.yBottom *= (float)usGrpHeight;
   pBox->rclf.xRight  *= (float)usGrpWidth;
   pBox->rclf.yTop    *= (float)usGrpHeight;

   pBox->rclf.xLeft   += (float)prcl->xLeft;
   pBox->rclf.yBottom += (float)prcl->yBottom;
   pBox->rclf.xRight  += (float)prcl->xLeft;
   pBox->rclf.yTop    += (float)prcl->yBottom;
   /*
   ** Restore the center point for rotation
   */
   pBox->ptlfCenter.x = pBox->rclf.xLeft   + (pBox->rclf.xRight - pBox->rclf.xLeft);
   pBox->ptlfCenter.y = pBox->rclf.yBottom + (pBox->rclf.yTop - pBox->rclf.yBottom);

   pBox->rclf.xLeft   /= (float)pwi->usFormWidth;
   pBox->rclf.yBottom /= (float)pwi->usFormHeight;
   pBox->rclf.xRight  /= (float)pwi->usFormWidth;
   pBox->rclf.yTop    /= (float)pwi->usFormHeight;

   pBox->ptlfCenter.x /= (float)pwi->usFormWidth;
   pBox->ptlfCenter.y /= (float)pwi->usFormHeight;
   return;
}
/*------------------------------------------------------------------------*/
/*  CirStretch.                                                           */
/*                                                                        */
/*  Description : Stretches the circle. Called during the following msg's.*/
/*                WM_BUTTON1DOWN,WM_BUTTON1UP and WM_MOUSEMOVE.           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void boxStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg)
{
   static USHORT usWidth,usHeight;
   USHORT usOldWidth,usOldHeight;
   static float fOffx,fOffy;
   float fOldx,fOldy;

   PBOX pBox = (PBOX)pObj;

   if (!pBox) 
      return;

   usWidth  =(USHORT)(prcl->xRight - prcl->xLeft);
   usHeight =(USHORT)(prcl->yTop - prcl->yBottom);

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         pBox->rclf.xLeft   *= (float)pwi->usFormWidth ;
         pBox->rclf.yBottom *= (float)pwi->usFormHeight;
         pBox->rclf.xRight  *= (float)pwi->usFormWidth;
         pBox->rclf.yTop    *= (float)pwi->usFormHeight;

         pBox->rclf.xLeft   -= (float)prcl->xLeft;
         pBox->rclf.yBottom -= (float)prcl->yBottom;
         pBox->rclf.xRight  -= (float)prcl->xLeft;
         pBox->rclf.yTop    -= (float)prcl->yBottom;

         pBox->rclf.xLeft   /= (float)usWidth;
         pBox->rclf.yBottom /= (float)usHeight;
         pBox->rclf.xRight  /= (float)usWidth;
         pBox->rclf.yTop    /= (float)usHeight;

         pBox->ptlfCenter.x *= (float)pwi->usFormWidth;
         pBox->ptlfCenter.y *= (float)pwi->usFormHeight;
         pBox->ptlfCenter.x -= (float)prcl->xLeft;
         pBox->ptlfCenter.y -= (float)prcl->yBottom;
         pBox->ptlfCenter.x /= (float)usWidth;
         pBox->ptlfCenter.y /= (float)usHeight;

      case WM_MOUSEMOVE:
         fOffx = (float)prcl->xLeft;
         fOffy = (float)prcl->yBottom;
         fOldx = pwi->fOffx;
         fOldy = pwi->fOffy;
         pwi->fOffx = fOffx;
         pwi->fOffy = fOffy;
         usOldWidth  = pwi->usFormWidth;
         usOldHeight = pwi->usFormHeight;
         pwi->usFormWidth  = usWidth;
         pwi->usFormHeight = usHeight;
         boxMoveOutLine(pObj,pwi,0,0);
         pwi->usFormWidth  = usOldWidth;
         pwi->usFormHeight = usOldHeight;
         pwi->fOffx = fOldx;
         pwi->fOffy = fOldy;
         return;

      case WM_BUTTON1UP:
         pBox->rclf.xLeft   *= (float)usWidth ;
         pBox->rclf.yBottom *= (float)usHeight;
         pBox->rclf.xRight  *= (float)usWidth;
         pBox->rclf.yTop    *= (float)usHeight;

         pBox->rclf.xLeft   += (float)prcl->xLeft;
         pBox->rclf.yBottom += (float)prcl->yBottom;
         pBox->rclf.xRight  += (float)prcl->xLeft;
         pBox->rclf.yTop    += (float)prcl->yBottom;

         pBox->rclf.xLeft   /= (float)pwi->usFormWidth;
         pBox->rclf.yBottom /= (float)pwi->usFormHeight;
         pBox->rclf.xRight  /= (float)pwi->usFormWidth;
         pBox->rclf.yTop    /= (float)pwi->usFormHeight;

         pBox->ptlfCenter.x *= (float)usWidth;
         pBox->ptlfCenter.y *= (float)usHeight;
         pBox->ptlfCenter.x += (float)prcl->xLeft;
         pBox->ptlfCenter.y += (float)prcl->yBottom;
         pBox->ptlfCenter.x /= (float)pwi->usFormWidth;
         pBox->ptlfCenter.y /= (float)pwi->usFormHeight;
         return;
   }
   return;
}
/*------------------------------------------------------------------------*/
VOID * boxSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   PBOX pBox = (PBOX)pObj;

   if (pBox->uslayer == pwi->uslayer || pwi->bSelAll)
      if (WinPtInRect((HAB)0,&pObj->rclOutline,&ptl))
         return (void *)pObj;
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/* boxGetCenter.                                                          */
/*                                                                        */
/* Returns     :  NONE.                                                   */
/*------------------------------------------------------------------------*/
BOOL boxGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl)
{
   PBOX pBox = (PBOX)pObj;

   if (!pObj) 
      return FALSE;

   ptl->x = (LONG)(pBox->ptlfCenter.x * pwi->usFormWidth);
   ptl->y = (LONG)(pBox->ptlfCenter.y * pwi->usFormHeight);
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* boxSetCenter.                                                          */
/*                                                                        */
/* Returns     :  NONE.                                                   */
/*------------------------------------------------------------------------*/
BOOL boxSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl)
{

   PBOX pBox = (PBOX)pObj;

   if (!pObj) 
     return FALSE;

   pBox->ptlfCenter.x = (float)ptl->x; 
   pBox->ptlfCenter.x /= pwi->usFormWidth;
   pBox->ptlfCenter.y = (float)ptl->y;
   pBox->ptlfCenter.y /= pwi->usFormHeight;
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* boxPtrAboveCenter.                                                     */
/*                                                                        */
/* Returns     :  BOOL TRUE if mousepointer is above the center.          */
/*------------------------------------------------------------------------*/
BOOL boxPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse)
{
   PBOX pBox = (PBOX)pObj;
   RECTL rcl;

   if (!pObj) return FALSE;

   rcl.xLeft = (LONG)(pBox->ptlfCenter.x * pwi->usFormWidth);
   rcl.xLeft  -= HANDLESIZE;
   rcl.xRight = rcl.xLeft + (2 * HANDLESIZE);

   rcl.yBottom = (LONG)(pBox->ptlfCenter.y * pwi->usFormHeight);
   rcl.yBottom -= HANDLESIZE;
   rcl.yTop  = rcl.yBottom + (2 * HANDLESIZE );
   return WinPtInRect((HAB)0,&rcl,&ptlMouse);
}


BOOL boxNewPattern(WINDOWINFO *pwi, POBJECT pObj, ULONG ulPattern,BOOL bDialog)
{
   PBOX pBox = (PBOX)pObj;

   switch(ulPattern)
   {
      case PATSYM_GRADIENTFILL:
         if (bDialog)
         {
            if (WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)GradientDlgProc,
                          0L,ID_GRADIENT,(PVOID)&pBox->gradient) == DID_OK)
            {
               pBox->lPattern = ulPattern;
               return TRUE;
            }
            else
               return FALSE;
         }
         else
         {
            pBox->lPattern = ulPattern;
            pBox->gradient.ulStart      = pwi->Gradient.ulStart;
            pBox->gradient.ulSweep      = pwi->Gradient.ulSweep;
            pBox->gradient.ulSaturation = pwi->Gradient.ulSaturation;
            pBox->gradient.ulDirection  = pwi->Gradient.ulDirection;
         }
         return TRUE;
      case PATSYM_FOUNTAINFILL:
         if (bDialog)
         {
            if (WinDlgBox(HWND_DESKTOP,pwi->hwndClient,
                         (PFNWP)FountainDlgProc,0L,
                         ID_DLGFOUNTAIN,(PVOID)&pBox->fountain) == DID_OK)
            {
               pBox->lPattern = ulPattern;
               return TRUE;
            }
            else
               return FALSE;
         }
         else
         {
            pBox->lPattern = ulPattern;
            memcpy(&pBox->fountain,&pwi->fountain,sizeof(FOUNTAIN));
         }
         break;
      default:
         pBox->lPattern = ulPattern;
         break;
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
VOID boxMake(POBJECT pObj, WINDOWINFO *pwi, POINTL ptle, POINTL ptls, ULONG msg)
{
   PBOX   pBox = (PBOX)pObj;

   GpiSetLineType(pwi->hps, pBox->line.LineType);
   GpiSetMix(pwi->hps,FM_INVERT);
   GpiMove(pwi->hps, &ptls);
   GpiBox(pwi->hps,DRO_OUTLINE,&ptle,pwi->lRound,pwi->lRound);

   if (msg == WM_BUTTON1UP)
   {
      POINTLF ptlf;
      /* 
      ** Get the final point where we released the mouse while drawing the 
      ** box.   
      */ 
      pBox->rclf.xRight  = (float)ptle.x;
      pBox->rclf.yTop    = (float)ptle.y;
      
      if ( pBox->rclf.xRight < pBox->rclf.xLeft)
      {
         ptlf.x = pBox->rclf.xLeft;
         pBox->rclf.xLeft  = pBox->rclf.xRight;
         pBox->rclf.xRight = ptlf.x;
      }

      if ( pBox->rclf.yTop < pBox->rclf.yBottom)
      {
         ptlf.y = pBox->rclf.yBottom;
         pBox->rclf.yBottom = pBox->rclf.yTop;
         pBox->rclf.yTop    = ptlf.y;
      }

      pBox->ptlfCenter.x = pBox->rclf.xLeft + 
                           ( pBox->rclf.xRight - pBox->rclf.xLeft )/2;

      pBox->ptlfCenter.y = pBox->rclf.yBottom + 
                           ( pBox->rclf.yTop - pBox->rclf.yBottom )/2;

      pBox->rclf.xRight  /= (float)pwi->usFormWidth ;
      pBox->rclf.yTop    /= (float)pwi->usFormHeight;
      pBox->rclf.xLeft   /= (float)pwi->usFormWidth ;
      pBox->rclf.yBottom /= (float)pwi->usFormHeight;
      pBox->ptlfCenter.x /= (float)pwi->usFormWidth ;
      pBox->ptlfCenter.y /= (float)pwi->usFormHeight;
   }
   return;
}

void boxDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg,POINTL *pt)
{
   static POINTL ptl1,ptl2,ptlCenter;
   MATRIXLF matlf;
   LONG deg;

   PBOX pBox = (PBOX)pObj;

   if (!pObj) return;

   switch (ulMsg)
   {
      case WM_BUTTON1UP:
         deg = (LONG)((lRotate * 360)/(2 * PI));  /* Make it degrees      */
         pBox->lRotate += deg;
         break;

      case WM_MOUSEMOVE:
         ptl1.y  = (LONG)(pwi->fOffy);
         ptl1.x  = (LONG)(pwi->fOffx);
         ptl1.x += (LONG)(pBox->rclf.xLeft   * pwi->usFormWidth);
         ptl1.y += (LONG)(pBox->rclf.yBottom * pwi->usFormHeight);

         ptl2.y  = (LONG)(pwi->fOffy);
         ptl2.x  = (LONG)(pwi->fOffx);
         ptl2.x += (LONG)(pBox->rclf.xRight  * pwi->usFormWidth);
         ptl2.y += (LONG)(pBox->rclf.yTop    * pwi->usFormHeight);

         if (!pt)
         {
            ptlCenter.x = (LONG)(pBox->ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pBox->ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter = *pt;

         deg = (LONG)((lRotate * 360)/(2 * PI));  /* Make it degrees      */
         deg += pBox->lRotate;
         GpiRotate(pwi->hps,&matlf,TRANSFORM_REPLACE,
                   MAKEFIXED(deg,0),&ptlCenter);

         GpiSetModelTransformMatrix(pwi->hps,9L,&matlf,TRANSFORM_REPLACE);
         GpiMove(pwi->hps,&ptl1);
         GpiBox(pwi->hps,DRO_OUTLINE, &ptl2,pBox->lHRound,pBox->lVRound);
         GpiSetModelTransformMatrix(pwi->hps,9L,&pwi->matOrg,TRANSFORM_REPLACE);
         break;
   }
   return;
}
/*------------------------------------------------------------------------*/
void RegisterBox(HAB hab)
{
   WinRegisterClass( hab,
                    (PSZ)szBoxClass,   // Name of class being registered
                    (PFNWP)boxWndProc, // Window procedure for class
                    CS_SIZEREDRAW,     // Class style
                    (BOOL)0);          // Extra bytes to reserve
   return;
}
/*------------------------------------------------------------------------*/
void boxDetails(WINDOWINFO *pwi,HWND hOwner,POBJECT pObj)
{
   LONG lResult;

   if (!pObj )
      return;

   if (pObj->usClass != CLS_BOX)
      return;

   lResult = WinDlgBox(HWND_DESKTOP,hOwner,(PFNWP)boxDlgProc,
                        (HMODULE)0,ID_BOXDETAIL,(PVOID)pObj);

   if (lResult == DID_OK)
       ObjRefresh(pObj,pwi);
   return;
}
/*------------------------------------------------------------------------*/
/* Set the rounding program wide. Called directly from attributes menu    */
/*------------------------------------------------------------------------*/
MRESULT boxRounding(WINDOWINFO *pwi,HWND hOwner)
{
   WinDlgBox(HWND_DESKTOP,hOwner,(PFNWP)roundDlgProc,
            (HMODULE)0,ID_BOXDETAIL,(PVOID)&(pwi->lRound));
   return (MRESULT)0;
}
