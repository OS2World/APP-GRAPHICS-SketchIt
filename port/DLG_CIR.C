/*------------------------------------------------------------------------*/
/*  Name: dlg_cir.c                                                       */
/*                                                                        */
/*  Description : Functions for handling a circle.                        */
/*                                                                        */
/*  Functions  :                                                          */
/*     CirShowHandles : Show form change handle.                          */
/*     CirPutInGroup  : Put the given object in the group.                */
/*     CirRemFromGroup: Remove the given object from the group.           */
/*     CirStretch     : Stretch the circle.                               */
/*     cirDrawOutline : Draws the outline of the circle.                  */
/*                                                                        */
/* Private Functions:                                                     */
/*     CirGetClippingRect :Get the clipping rect on a printer PS.         */
/*     cirCalcOutLine     :Get the circle outline.                        */
/*                                                                        */
/*                                                                        */
/*--ch---date---version-------description---------------------------------*/
/*  1   180598  2.8      Used the multiplier var for bClose.              */
/*  2   020698  2.8      Removed the partial creation func.               */
/*  3   191198  2.9      Point editing of partial arc show complete arc.  */
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
#include <float.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwarr.h"
#include "dlg_hlp.h"
#include "dlg_sqr.h"
#include "dlg_cir.h"
#include "drwutl.h"
#include "drwfount.h"
#include "dlg_clr.h"
#include "resource.h"

#define PI 3.1415926

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#define MAX_SWEEPANGLE 360
#define MIN_SWEEPANGLE   5

#define MAX_STARTANGLE 360
#define MIN_STARTANGLE   0
/*
** We need to know what type of cic we are supposed to draw
** during development.
*/
static SHORT usType;
/*
** During editing of the selected circle this
** structure is used.
*/
typedef struct _defcir
{
   ULONG ulLayer;
   ULONG ulSweepAngle;
   ULONG ulStartAngle;
   BOOL  bClose;
   BOOL  bCirc;
   BOOL  bDialogActive; /* Is the dialog already running? */
} defcircle;

static defcircle defCir; /* development / editing a circle */

/*------------------------------------------------------------------------*/
void cirGetRealCenter(WINDOWINFO *pwi,POBJECT pObj, POINTL *pptl)
{
   pCircle pCir = (pCircle)pObj;
   pptl->x = (long)(pCir->ptlPosn.x * pwi->usFormWidth);
   pptl->y = (long)(pCir->ptlPosn.y * pwi->usFormHeight);
}
/*------------------------------------------------------------------------*/
void cirRestoreCenter(WINDOWINFO *pwi,POBJECT pObj)
{
   pCircle pCir = (pCircle)pObj;
   pCir->bt.ptlfCenter.x = pCir->ptlPosn.x;
   pCir->bt.ptlfCenter.y = pCir->ptlPosn.y;
}
/*------------------------------------------------------------------------*/
static BOOL bHasAreaFill(pCircle pCir)
{
      if (pCir->bt.lPattern != PATSYM_DEFAULT       &&
          pCir->bt.lPattern != PATSYM_GRADIENTFILL  &&
          pCir->bt.lPattern != PATSYM_FOUNTAINFILL )
          return TRUE;
       else
          return FALSE;
}
/*------------------------------------------------------------------------*/
static BOOL bHasClipFill( pCircle pCir)
{
   if (pCir->bt.lPattern == PATSYM_GRADIENTFILL ||
       pCir->bt.lPattern == PATSYM_FOUNTAINFILL)
       return TRUE;
    else
       return FALSE;
}
/*------------------------------------------------------------------------*/
static VOID QueryArcParams(ARCPARAMS *arcp,ARCFPARAMS *af,WINDOWINFO *pwi,pCircle pCir )
{
   /*
   ** If we are a full circle or ellipse, e.g sweepangle
   ** is 360 degrees, then look if we are rotated.
   ** Important for ellipses, not for circles.
   */
   if (pCir->SweepAngle == MAKEFIXED(360,0))
   {
      af->lR = ( pCir->arcpParms.lR * pwi->usFormWidth );
      af->lS = ( pCir->arcpParms.lS * pwi->usFormHeight);
      af->lP = ( pCir->arcpParms.lP * pwi->usFormWidth );
      af->lQ = ( pCir->arcpParms.lQ * pwi->usFormHeight);
   }
   else   /* Just a partial arc.... */
   {
         af->lR = ( pCir->arcpParms.lR * pwi->usFormWidth );
         af->lS = ( pCir->arcpParms.lS * pwi->usFormHeight);
         af->lP = ( pCir->arcpParms.lP * pwi->usFormWidth );
         af->lQ = ( pCir->arcpParms.lQ * pwi->usFormHeight);
   }
   arcp->lR = (LONG) af->lR;
   arcp->lS = (LONG) af->lS;
   arcp->lP = (LONG) af->lP;
   arcp->lQ = (LONG) af->lQ;
}
/*-----------------------------------------------[ private ]--------------*/
/* Get clipping rect for printing. The GpiBoundary functions do not       */
/* function correctly. So here is the workareound.                        */
/*------------------------------------------------------------------------*/
static void CirGetClippingRect(POBJECT pObj,WINDOWINFO *pwi,RECTL *rcl)
{
   rcl->xLeft    =  (LONG)(pObj->rclf.xLeft   * pwi->usFormWidth );
   rcl->xRight   =  (LONG)(pObj->rclf.xRight  * pwi->usFormWidth );
   rcl->yBottom  =  (LONG)(pObj->rclf.yBottom * pwi->usFormHeight);
   rcl->yTop     =  (LONG)(pObj->rclf.yTop    * pwi->usFormHeight);
   return;
}
/*------------------------------------------------------------------------*/
static void cirDrawArrows(WINDOWINFO *pwi,pCircle pCir,
                          POINTL ptlStart,POINTL ptlEnd)
{
   LONG       lAngle;
   LONG       lPointAngle;
   ARCPARAMS  arcParms;
   ARCFPARAMS af;       /* Floating point version (notused)*/
   POINTL     ptl1,ptl2;

   /*
   ** Start of arrow stuff........
   */
   if (!pCir->m_bClose && pCir->SweepAngle <= MAKEFIXED(350,0))
   {
      lPointAngle = 0L;

      QueryArcParams(&arcParms,&af,pwi,pCir);
      ptl1.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
      ptl1.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
      ptl2.x = ptl1.x + arcParms.lP;
      ptl2.y = ptl1.y + arcParms.lS;

      lPointAngle =  angleFromPoints( ptl1, ptl2 );

      lAngle = pCir->StartAngle;
      lAngle = lAngle >> 16;
      lAngle -= 90;
      lAngle += lPointAngle;

      if (pCir->bt.arrow.lStart != DEF_LINESTART)
         drwStartPtAtAngle(pwi,pCir->bt.arrow,lAngle,ptlStart);

      ptl1.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth );
      ptl1.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);

      ptl2.x = ptl1.x + arcParms.lR;
      ptl2.y = ptl1.y + arcParms.lQ;

      lPointAngle =  angleFromPoints( ptl1, ptl2 );


      lAngle = pCir->SweepAngle + pCir->StartAngle;
      lAngle = lAngle >> 16;
      lAngle += 90;
      lAngle += (lPointAngle + 90);

      if (pCir->bt.arrow.lStart != DEF_LINEEND)
         drwEndPtAtAngle(pwi,pCir->bt.arrow,lAngle,ptlEnd);
   }
   /*
   ** End of arrow stuff.......
   */
}
/*------------------------------------------------------------------------*/
pCircle OpenCircleSegment(WINDOWINFO *pwi)
{
   pCircle pCir;
   POBJECT pObj;
   pCir = (pCircle)pObjCreate(pwi,CLS_CIR);

   if (!pCir)
   {
       return (pCircle)0;
   }
   pCir->ustype        = CLS_CIR;
   pCir->StartAngle = 0x00000000L;
   pCir->SweepAngle = pwi->fxArc;
   pCir->m_bClose     = TRUE;
   pObj = (POBJECT)pCir;
   /*
   ** Our paint and move routine....
   */
   pObj->paint			= DrawCircleSegment;
   pObj->moveOutline	        = CirMoveOutLine;
   pObj->getInvalidationArea    = CircleInvArea;
   return pCir;
}
/*-----------------------------------------------------------------------*/
BOOL cirCheck(POBJECT pObj)
{
   pCircle pCir = (pCircle )pObj;

   if (pCir->arcpParms.lP < FLT_MIN &&  pCir->arcpParms.lP != (float)0)
      return FALSE;
   if (pCir->arcpParms.lQ < FLT_MIN &&  pCir->arcpParms.lQ != (float)0)
      return FALSE;
   if (pCir->arcpParms.lR < FLT_MIN &&  pCir->arcpParms.lR != (float)0 )
      return FALSE;
   if (pCir->arcpParms.lS < FLT_MIN &&  pCir->arcpParms.lS != (float)0 )
      return FALSE;

   return TRUE;
}
/*------------------------------------------------------------------------*/
/* x in units of 0.1 mm, y in units of 0.1 mm                             */
/*------------------------------------------------------------------------*/
POBJECT CloseCircleSegment(pCircle pCir,POINTL ptlStart, POINTL ptlEnd, WINDOWINFO *pwi)
{
   POINTL ptlPosn;

   /*
   ** Get the final point where we released the mouse while drawing the
   ** Circle.
   */
   ptlPosn.x = abs(ptlEnd.x);   /* radius in x direction in pixels */
   ptlPosn.y = abs(ptlEnd.y);   /* radius in y direction in pixels */

   pCir->arcpParms.lP = (float)ptlPosn.x;
   pCir->arcpParms.lQ = (float)ptlPosn.y;

   pCir->arcpParms.lP /= (float)pwi->usFormWidth ;
   pCir->arcpParms.lQ /= (float)pwi->usFormHeight;

   pCir->ptlPosn.x  = (float)ptlStart.x;
   pCir->ptlPosn.y  = (float)ptlStart.y;

   pCir->ptlPosn.x  /= (float)pwi->usFormWidth;
   pCir->ptlPosn.y  /= (float)pwi->usFormHeight;
   pCir->bt.ptlfCenter.x = pCir->ptlPosn.x;
   pCir->bt.ptlfCenter.y = pCir->ptlPosn.y;

   pCir->arcpParms.lR = 0L;
   pCir->arcpParms.lS = 0L;
   if (pwi->fxArc != MAKEFIXED(360,0) )
   {
      pCir->SweepAngle = pwi->fxArc;
   }

   /*
   ** Check the circle and if there is floating point
   ** underflow delete it. Else an program exception
   ** will be raised.
   */
   if (!cirCheck((POBJECT)pCir))
   {
      ObjDelete((POBJECT)pCir);
      return NULL;
   }
   else
      return (POBJECT)pCir;
}
/*------------------------------------------------------------------------*/
/* CirShowHandle.                                                         */
/*                                                                        */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID CirShowHandles(POBJECT pObj, WINDOWINFO *pwi)
{
   ARCPARAMS  arcParms;
   ARCFPARAMS af;       /* Floating point version (notused)*/
   pCircle pCir = (pCircle)pObj;
   POINTL ptl;

   QueryArcParams(&arcParms,&af,pwi,pCir);

   ptl.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
   ptl.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);

   ptl.x += arcParms.lP;
   ptl.y += arcParms.lS;

   DrawMovePointHandle(pwi->hps,ptl.x,ptl.y);

   ptl.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth );
   ptl.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);

   ptl.x += arcParms.lR;
   ptl.y += arcParms.lQ;

   DrawMovePointHandle(pwi->hps,ptl.x,ptl.y);
}
/*------------------------------------------------------------------------*/
static void drawCircleAtPoint(pCircle pCir, WINDOWINFO *pwi, POINTL ptlCenter)
{
   POINTL     ptlEnd,ptlStart;

   ARCPARAMS  arcParms;  /* used for rotation               */
   ARCFPARAMS af;        /* Floating point version (notused)*/
   LONG       lLineType; /* Remember old linetype           */

   GpiSetCurrentPosition(pwi->hps,&ptlCenter);

   lLineType = GpiQueryLineType(pwi->hps);

   GpiSetLineType(pwi->hps,LINETYPE_INVISIBLE);

   QueryArcParams(&arcParms,&af,pwi,pCir);
   GpiSetArcParams(pwi->hps,&arcParms);
   if (pCir->SweepAngle == MAKEFIXED(360,0))
   {
      pCir->StartAngle = 0;
      GpiPartialArc(pwi->hps,&ptlCenter,0x0000ff00,pCir->StartAngle,
                    pCir->SweepAngle);
   }
   else
   {
      GpiPartialArc(pwi->hps,&ptlCenter,0x0000ff00,pCir->StartAngle,
                    MAKEFIXED(360,0));
   }

   /* Starting point used for arrows */
   GpiQueryCurrentPosition(pwi->hps,&ptlStart); 

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose )
      GpiSetCurrentPosition(pwi->hps,&ptlCenter);

   GpiSetLineType(pwi->hps,lLineType);
   GpiPartialArc(pwi->hps,&ptlCenter,0x0000ff00,pCir->StartAngle,
                 pCir->SweepAngle);

   /* End point used for arrows */
   GpiQueryCurrentPosition(pwi->hps,&ptlEnd); 

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
      GpiLine(pwi->hps,&ptlCenter);
   /*
   ** Start of arrow stuff........
   */
   cirDrawArrows(pwi,pCir,ptlStart,ptlEnd);
}
/*-----------------------------------------------[ public ]---------------*/
/*  CirMoveOutline.                                                       */
/*                                                                        */
/*  Description : Called during the circle drag operation.                */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID CirMoveOutLine (POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   POINTL ptl;
   pCircle pCir = (pCircle)pObj;

   ptl.y  = (ULONG)(pwi->fOffy);
   ptl.x  = (ULONG)(pwi->fOffx);
   ptl.x += (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
   ptl.y += (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
   ptl.x += dx;
   ptl.y += dy;
   drawCircleAtPoint(pCir,pwi,ptl);
   return;
}
/*-----------------------------------------------[ private ]--------------*/
/*  cirShadow.                                                            */
/*                                                                        */
/*  Description : Draws the shadow of the circle.                         */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void cirShadow(pCircle pCir, HPS hps, WINDOWINFO *pwi, POINTL ptlC)
{
   POINTL  ptl;
   POINTL  ptlCenter;
   ARCPARAMS  arcParms; /* used for rotation */
   ARCFPARAMS af;       /* Floating point version (notused)*/
   BOOL       bClipFill,bAreaFill;

   ptlCenter = ptlC;

   setShadingOffset(pwi,pCir->bt.Shade.lShadeType, 
                    pCir->bt.Shade.lUnits,&ptlCenter,1);

   ptl = ptlCenter;

   bAreaFill = bHasAreaFill(pCir);
   bClipFill = bHasClipFill(pCir);

   if (bClipFill || bAreaFill)
      bAreaFill = TRUE;

   QueryArcParams(&arcParms,&af,pwi,pCir);
   GpiSetCurrentPosition(hps,&ptlCenter);
   GpiSetLineType(hps,LINETYPE_INVISIBLE);


   if (pCir->SweepAngle == MAKEFIXED(360,0))
   {
      /*
      ** Set ptl at outline of circle for the FULL circle
      */
      GpiSetArcParams(hps,&arcParms);
      GpiPartialArc(hps,&ptl,0x0000ff00,pCir->StartAngle,
                    pCir->SweepAngle);
   }
   else if (!pCir->m_bClose)
   {
      /*
      ** Set ptl at outline for partial open circle
      */
      GpiSetArcParams(hps,&arcParms);
      GpiPartialArc(hps,&ptl,0x0000ff00,
                    pCir->StartAngle,MAKEFIXED(360,0));
   }
   /*
   ** Make the linetype visible?
   */
   GpiSetLineType(hps,pCir->bt.line.LineType);
   GpiSetPattern(hps, pCir->bt.lPattern);
   GpiSetColor(hps,pCir->bt.Shade.lShadeColor);
   /*
   ** If the outlinecolor is the same as the filling
   ** color let OS/2 draw the boundary
   ** See for detailed comments in dlg_sqr.c...
   */
   if (bAreaFill)
      GpiBeginArea(hps,BA_NOBOUNDARY | BA_ALTERNATE);
   else if (pCir->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
   {
         LONG lLineWidth = pCir->bt.line.LineWidth;

         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pCir->bt.line.LineJoin);
         GpiSetLineEnd(hps,pCir->bt.line.LineEnd);

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }
         GpiSetLineWidthGeom(hps,lLineWidth);
         GpiBeginPath( hps, 1L);  /* define a clip path    */
   }

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
   {
      ptl = ptlCenter;
      GpiSetCurrentPosition(hps,&ptl);
   }
   GpiSetArcParams(hps,&arcParms);

   GpiPartialArc(hps,&ptl,0x0000ff00,
                 pCir->StartAngle,pCir->SweepAngle);

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
      GpiLine(hps,&ptl);

   if (bAreaFill)
      GpiEndArea(hps);
   else if (pCir->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
   {
      GpiEndPath(hps);
      GpiStrokePath (hps, 1, 0);
   }
}
/*-----------------------------------------------[ private ]----------*/
/* DrawCirOutline.   Used for boundary calculations. (circleoutline)  */
/*                                                                    */
/* Description  : Draws the outline of the circle. When bDraw is false*/
/*                the function is called for boundary calculations or */
/*                to check if there is a hit. Selection on outline... */
/*--------------------------------------------------------------------*/
static long DrawCirOutline(WINDOWINFO *pwi, pCircle pCir, BOOL bDraw)
{
   POINTL ptl;
   POINTL ptlCenter;
   LONG   lRet;
   ARCPARAMS  arcParms; /* used for rotation */
   ARCFPARAMS af;       /* Floating point version (notused)*/

   ptl.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
   ptl.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
   ptl.x +=(LONG)pwi->fOffx;
   ptl.y +=(LONG)pwi->fOffy;

   ptlCenter = ptl;

   if (bDraw)
   {
      drawCircleAtPoint(pCir,pwi,ptl);
      return 0L;
   }
   /*
   ** Code is tested by commenting out the above if statement...
   ** So we're called for boundary calculation or
   ** GPI_HITS due to correlation.
   */
   GpiSetCurrentPosition(pwi->hps,&ptlCenter);
   QueryArcParams(&arcParms,&af,pwi,pCir);
   
   GpiSetArcParams(pwi->hps,&arcParms);
   lRet = GpiPartialArc(pwi->hps,&ptl,
                        0x0000ff00, //@ch1 pCir->Multiplier,
                        pCir->StartAngle,
                        pCir->SweepAngle);

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
   {
      LONG lHit;
      lHit = GpiLine(pwi->hps,&ptl);
      if (lHit == GPI_HITS)
         lRet = GPI_HITS;
   }
   return lRet;
}
/*-----------------------------------------------[ private ]-----------------*/
/* Name         : cirCalcOutline                                             */
/*                                                                           */
/* Description  : Calculates the circle outline.                             */
/*---------------------------------------------------------------------------*/
static void cirCalcOutLine(pCircle pCir, RECTL *prclOutline,WINDOWINFO *pwi)
{
   GpiResetBoundaryData(pwi->hps);
   GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_ON);
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
   DrawCirOutline(pwi,pCir,FALSE);
   GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_OFF);
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
   GpiQueryBoundaryData(pwi->hps,prclOutline);
   /*
   ** Add the size of the arrows (if there) when the circle is not
   ** closed.
   */
   if (!pCir->m_bClose  && pCir->SweepAngle <= MAKEFIXED(350,0))
      arrowAreaExtra(pCir->bt.arrow,prclOutline);
}
/*-----------------------------------------------[ public ]----------*/
/* cirDrawOutline.                                                   */
/*-------------------------------------------------------------------*/
void cirDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer)
{
   pCircle pCir = (pCircle)pObj;

   if (pwi->usdrawlayer == pCir->bt.usLayer || bIgnorelayer)
      DrawCirOutline(pwi,(pCircle)pObj, TRUE);
   return;
}
/*-------------------------------------------------------------------*/
void DrawCircleSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl)
{
   POINTL  ptl;
   POINTL  ptlStart,ptlEnd; /* Used for drawing arrows at the endpoints */
   POINTL  ptlCenter;
   BOOL    bClipFill;
   BOOL    bAreaFill;
   RECTL   rcl,rclDest;
   ULONG   flOptions;
   pCircle pCir;
   ARCPARAMS  arcParms; /* used for rotation */
   ARCFPARAMS af;       /* Floating point version (notused)*/

   pCir = (pCircle)pObj;

   if (prcl)
   {
      /*
      ** prcl is only true for screen operations!
      */
      CircleOutLine(pObj,&rcl,pwi,TRUE);
      if (!WinIntersectRect(hab,&rclDest,prcl,&rcl))
         return;
      else if (pObj->bSuppressfill)
      {
         GpiSetColor(hps,pCir->bt.line.LineColor);
         DrawCirOutline(pwi,pCir,TRUE);
         return;
      }
   }

   GpiSetMix(hps,FM_DEFAULT);

   if (pwi->usdrawlayer == pCir->bt.usLayer)
   {
      ptlCenter.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth );
      ptlCenter.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
      ptlCenter.x +=(LONG)pwi->fOffx;
      ptlCenter.y +=(LONG)pwi->fOffy;

      if (pCir->bt.Shade.lShadeType != SHADE_NONE)
         cirShadow(pCir,hps,pwi,ptlCenter);

      ptl = ptlCenter;
      /*
      ** If we are a full circle or ellipse, e.g sweepangle
      ** is 360 degrees, then look if we are rotated.
      ** Important for ellipses, not for circles.
      ** Here we draw with an invisible line to get the
      ** last point in ptl, which is the outline of the arc.
      ** Then we start drawing the arc beginning at the outline
      ** instead of the center. Else we get a line from the center
      ** to the arc.
      ** Since we need this line for a partiale arc, (pie slices)
      ** we put it in an if statement.
      */
      QueryArcParams(&arcParms,&af,pwi,pCir);
      GpiSetCurrentPosition(hps,&ptlCenter);
      GpiSetLineType(hps,LINETYPE_INVISIBLE);

      if (pCir->SweepAngle == MAKEFIXED(360,0))
      {
         /*
         ** Set ptl at outline of circle for the FULL circle
         */
         pCir->StartAngle = 0;
         GpiSetArcParams(hps,&arcParms);
         GpiPartialArc(hps,&ptl,0x0000ff00,pCir->StartAngle,
                       pCir->SweepAngle);
      }
      else if (!pCir->m_bClose)
      {
         /*
         ** Set ptl at outline for partial open circle
         */
         GpiSetArcParams(hps,&arcParms);
         GpiPartialArc(hps,&ptl,0x0000ff00,
                       pCir->StartAngle,MAKEFIXED(360,0));
      }
      /*
      ** Make the linetype visible?
      */
      GpiSetLineType(hps,pCir->bt.line.LineType);
      GpiSetPattern(hps, pCir->bt.lPattern);
      GpiSetColor(hps,pCir->bt.fColor);

      /*
      ** If the outlinecolor is the same as the filling
      ** color let OS/2 draw the boundary
      ** See for detailed comments in dlg_sqr.c...
      */
      bAreaFill = bHasAreaFill(pCir);
      bClipFill = bHasClipFill(pCir);

      if ( (pCir->bt.line.LineColor == pCir->bt.fColor) &&
          !(pCir->bt.line.LineWidth > 1))
         flOptions = BA_BOUNDARY;
      else
         flOptions = BA_NOBOUNDARY;

      if (bAreaFill)
         GpiBeginArea(hps,flOptions | BA_ALTERNATE);
      else if (bClipFill)
      {
         GpiBeginPath( hps, 1L);  /* define a clip path    */
         /*
         ** If we are drawing on the screen we don't need to recalc the
         ** area for clipping (speed speed speed).
         */
         if (pwi->ulUnits == PU_PELS)
         {
            HPS hpsTemp = pwi->hps;
            SIZEL  sizlx;
            sizlx.cx = sizlx.cy = 2000L;
            /*
            ** export as bitmap!
            */          
            pwi->hps = GpiCreatePS( hab , (HDC)0, &sizlx, PU_PELS | GPIA_NOASSOC | GPIT_NORMAL );
            cirCalcOutLine(pCir,&rcl,pwi);
            GpiDestroyPS(pwi->hps);
            pwi->hps = hpsTemp;
            
         }
         else
         {
            if (prcl)
               CircleOutLine(pObj,&rcl,pwi,FALSE); /*screen  */
            else
               CirGetClippingRect(pObj,pwi,&rcl);  /*printer and preview screens*/
         }
      }

      /* Starting point used for arrows */
      GpiQueryCurrentPosition(hps,&ptlStart); 

      if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
      {
         ptl = ptlCenter;
         GpiSetCurrentPosition(hps,&ptl);
      }

      GpiSetArcParams(hps,&arcParms);

      GpiPartialArc(hps,&ptl,0x0000ff00,
                    pCir->StartAngle,pCir->SweepAngle);

      /* End point used for arrows */
      GpiQueryCurrentPosition(hps,&ptlEnd); 

      if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
         GpiLine(hps,&ptl);

      if (bAreaFill)
      {
         GpiEndArea(hps);
      }
      else if (pCir->bt.lPattern == PATSYM_GRADIENTFILL)
      {
         GpiEndPath(hps);
         GpiSetClipPath(hps,1L,SCP_AND);
         GpiSetPattern(hps,PATSYM_SOLID);
         GradientFill(pwi,hps,&rcl,&pCir->bt.gradient);
         GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
      }
      else if (pCir->bt.lPattern == PATSYM_FOUNTAINFILL)
      {
         GpiEndPath(hps);
         GpiSetClipPath(hps,1L,SCP_AND);
         GpiSetPattern(hps,PATSYM_SOLID);
         FountainFill(pwi,hps,&rcl,&pCir->bt.fountain);
         GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
      }
      /*
      ** Start working on the outline
      */
      GpiSetCurrentPosition(hps,&ptlCenter);
      GpiSetColor(hps,pCir->bt.line.LineColor);

      GpiSetLineType(hps,LINETYPE_INVISIBLE);
      if (pCir->SweepAngle == MAKEFIXED(360,0))
         GpiPartialArc(hps,&ptl,0x0000ff00,//@ch1 pCir->Multiplier,
                       pCir->StartAngle,pCir->SweepAngle);
      else if (!pCir->m_bClose)            //@ch1
         GpiPartialArc(hps,&ptl,0x0000ff00,
                       pCir->StartAngle,MAKEFIXED(360,0));


      GpiSetLineType(hps,pCir->bt.line.LineType);

      if (pCir->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         LONG lLineWidth = pCir->bt.line.LineWidth;

         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pCir->bt.line.LineJoin);
         GpiSetLineEnd(hps,pCir->bt.line.LineEnd);

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }
         GpiSetLineWidthGeom(hps,lLineWidth);
         GpiBeginPath( hps, 1L);  /* define a clip path    */
      }

      GpiPartialArc(hps,&ptl,
                    0x0000ff00,//@ch1 pCir->Multiplier,
                    pCir->StartAngle,
                    pCir->SweepAngle);

      if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
         GpiLine(hps,&ptl);
      /*
      ** Start of arrow stuff........
      */
      cirDrawArrows(pwi,pCir,ptlStart,ptlEnd);

      if (pCir->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         GpiEndPath(hps);
         GpiStrokePath (hps, 1, 0);
      }
   }     /* if drawlayer is pcir->bt.usLayer */
}
/*------------------------------------------------------------------------*/
/* Move a Circle segment.                                                 */
/*------------------------------------------------------------------------*/
void MoveCirSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy;
   float fcx,fcy;
   pCircle pCir = (pCircle)pObj;
   if (!pCir)
      return;

   fdx = (float)dx;
   fdy = (float)dy;
   fcx = pwi->usFormWidth;
   fcy = pwi->usFormHeight;

   pCir->ptlPosn.x += (float)( fdx / fcx );
   pCir->ptlPosn.y += (float)( fdy / fcy );
   pCir->bt.ptlfCenter.x += (float)( fdx / fcx );
   pCir->bt.ptlfCenter.y += (float)( fdy / fcy );
   return;
}
/*------------------------------------------------------------------------*/
VOID * CircleSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   pCircle pCir = (pCircle)pObj;
   LONG lRet;

   if (pCir->bt.usLayer == pwi->uslayer || pwi->bSelAll)
   {
      if (pwi->bOnArea)
      {
         if (WinPtInRect((HAB)0,&pObj->rclOutline,&ptl))
            return (void *)pObj;
      }
      else
      {
         lRet = DrawCirOutline(pwi,pCir,FALSE);
         if (lRet == GPI_HITS)
            return (void *)pObj;
      }
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
void CircleOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc)
{
   pCircle pCir = (pCircle)pObj;

   if (!pCir)  return;

   if (bCalc)
   {
      cirCalcOutLine(pCir,&pObj->rclOutline,pwi);

      if (pCir->bt.lPattern == PATSYM_GRADIENTFILL ||
          pCir->bt.lPattern == PATSYM_FOUNTAINFILL )
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
void CircleInvArea(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi, BOOL bInc)
{
   pCircle pCir = (pCircle)pObj;

   if (!pCir)
      return;

   CircleOutLine((POBJECT)pCir,prcl,pwi,TRUE);

   prcl->xLeft    -= pCir->bt.line.LineWidth;
   prcl->xRight   += pCir->bt.line.LineWidth;
   prcl->yBottom  -= pCir->bt.line.LineWidth;
   prcl->yTop     += pCir->bt.line.LineWidth;

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
/*----------------------------------------------------[ private ]---------*/
/*  ChangeForm                                                            */
/*                                                                        */
/*                                                                        */
/*  Point 1 is the topmost point.                                         */
/*------------------------------------------------------------------------*/
static void ChangeForm(pCircle pCir,WINDOWINFO *pwi,
                       POINTL *pptlcen,ARCPARAMS *arcParms)
{
   LONG lLineType = GpiQueryLineType(pwi->hps);

   GpiSetLineType (pwi->hps,LINETYPE_INVISIBLE);
   GpiSetArcParams(pwi->hps,arcParms);

   if (pCir->SweepAngle == MAKEFIXED(360,0))
      GpiPartialArc(pwi->hps,pptlcen,0x0000ff00,pCir->StartAngle,
                    pCir->SweepAngle);
   else
      GpiPartialArc(pwi->hps,pptlcen,0x0000ff00,pCir->StartAngle,
                    MAKEFIXED(360,0));

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose )
      GpiSetCurrentPosition(pwi->hps,pptlcen);

   GpiSetLineType(pwi->hps,LINETYPE_SOLID);
   GpiPartialArc(pwi->hps,pptlcen,0x0000ff00,pCir->StartAngle,
                 pCir->SweepAngle);

   if (pCir->SweepAngle != MAKEFIXED(360,0) && pCir->m_bClose)
      GpiLine(pwi->hps,pptlcen);

   GpiSetLineType(pwi->hps,lLineType); /* restore linetype */
}
/*------------------------------------------------------------------------*/
/*  CirDragHandle                                                         */
/*                                                                        */
/*                                                                        */
/*  Point 1 is the topmost point.                                         */
/*------------------------------------------------------------------------*/
BOOL CirDragHandle(POBJECT pObj, WINDOWINFO *pwi, POINTL ptlMouse, ULONG ulMode, MPARAM mp2)
{
   static ARCPARAMS arcParms;
   ARCFPARAMS af;       /* Floating point version (notused)*/
   static long lsel;    /* Selected handle 1 or 2          */
   pCircle pCir = (pCircle)pObj;
   static POINTL ptl,ctr;
   RECTL  rcl;

   switch(ulMode)
   {
      case WM_BUTTON1DOWN:
         lsel = 0;
         QueryArcParams(&arcParms,&af,pwi,pCir);
         ctr.x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth );
         ctr.y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
         ptl.x = ctr.x + arcParms.lP;
         ptl.y = ctr.y + arcParms.lS;
         rcl.xLeft   = ptl.x - HANDLESIZE;
         rcl.xRight  = ptl.x + HANDLESIZE;
         rcl.yBottom = ptl.y - HANDLESIZE;
         rcl.yTop    = ptl.y + HANDLESIZE;

         if ((ptlMouse.x >= rcl.xLeft && ptlMouse.x <= rcl.xRight) &&
             (ptlMouse.y >= rcl.yBottom && ptlMouse.y <= rcl.yTop))
         {
            lsel = 2;
            return TRUE;
         }
         ptl.x = ctr.x + arcParms.lR;
         ptl.y = ctr.y + arcParms.lQ;
         rcl.xLeft   = ptl.x - HANDLESIZE;
         rcl.xRight  = ptl.x + HANDLESIZE;
         rcl.yBottom = ptl.y - HANDLESIZE;
         rcl.yTop    = ptl.y + HANDLESIZE;

         if ((ptlMouse.x >= rcl.xLeft && ptlMouse.x <= rcl.xRight) &&
             (ptlMouse.y >= rcl.yBottom && ptlMouse.y <= rcl.yTop))
         {
            lsel = 1;
            return TRUE;
         }
         return FALSE;

      case WM_MOUSEMOVE:
         if (lsel)
         {
            if (lsel == 1)
            {
               arcParms.lR = ptlMouse.x - ctr.x;
               arcParms.lQ = ptlMouse.y - ctr.y;
            }
            else
            {
               arcParms.lP = ptlMouse.x - ctr.x;
               arcParms.lS = ptlMouse.y - ctr.y;
            }
            ChangeForm(pCir,pwi,&ctr,&arcParms);
         }
         return TRUE;
      case WM_BUTTON1UP:
         if (lsel)
         {
            if (lsel == 2)
            {
               pCir->arcpParms.lP = (float)(ptlMouse.x - ctr.x);
               pCir->arcpParms.lS = (float)(ptlMouse.y - ctr.y);
               pCir->arcpParms.lP /= (float)pwi->usFormWidth ;
               pCir->arcpParms.lS /= (float)pwi->usFormHeight;
            }
            else
            {
               pCir->arcpParms.lR = (float)(ptlMouse.x - ctr.x);
               pCir->arcpParms.lQ = (float)(ptlMouse.y - ctr.y);
               pCir->arcpParms.lR /= (float)pwi->usFormWidth ;
               pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
            }
         }
         return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/*  CirPutInGroup.                                                        */
/*                                                                        */
/*  Description : Here we put the CIR in the given rectl of the group.    */
/*                In fact the original parent is replaced by a new one.   */
/*                When the CIR is not part of a group than the parent is  */
/*                the paper.                                              */
/*                                                                        */
/*  Parameters  : POBJECT pObj-pointer to a CIR object which is put in the*/
/*                             group.                                     */
/*                RECTL * - pointer to the rect of the group.             */
/*                WINDOWINFO * - pointer to the original windowinfo.      */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void CirPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pCircle pCir = (pCircle)pObj;
   USHORT  usGrpWidth,usGrpHeight;

   if (!pCir) return;

   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);
   /*
   ** Setup the new rotation center.
   */
   pCir->bt.ptlfCenter.x = (float)(prcl->xLeft + (prcl->xRight - prcl->xLeft)/2);
   pCir->bt.ptlfCenter.y = (float)(prcl->yBottom + (prcl->yTop - prcl->yBottom)/2);
   pCir->bt.ptlfCenter.x /= (float)usGrpWidth;
   pCir->bt.ptlfCenter.y /= (float)usGrpHeight;


   pCir->arcpParms.lP *= (float)pwi->usFormWidth ;
   pCir->arcpParms.lQ *= (float)pwi->usFormHeight;
   pCir->arcpParms.lR *= (float)pwi->usFormWidth;
   pCir->arcpParms.lS *= (float)pwi->usFormHeight;


   pCir->ptlPosn.x  *= (float)pwi->usFormWidth;
   pCir->ptlPosn.y  *= (float)pwi->usFormHeight;


   pCir->ptlPosn.x  -= (float)prcl->xLeft;
   pCir->ptlPosn.y  -= (float)prcl->yBottom;


   pCir->ptlPosn.x  /= (float)usGrpWidth;
   pCir->ptlPosn.y  /= (float)usGrpHeight;

   pCir->arcpParms.lP /= (float)usGrpWidth;
   pCir->arcpParms.lQ /= (float)usGrpHeight;
   pCir->arcpParms.lR /= (float)usGrpWidth;
   pCir->arcpParms.lS /= (float)usGrpHeight;
}
/*------------------------------------------------------------------------*/
/*  CirRemFromGroup.                                                      */
/*                                                                        */
/*  Remove the object from the group.                                     */
/*------------------------------------------------------------------------*/
void CirRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pCircle pCir = (pCircle)pObj;
   USHORT  usGrpWidth,usGrpHeight;

   if (!pCir) return;

   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   pCir->arcpParms.lP *= (float)usGrpWidth;
   pCir->arcpParms.lQ *= (float)usGrpHeight;
   pCir->arcpParms.lR *= (float)usGrpWidth;
   pCir->arcpParms.lS *= (float)usGrpHeight;

   pCir->ptlPosn.x  *= (float)usGrpWidth;
   pCir->ptlPosn.y  *= (float)usGrpHeight;

   pCir->ptlPosn.x  += (float)prcl->xLeft;
   pCir->ptlPosn.y  += (float)prcl->yBottom;

   pCir->ptlPosn.x  /= (float)pwi->usFormWidth;
   pCir->ptlPosn.y  /= (float)pwi->usFormHeight;
   /*
   ** Restore the center point for rotation
   */
   pCir->bt.ptlfCenter.x = pCir->ptlPosn.x;
   pCir->bt.ptlfCenter.y = pCir->ptlPosn.y;

   pCir->arcpParms.lP /= (float)pwi->usFormWidth;
   pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
   pCir->arcpParms.lR /= (float)pwi->usFormWidth;
   pCir->arcpParms.lS /= (float)pwi->usFormHeight;
}
/*------------------------------------------------------------------------*/
/*  CirStretch.                                                           */
/*                                                                        */
/*  Description : Stretches the circle. Called during the following msg's.*/
/*                WM_BUTTON1DOWN,WM_BUTTON1UP and WM_MOUSEMOVE.           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void CirStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg)
{
   static USHORT usWidth,usHeight;
   USHORT usOldWidth,usOldHeight;
   static float fOffx,fOffy;
   float fOldx,fOldy;

   pCircle pCir = (pCircle)pObj;

   if (!pCir) return;

   usWidth  =(USHORT)(prcl->xRight - prcl->xLeft);
   usHeight =(USHORT)(prcl->yTop - prcl->yBottom);

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         pCir->arcpParms.lP *= (float)pwi->usFormWidth ;
         pCir->arcpParms.lQ *= (float)pwi->usFormHeight;
         pCir->arcpParms.lR *= (float)pwi->usFormWidth;
         pCir->arcpParms.lS *= (float)pwi->usFormHeight;

         pCir->ptlPosn.x *= (float)pwi->usFormWidth;
         pCir->ptlPosn.y *= (float)pwi->usFormHeight;
         pCir->ptlPosn.x -= (float)prcl->xLeft;
         pCir->ptlPosn.y -= (float)prcl->yBottom;
         pCir->ptlPosn.x /= (float)usWidth;
         pCir->ptlPosn.y /= (float)usHeight;
         pCir->arcpParms.lP /= (float)usWidth;
         pCir->arcpParms.lQ /= (float)usHeight;
         pCir->arcpParms.lR /= (float)usWidth;
         pCir->arcpParms.lS /= (float)usHeight;

         pCir->bt.ptlfCenter.x *= (float)pwi->usFormWidth;
         pCir->bt.ptlfCenter.y *= (float)pwi->usFormHeight;
         pCir->bt.ptlfCenter.x -= (float)prcl->xLeft;
         pCir->bt.ptlfCenter.y -= (float)prcl->yBottom;
         pCir->bt.ptlfCenter.x /= (float)usWidth;
         pCir->bt.ptlfCenter.y /= (float)usHeight;

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
         CirMoveOutLine((POBJECT)pCir,pwi,0,0);
         pwi->usFormWidth  = usOldWidth;
         pwi->usFormHeight = usOldHeight;
         pwi->fOffx = fOldx;
         pwi->fOffy = fOldy;
         return;

      case WM_BUTTON1UP:
         pCir->arcpParms.lP *= (float)usWidth ;
         pCir->arcpParms.lQ *= (float)usHeight;
         pCir->arcpParms.lR *= (float)usWidth ;
         pCir->arcpParms.lS *= (float)usHeight;

         pCir->ptlPosn.x *= (float)usWidth;
         pCir->ptlPosn.y *= (float)usHeight;

         pCir->ptlPosn.x += (float)prcl->xLeft;
         pCir->ptlPosn.y += (float)prcl->yBottom;

         pCir->arcpParms.lP /= (float)pwi->usFormWidth ;
         pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
         pCir->arcpParms.lR /= (float)pwi->usFormWidth ;
         pCir->arcpParms.lS /= (float)pwi->usFormHeight;

         pCir->ptlPosn.x /= (float)pwi->usFormWidth;
         pCir->ptlPosn.y /= (float)pwi->usFormHeight;

         pCir->bt.ptlfCenter.x *= (float)usWidth;
         pCir->bt.ptlfCenter.y *= (float)usHeight;
         pCir->bt.ptlfCenter.x += (float)prcl->xLeft;
         pCir->bt.ptlfCenter.y += (float)prcl->yBottom;
         pCir->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
         pCir->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
         return;
   }
   return;
}
/*-----------------------------------------------[ public ]---------------*/
void CirDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg, POINTL *pt)
{
   POINTL ptl[4],ptlCenter;
   ARCPARAMS  arcParms;

   pCircle pCir = (pCircle)pObj;

   if (!pObj) return;

   switch (ulMsg)
   {
      case WM_BUTTON1UP:
         ptl[0].x = (LONG)(pCir->arcpParms.lP * pwi->usFormWidth );
         ptl[0].y = (LONG)(pCir->arcpParms.lS * pwi->usFormHeight);
         ptl[1].x = (LONG)(pCir->arcpParms.lR * pwi->usFormWidth );
         ptl[1].y = (LONG)(pCir->arcpParms.lQ * pwi->usFormHeight);
         ptl[2].x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
         ptl[2].y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
         ptl[2].x += (LONG)(pwi->fOffx);
         ptl[2].y += (LONG)(pwi->fOffy);

         ptl[0].x += ptl[2].x;
         ptl[0].y += ptl[2].y;
         ptl[1].x += ptl[2].x;
         ptl[1].y += ptl[2].y;

         if (!pt)
         {
            ptlCenter.x = (LONG)(pCir->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pCir->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
         {
            ptlCenter = *pt;
         }
         RotateSqrSegment(lRotate,ptlCenter,ptl,3);

         ptl[0].x -= ptl[2].x;
         ptl[0].y -= ptl[2].y;
         ptl[1].x -= ptl[2].x;
         ptl[1].y -= ptl[2].y;

         pCir->arcpParms.lP = (float)ptl[0].x;
         pCir->arcpParms.lS = (float)ptl[0].y;
         pCir->arcpParms.lR = (float)ptl[1].x;
         pCir->arcpParms.lQ = (float)ptl[1].y;

         pCir->arcpParms.lP /= (float)pwi->usFormWidth;
         pCir->arcpParms.lS /= (float)pwi->usFormHeight;
         pCir->arcpParms.lR /= (float)pwi->usFormWidth;
         pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
         ptl[2].x -= (LONG)(pwi->fOffx);
         ptl[2].y -= (LONG)(pwi->fOffy);

         pCir->ptlPosn.x = (float)ptl[2].x;
         pCir->ptlPosn.y = (float)ptl[2].y;

         pCir->ptlPosn.x /= (float)pwi->usFormWidth;
         pCir->ptlPosn.y /= (float)pwi->usFormHeight;
         break;
      case WM_MOUSEMOVE:
         ptl[0].x = (LONG)(pCir->arcpParms.lP * pwi->usFormWidth );
         ptl[0].y = (LONG)(pCir->arcpParms.lS * pwi->usFormHeight);
         ptl[1].x = (LONG)(pCir->arcpParms.lR * pwi->usFormWidth );
         ptl[1].y = (LONG)(pCir->arcpParms.lQ * pwi->usFormHeight);
         ptl[2].x = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth);
         ptl[2].y = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
         ptl[2].x += (LONG)(pwi->fOffx);
         ptl[2].y += (LONG)(pwi->fOffy);

         if (!pt)
         {
            ptlCenter.x = (LONG)(pCir->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pCir->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
         {
            ptlCenter = *pt;
         }
         ptl[0].x += ptl[2].x;
         ptl[0].y += ptl[2].y;
         ptl[1].x += ptl[2].x;
         ptl[1].y += ptl[2].y;

         RotateSqrSegment(lRotate,ptlCenter,ptl,3);

         ptl[0].x -= ptl[2].x;
         ptl[0].y -= ptl[2].y;
         ptl[1].x -= ptl[2].x;
         ptl[1].y -= ptl[2].y;

         arcParms.lP = (float)ptl[0].x;
         arcParms.lS = (float)ptl[0].y;
         arcParms.lR = (float)ptl[1].x;
         arcParms.lQ = (float)ptl[1].y;

         GpiSetCurrentPosition(pwi->hps,&ptl[2]);
         GpiSetLineType(pwi->hps,LINETYPE_INVISIBLE);

         if (pCir->SweepAngle == MAKEFIXED(360,0))
         {
            pCir->StartAngle = 0;
            GpiSetArcParams(pwi->hps,&arcParms);
            GpiPartialArc(pwi->hps,&ptl[2],
                          MAKEFIXED(1,0),
                          pCir->StartAngle,
                          pCir->SweepAngle);
         }
         GpiSetArcParams(pwi->hps,&arcParms);
         GpiSetLineType(pwi->hps,LINETYPE_SOLID);
         GpiPartialArc(pwi->hps,&ptl[2],MAKEFIXED(1,0),
                       pCir->StartAngle,pCir->SweepAngle);

         if (pCir->SweepAngle != MAKEFIXED(360,0))
            GpiLine(pwi->hps,&ptl[2]);
         break;
   }
}
/*-----------------------------------------------[ public ]---------------*/
/* circleCommand.                                                         */
/*                                                                        */
/* Description  : During a WM_COMMAND the type of circle is set when the  */
/*                command id is on of the circle buttons. We use the ID   */
/*                to set the usType and with this var we remember what    */
/*                we are supposed to draw.                                */
/*                                                                        */
/* Returns     :  MRESULT.                                                */
/*------------------------------------------------------------------------*/
MRESULT circleCommand(USHORT usCommand)
{
   if ( usCommand >= IDBTN_CIRCLE && usCommand <= IDBTN_CRFBOT )
   {
      usType = usCommand;
      return (MRESULT)1;
   }
   return (MRESULT)0;
}
/*-----------------------------------------------[ public ]---------------*/
VOID createCircle(WINDOWINFO *pwi,POINTL ptMouse,ULONG ulMsg, USHORT sShift)
{
   ARCPARAMS arcParms;
   static    POINTL    ptCenter;
   static    POINTL    ptStart;
   static    FIXED     fStartAngle;
   static    FIXED     fSweepAngle;
   static    BOOL      bClose;
   static    ULONG     usCir;
   pCircle   pCir;

   if (!usType || !pwi->pvCurrent)
      return;

   if (ulMsg == WM_BUTTON1DOWN)
   {
      bClose   =  FALSE;

      ptCenter = ptMouse;
      ptStart  = ptMouse;
      usCir    = usType;

      switch(usType)
      {
      case IDBTN_CIRCLE:
         fStartAngle = MAKEFIXED(0,0);
         fSweepAngle = MAKEFIXED(360,0);
         break;
      case IDBTN_CLFBOT:
         bClose = TRUE;
      case IDBTN_CLBOT:
         fStartAngle = MAKEFIXED(180,0);
         fSweepAngle = MAKEFIXED(90,0);
         break;
      case IDBTN_CLFTOP:
         bClose = TRUE;
      case IDBTN_CLTOP:
         fStartAngle = MAKEFIXED(90,0); //MAKEFIXED(180,0);
         fSweepAngle = MAKEFIXED(90,0);
         break;
      case IDBTN_CRFBOT:
         bClose = TRUE;
      case IDBTN_CRBOT:
         fStartAngle = MAKEFIXED(270,0);
         fSweepAngle = MAKEFIXED(90,0);
         break;
      case IDBTN_CRFTOP:
         bClose = TRUE;
      case IDBTN_CRTOP:
         fStartAngle = MAKEFIXED(0,0);
         fSweepAngle = MAKEFIXED(90,0);
         break;
      default:
         fStartAngle = MAKEFIXED(0,0);
         fSweepAngle = MAKEFIXED(360,0);
         break;
      }
      return;

   }

   GpiSetMix(pwi->hps,FM_INVERT);
   arcParms.lR = 0L;
   arcParms.lS = 0L;

   switch(usCir)
      {
      case IDBTN_CLTOP:
      case IDBTN_CLFTOP:
         ptCenter = ptMouse;
         arcParms.lP = ptMouse.x - ptStart.x;
         if (sShift == KC_SHIFT)
            arcParms.lQ = ptMouse.x - ptStart.x;
         else
            arcParms.lQ = ptStart.y - ptMouse.y;
         break;
      case IDBTN_CLBOT:
      case IDBTN_CLFBOT:
         ptCenter.x = ptMouse.x;
         arcParms.lP = ptMouse.x - ptStart.x;
         if (sShift == KC_SHIFT)
            arcParms.lQ = ptMouse.x - ptStart.x;
         else
            arcParms.lQ = ptStart.y - ptMouse.y;
         break;
      case IDBTN_CRFBOT:
      case IDBTN_CRBOT:
         arcParms.lP = ptMouse.x - ptStart.x;
         if (sShift == KC_SHIFT)
            arcParms.lQ = ptMouse.x - ptStart.x;
         else
            arcParms.lQ = ptStart.y - ptMouse.y;
         break;
      case IDBTN_CRFTOP:
      case IDBTN_CRTOP:
         ptCenter.y = ptMouse.y;
         arcParms.lP = ptMouse.x - ptStart.x;
         if (sShift == KC_SHIFT)
            arcParms.lQ = ptMouse.x - ptStart.x;
         else
            arcParms.lQ = ptStart.y - ptMouse.y;
         break;
      case IDBTN_CIRCLE:
         arcParms.lP = ptMouse.x - ptStart.x;
         if (sShift == KC_SHIFT)
            arcParms.lQ = ptMouse.x - ptStart.x;
         else
            arcParms.lQ = abs(ptMouse.y - ptStart.y);
         break;
      }

   GpiSetArcParams(pwi->hps,&arcParms);
   GpiSetCurrentPosition(pwi->hps,&ptCenter);

   if (!bClose)
   {
      GpiSetLineType(pwi->hps,LINETYPE_INVISIBLE);
      GpiPartialArc(pwi->hps,&ptCenter,0x0000ff00,fStartAngle,MAKEFIXED(360,0));
   }
   GpiSetLineType(pwi->hps,LINETYPE_SOLID);
   GpiPartialArc(pwi->hps,&ptCenter,0x0000ff00,fStartAngle,fSweepAngle);

   if (bClose)
       GpiLine(pwi->hps,&ptCenter);

   if (ulMsg == WM_BUTTON1UP && pwi->pvCurrent)
   {
      pCir = (pCircle)pwi->pvCurrent;
      pCir->arcpParms.lP = (float)arcParms.lP;
      pCir->arcpParms.lQ = (float)arcParms.lQ;
      pCir->arcpParms.lR = (float)arcParms.lR;
      pCir->arcpParms.lS = (float)arcParms.lS;
      pCir->arcpParms.lP /= (float)pwi->usFormWidth;
      pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
      pCir->arcpParms.lR /= (float)pwi->usFormWidth;
      pCir->arcpParms.lS /= (float)pwi->usFormHeight;
      pCir->ptlPosn.x   = (float)ptCenter.x;
      pCir->ptlPosn.y   = (float)ptCenter.y;
      pCir->ptlPosn.x  /= (float)pwi->usFormWidth;
      pCir->ptlPosn.y  /= (float)pwi->usFormHeight;
      pCir->bt.ptlfCenter.x = pCir->ptlPosn.x;
      pCir->bt.ptlfCenter.y = pCir->ptlPosn.y;
      pCir->m_bClose     = bClose;
      pCir->SweepAngle   = fSweepAngle;
      pCir->StartAngle   = fStartAngle;
   }
}
/*
** Handle Checkbox for making an oval cicular...
*/
void CircleCheckBox(HWND hwnd,WINDOWINFO *pwi, pCircle pCir)
{
   ARCPARAMS arcParms;
   long      lDiff;
   SWP       swp;
   HWND      hCheck;

   arcParms.lP = (LONG)( pCir->arcpParms.lP * pwi->usFormWidth );
   arcParms.lQ = (LONG)( pCir->arcpParms.lQ * pwi->usFormHeight);

   lDiff = arcParms.lP - arcParms.lQ;
   lDiff = abs(lDiff);

   if (lDiff <= 4)
   {
      WinShowWindow(WinWindowFromID(hwnd,ID_CHKCIR),FALSE);
      hCheck = WinWindowFromID(hwnd,ID_CHKOPENCIR);
      if (!hCheck)
         return;

      WinQueryWindowPos(hCheck,&swp);
      swp.y -= swp.cy;
      WinSetWindowPos(hCheck,HWND_TOP,swp.x,swp.y,0,0,SWP_MOVE);
   }
   else
   {
      WinSendDlgItemMsg(hwnd,ID_CHKCIR,BM_SETCHECK,(MPARAM)0,(MPARAM)0);
      WinSetWindowText(hwnd,(PSZ)"Circular object (- Distorted -)");
   }
   return;
}
/*---------------------------------------------------------------------------*/
/* name:        CircleDlgProc.                                               */
/*                                                                           */
/* description  : Dialog procedure which handles the dailog for editing the  */
/*                selected circle.                                           */
/*                Started after the user has double clicked a circle.        */
/*                                                                           */
/* Parameters   : HWND hwnd - Window handle of the dialog.                   */
/*                ULONG ulMsg Window message.                                */
/*                MPARAM mp1 - Message parameter 1                           */
/*                MPARAM mp2 - Message parameter 2                           */
/*                                                                           */
/* Returns      : MRESULT - Message result.                                  */
/*---------------------------------------------------------------------------*/
MRESULT EXPENTRY CircleDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SWP   swp;
   static HWND hCircle;
   static pCircle pCir;
   static WINDOWINFO  *pwi;
   static hDrawing;       /* drawing window */
   static BOOL bInit;


   switch(msg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         pwi   = (WINDOWINFO *)mp2;
         pCir  = (pCircle)pwi->pvCurrent;
         ((POBJECT)pwi->pvCurrent)->bLocked = TRUE;
         hDrawing = pwi->hwndClient;

         defCir.ulLayer      = (ULONG)pCir->bt.usLayer;
         defCir.ulSweepAngle = (ULONG)(pCir->SweepAngle >> 16);
         defCir.ulStartAngle = (ULONG)(pCir->StartAngle >> 16);
         defCir.bClose       = pCir->m_bClose;
         defCir.bCirc	     = FALSE;
         defCir.bDialogActive= TRUE; /* Make active */
         if (defCir.bClose)
            WinSendDlgItemMsg(hwnd,ID_CHKOPENCIR,BM_SETCHECK,(MPARAM)1,(MPARAM)0);

         CircleCheckBox(hwnd,pwi,pCir);
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);
         hCircle = WinWindowFromID(hwnd, ID_CIRWIN);
         /* Set the layer spinbutton */
         WinSendDlgItemMsg( hwnd, ID_SPNCIRLAYER, SPBM_SETLIMITS,
                            MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));
         WinSendDlgItemMsg( hwnd, ID_SPNCIRLAYER, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)pCir->bt.usLayer), NULL);
         /* SweepAngle*/
         WinSendDlgItemMsg( hwnd, ID_SPNSWEEP, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_SWEEPANGLE), MPFROMLONG(MIN_SWEEPANGLE));
         WinSendDlgItemMsg( hwnd, ID_SPNSWEEP, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)defCir.ulSweepAngle), NULL);
         /* StartAngle*/
         WinSendDlgItemMsg( hwnd, ID_SPNSTART, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_STARTANGLE), MPFROMLONG(MIN_STARTANGLE));
         WinSendDlgItemMsg( hwnd, ID_SPNSTART, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)defCir.ulStartAngle), NULL);
         bInit = FALSE;
         return (MRESULT)0;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               pCir->SweepAngle = MAKEFIXED(defCir.ulSweepAngle,0);
               pCir->StartAngle = MAKEFIXED(defCir.ulStartAngle,0);
               pCir->bt.usLayer    = (SHORT)defCir.ulLayer;
               pCir->m_bClose   = defCir.bClose;
               defCir.bDialogActive = FALSE;
               if (defCir.bCirc)
               {
                  pCir->arcpParms.lQ =  pCir->arcpParms.lP * pwi->usFormWidth;
                  pCir->arcpParms.lQ /= (float)pwi->usFormHeight;
               }
               ((POBJECT)pCir)->bLocked = FALSE;
               pwi->bFileHasChanged = TRUE;
               WinPostMsg(hDrawing,UM_ENDDIALOG,(MPARAM)pCir,(MPARAM)0);
               WinDismissDlg( hwnd, DID_OK);
               break;

            case DID_CANCEL:
               ((POBJECT)pCir)->bLocked = FALSE;
               defCir.bDialogActive = FALSE;
               WinDismissDlg( hwnd, DID_CANCEL);
               break;
            case DID_HELP:
               ShowDlgHelp(hwnd);
               return 0;
         }
         return (MRESULT)0;
       case WM_CONTROL:
          switch (LOUSHORT(mp1))
          {
             /* SweepAngle */
             case ID_SPNSWEEP:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNSWEEP,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&defCir.ulSweepAngle),MPFROM2SHORT(0,0));
                  if (defCir.ulSweepAngle <= MIN_SWEEPANGLE)
                     defCir.ulSweepAngle = MIN_SWEEPANGLE;
                  WinInvalidateRect (hCircle,NULL,FALSE);
               }
               break;
             /* StartAngle */
             case ID_SPNSTART:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNSTART,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&defCir.ulStartAngle),MPFROM2SHORT(0,0));
                  WinInvalidateRect (hCircle,NULL,FALSE);
                  defCir.ulStartAngle = min(defCir.ulStartAngle,MAX_STARTANGLE);
               }
               break;
             /* Layer */
             case ID_SPNCIRLAYER:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNCIRLAYER,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&defCir.ulLayer),MPFROM2SHORT(0,0));
                  WinInvalidateRect (hCircle,NULL,FALSE);
               }
               break;
            case ID_CHKOPENCIR:
               defCir.bClose = !defCir.bClose;
               WinInvalidateRect (hCircle,NULL,FALSE);
               break;
            case ID_CHKCIR:
               defCir.bCirc = !defCir.bCirc;
               if (defCir.bCirc)
                  WinSetWindowText(hwnd,(PSZ)"Circular object");
               else
                  WinSetWindowText(hwnd,(PSZ)"Circular object (- Distorted -)");
               break;
          }
          return (MRESULT)0;
   }
   return WinDefDlgProc(hwnd,msg,mp1,mp2);
}
/*-----------------------------------------------[ private ]-----------------*/
/* name:        drawDefCircle.                                               */
/*                                                                           */
/* description  : Draws the circle in the dialog window.                     */
/*                called during the WM_PAINT of the window.                  */
/*---------------------------------------------------------------------------*/
static void drawDefCircle( HPS hps, ARCPARAMS arcParms, POINTL ptCenter)
{
   GpiSetArcParams(hps,&arcParms);
   GpiSetCurrentPosition(hps,&ptCenter);

   if (!defCir.bClose)
   {
      GpiSetLineType(hps,LINETYPE_INVISIBLE);
      GpiPartialArc(hps,&ptCenter,0x0000ff00,MAKEFIXED(defCir.ulStartAngle,0),MAKEFIXED(360,0));
   }
   GpiSetLineType(hps,LINETYPE_SOLID);
   GpiPartialArc(hps,&ptCenter,0x0000ff00,MAKEFIXED(defCir.ulStartAngle,0),MAKEFIXED(defCir.ulSweepAngle,0));

   if (defCir.bClose)
       GpiLine(hps,&ptCenter);
}
/*---------------------------------------------------------------------------*/
/* name         : CircleWndProc                                              */
/*                                                                           */
/* description  : Window procedure for the window in the circle dialog.      */
/*                Handles the messages for the "circleWnd" class.            */
/*                                                                           */
/* Parameters   : HWND hwnd - Window handle.                                 */
/*                                                                           */
/* returns      : MRESULT - Message result.                                  */
/*---------------------------------------------------------------------------*/
MRESULT EXPENTRY CircleWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   HPS hps;
   RECTL  rcl;
   static POINTL ptlCenter;
   static SWP    swp;
   static ARCPARAMS arcParms;

   switch(msg)
   {
      case WM_CREATE:
         WinQueryWindowPos(hwnd,&swp);
         ptlCenter.x = swp.cx /2;
         ptlCenter.y = swp.cy /2;
         arcParms.lP = swp.cx /3;
         arcParms.lQ = swp.cx /3;
         arcParms.lR = 0L;
         arcParms.lS = 0L;
         return (MRESULT)0;
      case WM_PAINT:
         WinQueryWindowRect(hwnd,&rcl);
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiErase(hps);
         WinDrawBorder(hps, &rcl,2L,2L,0L, 0L, DB_DEPRESSED);
         drawDefCircle(hps,arcParms,ptlCenter);
         WinEndPaint(hps);
         return 0;
   }
   return (WinDefWindowProc(hwnd,msg,mp1,mp2));
}
/*-----------------------------------------------[ public ]------------------*/
/* name         : registerCircle.                                            */
/*---------------------------------------------------------------------------*/
void registerCircle(HAB hab)
{
    static CHAR szCircleClass[]    = "circleWnd"; // See drawit.rc

    WinRegisterClass( hab,                     // Another block handle
                     (PSZ)szCircleClass,       // Name of class being registered
                     (PFNWP)CircleWndProc,     // Window procedure for class
                     CS_SIZEREDRAW,            // Class style
                     (BOOL)0);                 // Extra bytes to reserve
}
/*-----------------------------------------------[ public ]------------------*/
/* name         : circleDetail.                                              */
/*---------------------------------------------------------------------------*/
void circleDetail( HWND hOwner, WINDOWINFO *pwi)
{
   if (!defCir.bDialogActive)
      WinLoadDlg(HWND_DESKTOP,hOwner,CircleDlgProc,0L,DLG_CIROBJECT,(void *)pwi);
}
