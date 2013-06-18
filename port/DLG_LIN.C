/*------------------------------------------------------------------------*/
/*  Name: dlg_lin.c                                                       */
/*                                                                        */
/*  Description : Functions for handling single lines.                    */
/*                                                                        */
/*  Functions  :                                                          */
/*     LinGetCenter  : Get the coords of the rotating center.             */
/*     LinSetCenter  : Set the coords of the rotating center.             */
/*     LinDrawRotate : Draw the line during rotation. (ONLY WHEN PART OF  */
/*                     GROUP).                                            */
/*     linDrawOutline: Draws the line without changing attrs [public]     */
/*     DrawLine      : Used during the creation of the line.              */
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
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwarr.h"        /* Arrows..... */	
#include "dlg_lin.h"
#include "dlg_hlp.h"
#include "resource.h"
#include "drwutl.h"
#include "dlg_sqr.h"
#define LINESELSIZE     15


#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#define MARGIN         4
/*------------------------------------------------------------------------*/
pLines OpenLineSegment(POINTL ptlStart, WINDOWINFO *pwi)
{
   pLines pLin;
   POBJECT pObj;
   pLin                 = (pLines)pObjCreate(pwi, CLS_LIN);
   pLin->ustype         = CLS_LIN;
   pLin->ptl1.x         = (float)(ptlStart.x);
   pLin->ptl1.y         = (float)(ptlStart.y);

   pObj = (POBJECT)pLin;
   /*
   ** Our paint and move routine....
   */
   pObj->paint			= DrawLineSegment;
   pObj->moveOutline	        = LineMoving;
   pObj->getInvalidationArea    = LineInvArea;
   return pLin;
}
/*------------------------------------------------------------------------*/
pLines CloseLineSegment(pLines pLin,POINTL ptlEnd, WINDOWINFO *pwi)
{
    pLin->ptl2.x     = (float)ptlEnd.x;
    pLin->ptl2.y     = (float)ptlEnd.y;
    pLin->ptl1.y     = (float)pLin->ptl1.y;
    pLin->ptl1.x     = (float)pLin->ptl1.x;

    pLin->ptl2.x  /= (float) pwi->usFormWidth ;
    pLin->ptl2.y  /= (float) pwi->usFormHeight;
    pLin->ptl1.y  /= (float) pwi->usFormHeight;
    pLin->ptl1.x  /= (float) pwi->usFormWidth ;

    return pLin;
}
static void drawLine(pLines pLin, HPS hps,WINDOWINFO *pwi,POINTL ptl1, POINTL ptl2)
{

      GpiSetLineType(hps,pLin->bt.line.LineType);
      GpiMove(hps, &ptl1);
      if (pLin->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         LONG lLineWidth = pLin->bt.line.LineWidth;

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }
         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pLin->bt.line.LineJoin);
         GpiSetLineEnd(hps,pLin->bt.line.LineEnd);
         GpiSetLineWidthGeom (hps,lLineWidth);
         GpiBeginPath( hps, 1L);  /* define a clip path    */
      }
      GpiLine(hps,&ptl2);

      if (pwi->uXfactor >= (float)1)
      {
         GpiSetLineType(hps,LINETYPE_SOLID);
         drwEndPoints(pwi,pLin->bt.arrow,ptl1,ptl2);
      }

      if (pLin->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         GpiEndPath(hps);
         GpiStrokePath (hps, 1, 0);
      }
}

static void lineShadow(pLines pLin,HPS hps,WINDOWINFO *pwi, POINTL ptl1, POINTL ptl2)
{
   POINTL p[2];

   p[0] = ptl1;
   p[1] = ptl2;
   setShadingOffset(pwi,pLin->bt.Shade.lShadeType, 
                    pLin->bt.Shade.lUnits,p,2);

   GpiSetLineType(hps,pLin->bt.line.LineType);
   GpiSetColor(hps,pLin->bt.Shade.lShadeColor);
   drawLine(pLin,hps,pwi,p[0],p[1]);
}
/*------------------------------------------------------------------------*/
void DrawLineSegment(HPS hps, WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl)
{
   POINTL ptl1;
   POINTL ptl2;
   pLines pLin;

   pLin = (pLines)pObj;

   GpiSetMix(hps,FM_DEFAULT);

   if (pwi->usdrawlayer == pLin->bt.usLayer)
   {
      ptl1.x = (LONG)(pLin->ptl1.x * pwi->usFormWidth );
      ptl1.y = (LONG)(pLin->ptl1.y * pwi->usFormHeight);
      ptl1.x +=(LONG)pwi->fOffx;
      ptl1.y +=(LONG)pwi->fOffy;

      ptl2.x = (LONG)(pLin->ptl2.x * pwi->usFormWidth );
      ptl2.y = (LONG)(pLin->ptl2.y * pwi->usFormHeight);
      ptl2.x +=(LONG)pwi->fOffx;
      ptl2.y +=(LONG)pwi->fOffy;

      if (pLin->bt.Shade.lShadeType != SHADE_NONE)
         lineShadow(pLin,hps,pwi,ptl1,ptl2);

      GpiSetLineType(hps,pLin->bt.line.LineType);
      GpiSetColor(hps,pLin->bt.line.LineColor);
      drawLine(pLin,hps,pwi,ptl1,ptl2);
   }     /*layer stuff */
}
/*------------------------------------------------------------------------*/
void LineOutline(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi)
{
    POINTL ptlSt,ptlEnd;
    pLines pLin = (pLines)pObj;

    ptlSt.x   = (LONG)(pLin->ptl1.x * pwi->usFormWidth );
    ptlSt.y   = (LONG)(pLin->ptl1.y * pwi->usFormHeight);
    ptlSt.x +=  (LONG)pwi->fOffx;
    ptlSt.y +=  (LONG)pwi->fOffy;

    ptlEnd.x  = (LONG)(pLin->ptl2.x * pwi->usFormWidth );    
    ptlEnd.y  = (LONG)(pLin->ptl2.y * pwi->usFormHeight);
    ptlEnd.x += (LONG)pwi->fOffx;
    ptlEnd.y += (LONG)pwi->fOffy;

    if (ptlSt.x >= ptlEnd.x)
    {
       rcl->xRight = ptlSt.x;
       rcl->xLeft  = ptlEnd.x;
    }
    else
    {
       rcl->xRight = ptlEnd.x;
       rcl->xLeft  = ptlSt.x;
    }    

    if (ptlSt.y >= ptlEnd.y)
    {
       rcl->yTop = ptlSt.y;
       rcl->yBottom = ptlEnd.y;
    }
    else
    {
       rcl->yTop = ptlEnd.y;
       rcl->yBottom = ptlSt.y;
    }
    arrowAreaExtra(pLin->bt.arrow,rcl);
}
/*------------------------------------------------------------------------*/
VOID * LineSelect(POINTL ptlMouse,WINDOWINFO *pwi, POBJECT pObj)
{
   pLines pLin = (pLines)pObj;
   POINTL ptl;

   if (pLin->bt.usLayer == pwi->uslayer || pwi->bSelAll)
   {
      ptl.x = (LONG)(pLin->ptl1.x * pwi->usFormWidth );
      ptl.y = (LONG)(pLin->ptl1.y * pwi->usFormHeight);
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;

      GpiMove(pwi->hps, &ptl);
      ptl.x = (LONG)(pLin->ptl2.x * pwi->usFormWidth );
      ptl.y = (LONG)(pLin->ptl2.y * pwi->usFormHeight);
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;

      if (GpiLine(pwi->hps,&ptl) == GPI_HITS)
            return (void *)pLin;
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
void LineInvArea(POBJECT pLin, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc)
{

    LineOutline((POBJECT)pLin,rcl,pwi);

    /*
    ** To draw a square even around a line
    ** add pixels.
    */
    GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);
    if (bInc)
    {
       rcl->yTop    += LINESELSIZE;
       rcl->yBottom -= LINESELSIZE;
       rcl->xLeft   -= LINESELSIZE;
       rcl->xRight  += LINESELSIZE;
    }
}
/*------------------------------------------------------------------------*/
/* True if the mouse is on one of the sides of the line.                  */
/* linptl return the linepoint which is having the mousepointer focus.    */
/* Return on which the pointer was found. 1= ptl1 first point of the line */
/*                                        2= ptl2 sec point of the line   */
/*                                        0= No point found!		  */
/*------------------------------------------------------------------------*/
BOOL IsOnLineEnd(POINTL mouseptl, pLines pLin,WINDOWINFO *pwi, USHORT *corner )
{
   POINTL ptl1,ptl2;

   if (!pLin)
      return FALSE;

   ptl1.x = (LONG)(pLin->ptl1.x * pwi->usFormWidth );
   ptl1.y = (LONG)(pLin->ptl1.y * pwi->usFormHeight);
   ptl1.x += (LONG)pwi->fOffx;
   ptl1.y += (LONG)pwi->fOffy;

   ptl2.x =  (LONG)(pLin->ptl2.x * pwi->usFormWidth );    
   ptl2.y =  (LONG)(pLin->ptl2.y * pwi->usFormHeight);
   ptl2.x += (LONG)pwi->fOffx;
   ptl2.y += (LONG)pwi->fOffy;

   if (mouseptl.x >= ( ptl1.x - 5 ) &&
       mouseptl.x <= ( ptl1.x + 5 ) &&
       mouseptl.y >= ( ptl1.y - 5 ) &&
       mouseptl.y <= ( ptl1.y + 5 ))
    {
       *corner = 1;
       WinSetPointer(HWND_DESKTOP,
                     WinQuerySysPointer(HWND_DESKTOP,
                                        SPTR_SIZENWSE,FALSE));
       return TRUE;
    }
    else if (mouseptl.x >= ( ptl2.x - 5 ) &&
             mouseptl.x <= ( ptl2.x + 5 ) &&
             mouseptl.y >= ( ptl2.y - 5 ) &&
             mouseptl.y <= ( ptl2.y + 5 ))
    {
       *corner = 2;
       WinSetPointer(HWND_DESKTOP,
                     WinQuerySysPointer(HWND_DESKTOP,
                                        SPTR_SIZENWSE,FALSE));

       return TRUE;
    }
    return FALSE;
}
/*------------------------------------------------------------------------*/
/* LineMoving function, if we are selected via the mouse .....            */
/*------------------------------------------------------------------------*/
VOID LineMoving(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy)
{
   POINTL ptl1;
   POINTL ptl2;
   pLines pLin = (pLines)pObj;

   ptl1.x = (LONG)(pLin->ptl1.x * pwi->usFormWidth ) + (LONG)dx;
   ptl1.y = (LONG)(pLin->ptl1.y * pwi->usFormHeight) + (LONG)dy;
   ptl1.x += (LONG)pwi->fOffx;
   ptl1.y += (LONG)pwi->fOffy;
   GpiMove(pwi->hps, &ptl1);

   ptl2.x = (LONG)(pLin->ptl2.x * pwi->usFormWidth ) + (LONG)dx;
   ptl2.y = (LONG)(pLin->ptl2.y * pwi->usFormHeight) + (LONG)dy;
   ptl2.x += (LONG)pwi->fOffx;
   ptl2.y += (LONG)pwi->fOffy;
   GpiLine(pwi->hps,&ptl2);

   drwEndPoints(pwi,pLin->bt.arrow,ptl1,ptl2);
   return;   
}
/*-----------------------------------------------[ public ]---------------*/
/* linDrawOutline                                                         */
/*------------------------------------------------------------------------*/
void linDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer)
{
   pLines pLin = (pLines)pObj;

   if (pLin->bt.usLayer == pwi->uslayer || bIgnorelayer )
      LineMoving(pObj,pwi,0,0);
   return;
}
/*------------------------------------------------------------------------*/
void MoveLinSegment(POBJECT pObj, SHORT dx, SHORT dy,WINDOWINFO *pwi)
{
   float fdx,fdy,fcx,fcy;
   pLines pLin = (pLines)pObj;
   if (!pLin)
      return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;

   pLin->ptl1.x += (fdx /fcx);
   pLin->ptl1.y += (fdy /fcy);
   pLin->ptl2.x += (fdx /fcx);
   pLin->ptl2.y += (fdy /fcy);
}
/*------------------------------------------------------------------------*/
/* Insert the line in a given group object.                               */
/*------------------------------------------------------------------------*/
void LinPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pLines pLin = (pLines)pObj;
   USHORT usGrpWidth,usGrpHeight;

   if (!pLin) return;

   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   pLin->ptl1.x *= (float)pwi->usFormWidth;
   pLin->ptl1.y *= (float)pwi->usFormHeight;
   pLin->ptl2.x *= (float)pwi->usFormWidth;    
   pLin->ptl2.y *= (float)pwi->usFormHeight;

   pLin->ptl1.x -= (float)prcl->xLeft;
   pLin->ptl1.y -= (float)prcl->yBottom;
   pLin->ptl2.x -= (float)prcl->xLeft;
   pLin->ptl2.y -= (float)prcl->yBottom;

   pLin->ptl1.x /= (float)usGrpWidth;
   pLin->ptl1.y /= (float)usGrpHeight;
   pLin->ptl2.x /= (float)usGrpWidth;
   pLin->ptl2.y /= (float)usGrpHeight;

}
/*------------------------------------------------------------------------*/
/*  LinRemFromGroup.                                                      */
/*                                                                        */
/*  Remove the object from the group.                                     */
/*------------------------------------------------------------------------*/
void LinRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pLines pLin = (pLines)pObj;
   USHORT usGrpWidth,usGrpHeight;

   if (!pLin) return;

   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   pLin->ptl1.x *= (float)usGrpWidth;
   pLin->ptl1.y *= (float)usGrpHeight;
   pLin->ptl2.x *= (float)usGrpWidth;
   pLin->ptl2.y *= (float)usGrpHeight;

   pLin->ptl1.x += (float)prcl->xLeft;
   pLin->ptl1.y += (float)prcl->yBottom;
   pLin->ptl2.x += (float)prcl->xLeft;
   pLin->ptl2.y += (float)prcl->yBottom;

   pLin->ptl1.x /= (float)pwi->usFormWidth;
   pLin->ptl1.y /= (float)pwi->usFormHeight;
   pLin->ptl2.x /= (float)pwi->usFormWidth;    
   pLin->ptl2.y /= (float)pwi->usFormHeight;
}
/*------------------------------------------------------------------------*/
/* LinDrawRotate.                                                         */
/*                                                                        */
/* Description : Draws the line while it is rotated with the mouse.       */
/*               This function differs from the one's for the other obj's */
/*               since this one is only called when the line is part of   */
/*               a group!                                                 */
/*                                                                        */
/* Returns     :  NONE.                                                   */
/*------------------------------------------------------------------------*/
void LinDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg,POINTL *pt)
{
   POINTL ptlCenter,ptl[2];

   pLines pLin = (pLines)pObj;

   if (!pObj) return;

   switch (ulMsg)
   {
      case WM_BUTTON1UP:
         if (!pt)
            return; /* only accept rotation in groups */

         ptlCenter = *pt;

         ptl[0].x = pLin->ptl1.x * pwi->usFormWidth;
         ptl[0].y = pLin->ptl1.y * pwi->usFormHeight;
         ptl[1].x = pLin->ptl2.x * pwi->usFormWidth;    
         ptl[1].y = pLin->ptl2.y * pwi->usFormHeight;

         ptl[0].x += pwi->fOffx;
         ptl[0].y += pwi->fOffy;
         ptl[1].x += pwi->fOffx;
         ptl[1].y += pwi->fOffy;

         RotateSqrSegment(lRotate,ptlCenter,ptl,2);

         ptl[0].x -= pwi->fOffx;
         ptl[0].y -= pwi->fOffy;
         ptl[1].x -= pwi->fOffx;
         ptl[1].y -= pwi->fOffy;

         pLin->ptl1.x = (float)ptl[0].x;
         pLin->ptl1.y = (float)ptl[0].y;
         pLin->ptl2.x = (float)ptl[1].x;
         pLin->ptl2.y = (float)ptl[1].y;
         pLin->ptl1.x /= (float)pwi->usFormWidth;
         pLin->ptl1.y /= (float)pwi->usFormHeight;
         pLin->ptl2.x /= (float)pwi->usFormWidth;
         pLin->ptl2.y /= (float)pwi->usFormHeight;
         break;
      case WM_MOUSEMOVE:
         if (!pt)
            return; /* only accept rotation in groups */

         ptlCenter = *pt;

         ptl[0].x = pLin->ptl1.x * pwi->usFormWidth;
         ptl[0].y = pLin->ptl1.y * pwi->usFormHeight;
         ptl[1].x = pLin->ptl2.x * pwi->usFormWidth;    
         ptl[1].y = pLin->ptl2.y * pwi->usFormHeight;

         ptl[0].x += pwi->fOffx;
         ptl[0].y += pwi->fOffy;
         ptl[1].x += pwi->fOffx;
         ptl[1].y += pwi->fOffy;

         RotateSqrSegment(lRotate,ptlCenter,ptl,2);
         GpiMove(pwi->hps, &ptl[0]);
         GpiLine(pwi->hps, &ptl[1]);
         break;
   }
   return;
}
/*----------------------------------------------------------------------*/
/* Drawline.                                                            */
/*                                                                      */
/* Description  : Called during the creation of the line.               */
/*                see drwmain.c                                         */
/*----------------------------------------------------------------------*/
VOID DrawLine(WINDOWINFO *pwi,POINTL ptlSt,POINTL ptlE, short mode,POBJECT pObj)
{
   pLines pLin;

   if (pObj && pObj->usClass == CLS_LIN)
      pLin = (pLines)pObj;
   else
      pLin = (pLines)0;

   if ( mode == CREATEMODE)
      GpiSetMix(pwi->hps,FM_INVERT);
   else
   {
      GpiSetMix(pwi->hps,FM_DEFAULT);
      GpiSetColor(pwi->hps,pwi->ulOutLineColor);
      GpiSetLineType(pwi->hps,pwi->lLntype);
   }

  if ( mode != CREATEMODE && pwi->lLnWidth > 1)
  {
     GpiSetPattern(pwi->hps,PATSYM_SOLID);
     GpiSetLineJoin(pwi->hps,pwi->lLnJoin);
     GpiSetLineEnd(pwi->hps,pwi->lLnEnd);
     GpiSetLineWidthGeom (pwi->hps,pwi->lLnWidth);
     GpiBeginPath( pwi->hps, 1L);
  }

  GpiMove(pwi->hps,&ptlSt);
  GpiLine(pwi->hps,&ptlE);

  if ( mode != CREATEMODE && pwi->lLnWidth > 1)
  {
     GpiEndPath(pwi->hps);
     GpiStrokePath (pwi->hps, 1, 0);
  }
  
  if (pLin)
     drwEndPoints(pwi,pLin->bt.arrow,ptlSt,ptlE);
  else
     drwEndPoints(pwi,pwi->arrow,ptlSt,ptlE);
}

