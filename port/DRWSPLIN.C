/*------------------------------------------------------------------------*/
/*  Name: drwsplin.c                                                      */
/*                                                                        */
/*  Description : Polylines and polyspline functions.                     */
/*                                                                        */
/* Functions:                                                             */
/*    CreateSpline       : Creates the initial polyline/polyspline.       */
/*    FilePutSpline      : Puts the data of the spline into a file..      */
/*    FileGetSpline      : Gets the data of the spline from a file..      */
/*    SplineOutLine      : Gives the total outline (square) of the spline */
/*    SplineDelLastLine  : The last drawn line is deleted when in drawing */
/*                         mode and the user presses the '-' key.         */
/*    SplineSelect       : Returns and Spl if we clicked in it's area.    */
/*    SplineCopy         : Copies a splineobject (KC_SHIFT???)            */
/*    SplinDelHandle     : Deletes the selected handle when in formchange */
/*    SplMoveSegment     : Moves the spline over a distance dx,dy.        */
/*    SplinMoveOutLine   : Moves the outline when we are dragged by mouse */
/*    SplinShowHandles   : Draw on all line joins a handle.               */
/*    SplinHitHandle     : Tells you if the mouse hovers over an handle.  */
/*    DrawSplineSegment  : Draws a spline segment on a WM_PAINT etc.      */
/*    RegularPolyCreate  : Creates regular polypoint of class CLS_SPLINE  */
/*    SplPutInGroup      : Put object in a group.                         */
/*    SplRemFromGroup    : Remove the object from the group.              */
/*    SplStretch         : Stretch the spline during WM_MOUSE messages.   */
/*    splDrawOutline     : Draw only the outline of the spline.           */
/*                                                                        */
/* Private functions:                                                     */
/*    SplGetClippingRect : Printing only and private to this mod.         */
/*    insVertex          : Inserts a vertex at index i with a logical ptl */
/*    ptOnLine           : Checks if the mouse is on one of the poly lines*/
/*    PolyDraw           : Draws outline during move and rotation.        */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwarr.h"        /* Arrows..... */	
#include "dlg_hlp.h"
#include "dlg_sqr.h"
#include "drwsplin.h"
#include "drwutl.h"
#include "dlg_clr.h"
#include "dlg_lin.h"
#include "resource.h"
#include "drwfount.h"
#include "drwfigur.h"

#define LINESELSIZE     10
#define HANDLESIZE      10
#define MAXPOINTS       350
#define MINPTS          3    /* minimum nr of point in regular polypoint */
#define MAXPTS          48   /* maximum nr of point in regular polypoint */
#define POLYSPLINE    0x01
#define POLYLINE      0x02
#define PI 3.1415926

static POINTL splineptl[MAXPOINTS];
static SHORT  nr;
static SHORT  sPolyStar;  /* nr of polystar points */
static ULONG  ulState; /* set in createspline and finally used in closespline*/
static ULONG  ulType;
static BOOL   bDialog = FALSE;

/*-------------------------------------------------[ public  ]------------*/
/* PolyDump                                                               */
/*                    tempcode !!!!!                                      */
/*------------------------------------------------------------------------*/
void PolyDump(POBJECT pObj, WINDOWINFO *pwi)
{
   static Loadinfo  li;
   ULONG  ulPoints;
   ULONG  ulBytes;
   int    i,l;
   unsigned long *puc;
   pSpline pSpl = (pSpline)pObj;
   char   szBuf[25];
   POINTL *pPoint;
   LONG   ulxmin,ulymin;
   PPOINTLF pptl;
   if (!pObj)
      return;
   if (pObj->usClass != CLS_SPLINE)
      return;

   li.dlgflags = FDS_SAVEAS_DIALOG;
   strcpy(li.szExtension,".SPL");        /* Set default extension */

   if (!FileGetName(&li,pwi))
      return;

   if ( (li.handle = open(li.szFileName, 
                          O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 
                          S_IREAD|S_IWRITE)) == -1 )
      return;

   ulPoints = (ULONG)pSpl->nrpts;
   puc      = (unsigned char *)pSpl->pptl;
   ulBytes  =  sizeof(POINTLF)*pSpl->nrpts;
   
   pPoint   = (POINTL *)calloc(ulBytes,1);

   pptl = pSpl->pptl;
   ulxmin = 200000;
   ulymin = 200000;
   for ( i = 0; i < pSpl->nrpts; i++)
   {
      pPoint[i].x = (LONG)(pptl->x * pwi->usFormWidth );
      pPoint[i].y = (LONG)(pptl->y * pwi->usFormHeight);

      if (pPoint[i].x < ulxmin)
         ulxmin = pPoint[i].x;
      if (pPoint[i].y < ulymin)
         ulymin = pPoint[i].y;

      pptl++;
   }

   for ( i = 0; i < pSpl->nrpts; i++)
   {
      pPoint[i].x -= ulxmin;
      pPoint[i].y -= ulymin;
   }
   puc = pPoint;

//   _write(li.handle,(PVOID)&ulPoints,sizeof(ULONG));
   l = 1;
   for (i=0; i < pSpl->nrpts*2; i++)
   {
      sprintf(szBuf,"0x%08lx,",*puc);
     _write(li.handle,(PVOID)szBuf,strlen(szBuf));
      puc++;
      if (!(l % 5))
         _write(li.handle,(PVOID)"\n",strlen("\n"));
      
      l++;
   }
   close(li.handle);
   free(pPoint);
   li.handle = 0;
   return;
}
/*------------------------------------------------------------------------*/
/* FilePutSpline.                                                         */
/*                                                                        */
/*  Description : Writes the variable part of the splinestruct to file.   */
/*                Could not be done in the general file function in       */
/*                drwutl.c since the the size of the struct is variable.  */
/*                Dependend on the number of points.                      */
/*                                                                        */
/*                                                                        */
/*  Parameters : pLoadinfo - pointer to  filestructure defined in dlg.h   */
/*               pSpline * - pointer to polylinestructure.                */
/*                                                                        */
/*  Return     : int: Number of bytes written to file or -1 on error.     */
/*------------------------------------------------------------------------*/
int FilePutSpline(pLoadinfo pli, pSpline pSpl )
{
   ULONG ulBytes;
   PVOID p;
   int   i;

   ulBytes = pSpl->nrpts * sizeof(POINTL);

   p = (PVOID)pSpl->pptl;

   i = write(pli->handle,(PVOID)p,ulBytes);

   return i;
}
/*------------------------------------------------------------------------*/
/* FileGetSpline.                                                         */
/*------------------------------------------------------------------------*/
int FileGetSpline(pLoadinfo pli, pSpline pSpl)
{
   ULONG ulBytes;
   int   i;
   ulBytes = pSpl->nrpts * sizeof(POINTLF);

   pSpl->pptl = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

   i = read( pli->handle,(PVOID)pSpl->pptl,ulBytes);

   return i;
}
/*-----------------------------------------------[ private ]--------------*/
/* Get clipping rect for printing. The GpiBoundary functions do not       */
/* function correctly. So here is the workareound.                        */
/*------------------------------------------------------------------------*/
static void SplGetClippingRect(POBJECT pObj,WINDOWINFO *pwi,RECTL *rcl)
{
   rcl->xLeft    =  (LONG)(pObj->rclf.xLeft   * pwi->usFormWidth );
   rcl->xRight   =  (LONG)(pObj->rclf.xRight  * pwi->usFormWidth );
   rcl->yBottom  =  (LONG)(pObj->rclf.yBottom * pwi->usFormHeight);
   rcl->yTop     =  (LONG)(pObj->rclf.yTop    * pwi->usFormHeight);
   return;
}
/*-----------------------------------------------[ private ]--------------*/
/* insVertex.                                                             */
/*                                                                        */
/* Description  : Inserts a point in an existing polygons.                */
/*                                                                        */
/* Parameters   : pSpline pSpl - pointer to a spline.                     */
/*                POINTLF ptlf - nomalized point.                         */
/*                int iVertex  - Vertex or point index.                   */
/*                                                                        */
/* Returns      : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL insVertex(pSpline pSpl, POINTLF *ptlf, int iVertex)
{
   ULONG ulBytes;
   int i;
   POINTLF *pptlf,*pptl,*p;
   /*
   ** Some checks before going further.
   ** We can append but 2 is too far away...
   */
   if (iVertex > ( pSpl->nrpts + 2))
      return FALSE;

   pptl = pSpl->pptl;

   ulBytes = (pSpl->nrpts + 1) * sizeof(POINTLF);

   p = pptlf = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

   if (!p)
      return FALSE;

   for (i = 0; i < pSpl->nrpts + 1; i++)
   {
      if (i == iVertex )
      {
         pptlf->x = ptlf->x;
         pptlf->y = ptlf->y;
         pptlf++;
      }
      else
      {
         pptlf->x =  pptl->x;
         pptlf->y =  pptl->y;
         pptlf++;
         pptl++;
      }
   }
   free (pSpl->pptl);
   pSpl->pptl = p;
   pSpl->nrpts++;
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/* delVertex.                                                             */
/*                                                                        */
/* Description  : Deletes a point in an existing polygons.                */
/*                                                                        */
/* Parameters   : pSpline pSpl - pointer to a spline.                     */
/*                POINTLF ptlf - nomalized point.                         */
/*                int iVertex  - Vertex or point index.                   */
/*                                                                        */
/* Returns      : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL delVertex(pSpline pSpl,int iVertex)
{
   ULONG ulBytes;
   int i;
   POINTLF *pptlf,*pptl,*p;
   /*
   ** Some checks before going further.
   */
   if (iVertex <= 0 || pSpl->nrpts <= 2)
      return FALSE;

   pptl = pSpl->pptl;

   ulBytes = (pSpl->nrpts - 1) * sizeof(POINTLF);

   p = pptlf = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

   if (!p)
      return FALSE;

   for (i = 0; i < pSpl->nrpts;)
   {
      if (i == iVertex )
      {
         pptl++; /* Jump over this one */
      }
      else
      {
         pptlf->x =  pptl->x;
         pptlf->y =  pptl->y;
         pptlf++;
         pptl++;
      }
      i++;
   }
   free (pSpl->pptl);
   pSpl->pptl = p;
   pSpl->nrpts--;
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/* ptOnLine.                                                              */
/*                                                                        */
/* Description  : Check if the mouse position is on one of the polygon    */
/*                lines.                                                  */
/*                                                                        */
/* Parameters   : pSpline pSpl - pointer to a spline.                     */
/*                POINTL  *ptl - mouse position.                          */
/*                WINDOWINFO * - Pointer to the WINDOWINFO struct.        */
/*                                                                        */
/* Returns      : The index of the vertex or -1 on failure.               */
/*------------------------------------------------------------------------*/
static int ptOnLine(WINDOWINFO *pwi, POINTL *ptl, pSpline pSpl)
{
   int iIndex;
   int i;
   POINTLF *pptl;
   BOOL bHit;

   pptl = pSpl->pptl;

   GpiSetDrawControl(pwi->hps, DCTL_CORRELATE, DCTL_ON);
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
   GpiSetPickAperturePosition(pwi->hps, ptl);
   /*
   ** Use our points buffer defined at the top of
   ** this file to put in the calculated points...
   */
   for ( i = 0; i < pSpl->nrpts; i++)
   {
      splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
      splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
      splineptl[i].x +=(LONG)(pwi->fOffx);
      splineptl[i].y +=(LONG)(pwi->fOffy);
      pptl++;
   }

   if (pSpl->ulState & SPL_CLOSED )
   {
      pptl = pSpl->pptl;
      splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
      splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
      splineptl[i].x +=(LONG)(pwi->fOffx);
      splineptl[i].y +=(LONG)(pwi->fOffy);
      i++;
   }
   GpiMove(pwi->hps,&splineptl[0]);
   bHit = FALSE;
   iIndex = -1;
   for (i = 1; (i < pSpl->nrpts && !bHit); i++)
   {
      if ( GpiLine(pwi->hps,&splineptl[i]) == GPI_HITS)
      {
         bHit = TRUE;
         iIndex = i;
      }
   }

   if (!bHit && (pSpl->ulState & SPL_CLOSED ))
   {
      if ( GpiLine(pwi->hps,&splineptl[i]) == GPI_HITS)
      {
         bHit = TRUE;
         iIndex = i;
      }
   }
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
   GpiSetDrawControl(pwi->hps, DCTL_CORRELATE, DCTL_OFF);
   return iIndex;
}
/*------------------------------------------------------------------------*/
static pSpline OpenSpline(WINDOWINFO *pwi)
{
   pSpline pSpl;
   POBJECT pObj;

   pSpl =(pSpline)pObjCreate(pwi, CLS_SPLINE);

   pSpl->ustype     = CLS_SPLINE;
   pSpl->LinDeleted = FALSE;
   pSpl->LinMultiSelected = FALSE;

   pObj = (POBJECT)pSpl;
   pObj->moveOutline = SplinMoveOutLine;
   pObj->paint       = DrawSplineSegment;
   pObj->getInvalidationArea    = SplineInvArea;

   return pSpl;
}
/*------------------------------------------------------------------------*/
pSpline CloseSpline(WINDOWINFO *pwi)
{
   USHORT x;
   PPOINTLF pptl;
   ULONG ulBytes;
   pSpline pSpl;
   RECTL rcl;

   if (nr < 2 )
   {
      return NULL;
   }
   pSpl = OpenSpline(pwi);

   if (!pSpl)
      return NULL;

   ulBytes = (nr) * sizeof(POINTLF);

   pSpl->pptl = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

   pptl = pSpl->pptl;

   for (x=0; x < nr; x++)
   {
      pptl->x = (float)splineptl[x].x;
      pptl->y = (float)splineptl[x].y;
      pptl->x /= (float)pwi->usFormWidth;
      pptl->y /= (float)pwi->usFormHeight;

      pptl++;
   }
   pSpl->nrpts = nr;
   pSpl->ulState = ulState;
   SplineOutLine((POBJECT)pSpl,&rcl,pwi,TRUE);

   pSpl->bt.ptlfCenter.x = (float)rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
   pSpl->bt.ptlfCenter.y = (float)rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;

   pSpl->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
   pSpl->bt.ptlfCenter.y /= (float)pwi->usFormHeight;

   ObjRefresh((POBJECT)pSpl,pwi);
   return pSpl;
}
/*------------------------------------------------------------------------*/
pSpline splArrow(WINDOWINFO *pwi, POINTL *ptlMouse, long lFigure)
{
   USHORT x;
   PPOINTLF pptl;
   ULONG ulBytes,i,z;
   pSpline pSpl;
   RECTL rcl;
   POINTL *pl;
   int iType;

   if (lFigure < 0)
      return NULL;

   pSpl = OpenSpline(pwi);

   if (!pSpl)
      return NULL;

   pl = getFigure(lFigure, &ulBytes,&iType);

   if (!pl || !ulBytes)
      return NULL;

   pSpl->pptl = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));
   pptl       = pSpl->pptl;
   
   z = ulBytes / sizeof(POINTL);

   for (i = 0; i < z; i++)
   {
      pptl->x = (float)(pl->x + ptlMouse->x);
      pptl->y = (float)(pl->y + ptlMouse->y);
      pptl->x /= (float)pwi->usFormWidth;
      pptl->y /= (float)pwi->usFormHeight;
      pptl++;
      pl++;
   }
   pSpl->nrpts = ulBytes / sizeof(POINTLF);
   pSpl->ulState |= SPL_CLOSED;
   pSpl->ulState |= iType;
   SplineOutLine((POBJECT)pSpl,&rcl,pwi,TRUE);

   pSpl->bt.ptlfCenter.x = (float)rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
   pSpl->bt.ptlfCenter.y = (float)rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;

   pSpl->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
   pSpl->bt.ptlfCenter.y /= (float)pwi->usFormHeight;

   ObjRefresh((POBJECT)pSpl,pwi);
   return pSpl;
}
/*------------------------------------------------------------------------*/
/*  SplinShowHandles                                                      */
/*                                                                        */
/*  Description : Function shows the handles on the linejoins.            */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID SplinShowHandles(POBJECT pObj, WINDOWINFO *pwi)
{
   POINTL ptl;
   USHORT i;
   PPOINTLF pptl;
   pSpline pSpl = (pSpline)pObj;


   if (!pSpl || !pSpl->nrpts)
      return;

   pptl = pSpl->pptl;

   for ( i = 0; i < pSpl->nrpts; i++)
   {
      ptl.x =(LONG)(pptl->x * pwi->usFormWidth);
      ptl.y =(LONG)(pptl->y * pwi->usFormHeight);
      if (i== pObj->iHandle)
         DrawHandle(pwi->hps,ptl.x,ptl.y);
      else
         DrawMovePointHandle(pwi->hps,ptl.x,ptl.y);
      pptl++;
   }
}
/*------------------------------------------------------------------------*/
/*  SplinHitHandle                                                        */
/*                                                                        */
/*  Description : Looks if the mousepointer is above a linejoin which     */
/*                was drawn with SplinShowHandles.                        */
/*                Indirectly called during a WM_BUTTON1DOWN(?)            */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL SplinHitHandle(POBJECT pObj, WINDOWINFO *pwi, POINTL ptlMouse)
{
   POINTL ptl;
   USHORT i;
   PPOINTLF pptl;
   RECTL rcl;
   pSpline pSpl = (pSpline)pObj;

   if (!pSpl || !pSpl->nrpts)
      return FALSE;

   pptl = pSpl->pptl;

   for ( i = 0; i < pSpl->nrpts; i++)
   {

      ptl.x = (LONG)(pptl->x * pwi->usFormWidth );
      ptl.y = (LONG)(pptl->y * pwi->usFormHeight);
      pptl++;
      rcl.xLeft   = ptl.x - HANDLESIZE;
      rcl.xRight  = ptl.x + HANDLESIZE;
      rcl.yBottom = ptl.y - HANDLESIZE;
      rcl.yTop    = ptl.y + HANDLESIZE;

      if ((ptlMouse.x >= rcl.xLeft && ptlMouse.x <= rcl.xRight) &&
          (ptlMouse.y >= rcl.yBottom && ptlMouse.y <= rcl.yTop))
         return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/*  SplinDelHandle.                                                       */
/*                                                                        */
/*  Description : Deletes the selected handle.                            */
/*                                                                        */
/*  Parameters  : POBJECT pObj - pointer to the selected polygon.         */
/*                WINDOWINFO * - Not uses yet.                            */
/*                                                                        */
/* Returns      : BOOL TRUE on succes else FALSE.                         */
/*------------------------------------------------------------------------*/
BOOL SplinDelHandle(POBJECT pObj)
{
   BOOL bRet = FALSE;
   if (pObj->iHandle > 0 )
   {
      delVertex((pSpline)pObj,pObj->iHandle);
      bRet = TRUE;
   }
   pObj->iHandle = -1;
   return bRet;
}
/*-------------------------------------------------[ public ]-------------*/
BOOL splineMoveHandle(POBJECT pObj, WINDOWINFO *pwi, long ldx,long ldy)
{
   POINTLF ptf,*pptl;
   pSpline pSpl = (pSpline)pObj;

   if (pObj && pObj->iHandle >= 0)
   {
      ptf.x = (float)ldx;
      ptf.x /=(float)pwi->usFormWidth;
      ptf.y = (float)ldy;
      ptf.y /=(float)pwi->usFormHeight;

      pptl = (POINTLF *)(pSpl->pptl+pObj->iHandle);
      pptl->x += ptf.x;
      pptl->y += ptf.y;
      return TRUE;
   }
   return FALSE;
}
/*-------------------------------------------------[ private ]------------*/
/* PolyDraw                                                               */
/*                                                                        */
/* Returns the return value of the GPI function drawing the line.         */
/* This is needed for when the user is selecting the polygon.  GPI_HITS!  */
/*------------------------------------------------------------------------*/
static long PolyDraw(HPS hps,pSpline pSpl, WINDOWINFO *pwi,
                     double usRotate, POINTL *pt, BOOL bShade)
{
   USHORT i;
   PPOINTLF pptl;
   long lRet;

   pptl = pSpl->pptl;

   if ((pSpl->ulState & SPL_SPLINE))
   {
      /*
      ** Use our points buffer defined at the top of
      ** this file to put in the calculated points...
      */
      for ( i = 0; i < pSpl->nrpts; i++)
      {

         splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
         splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
         splineptl[i].x +=(LONG)(pwi->fOffx);
         splineptl[i].y +=(LONG)(pwi->fOffy);
         pptl++;
      }


      if (pSpl->ulState & SPL_CLOSED )
      {
         pptl = pSpl->pptl;
         splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
         splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
         splineptl[i].x +=(LONG)(pwi->fOffx);
         splineptl[i].y +=(LONG)(pwi->fOffy);
      }
      else
        i--;
      /*
      ** Give the polygon some offset since we are here to draw
      ** the shading, if of course that boolean is true.....
      */
      if (bShade)
         setShadingOffset(pwi,pSpl->bt.Shade.lShadeType, 
                          pSpl->bt.Shade.lUnits,splineptl,i+1);

      if (usRotate)
      {
         POINTL ptlCenter;
         if (!pt)
         {
            ptlCenter.x = (LONG)(pSpl->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pSpl->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter= *pt;

         if (bShade)
            setShadingOffset(pwi,pSpl->bt.Shade.lShadeType, 
                             pSpl->bt.Shade.lUnits,&ptlCenter,1);

         RotateSqrSegment(usRotate,ptlCenter,splineptl,i+1);

      }

      GpiMove(pwi->hps,&splineptl[0]);
      if (i > 1 )
      {
         lRet = GpiPolyFillet(pwi->hps,(LONG)(i),&splineptl[1]);

         if (!(pSpl->ulState & SPL_CLOSED))
         {
            drwStartPt(pwi,pSpl->bt.arrow,splineptl[0],splineptl[1]);
            drwEndPt(pwi,pSpl->bt.arrow,
                     splineptl[pSpl->nrpts-2],splineptl[pSpl->nrpts-1]);
         }
      }
      else
      {
         lRet = GpiLine(pwi->hps,&splineptl[1]);
         drwEndPoints(pwi,pSpl->bt.arrow,splineptl[0],splineptl[1]);
      }
   }
   else
   {
      /*
      ** Start working on a simple polyline....
      */
      for ( i = 0; i < pSpl->nrpts; i++)
      {
         splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
         splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
         splineptl[i].x +=(LONG)(pwi->fOffx);
         splineptl[i].y +=(LONG)(pwi->fOffy);
         pptl++;
      }
      /*
      ** If we are closed than connect the last line with the
      ** first one.
      */

      if (pSpl->ulState & SPL_CLOSED )
      {
         pptl = pSpl->pptl;
         splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
         splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
         splineptl[i].x +=(LONG)(pwi->fOffx);
         splineptl[i].y +=(LONG)(pwi->fOffy);
      }
      else
         i--;

      if (bShade)
         setShadingOffset(pwi,pSpl->bt.Shade.lShadeType, 
                          pSpl->bt.Shade.lUnits,splineptl,i+1);

      if (usRotate)
      {
         POINTL ptlCenter;

         if (!pt)
         {
            ptlCenter.x = (LONG)(pSpl->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pSpl->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter = *pt;

         if (bShade)
            setShadingOffset(pwi,pSpl->bt.Shade.lShadeType, 
                             pSpl->bt.Shade.lUnits,&ptlCenter,1);

         RotateSqrSegment(usRotate,ptlCenter,splineptl,i+1);
      }
      GpiMove(pwi->hps,&splineptl[0]);
      if (i > 1)
      {
         lRet = GpiPolyLine(pwi->hps,(LONG)(i),&splineptl[1]);

         if (!(pSpl->ulState & SPL_CLOSED))
         {
            drwStartPt(pwi,pSpl->bt.arrow,splineptl[0],splineptl[1]);
            drwEndPt(pwi,pSpl->bt.arrow,
                     splineptl[pSpl->nrpts-2],splineptl[pSpl->nrpts-1]);
         }
      }
      else
      {
         lRet = GpiLine(pwi->hps,&splineptl[1]);
         drwEndPoints(pwi,pSpl->bt.arrow,splineptl[0],splineptl[1]);
      }
   }
   return lRet;
}
/*-----------------------------------------------[ private ]-----------------*/
/* Name         : splCalcOutline                                             */
/*                                                                           */
/* Description  : Calculates the spline outline.                             */
/*---------------------------------------------------------------------------*/
static void splCalcOutline(pSpline pSpl, RECTL *prcl,WINDOWINFO *pwi)
{
      GpiResetBoundaryData(pwi->hps);
      GpiSetDrawControl(pwi->hps,DCTL_BOUNDARY,DCTL_ON);
      GpiSetDrawControl(pwi->hps,DCTL_DISPLAY, DCTL_OFF);
      PolyDraw(pwi->hps,pSpl,pwi,(SHORT)0,(POINTL *)0,FALSE);
      GpiSetDrawControl(pwi->hps,DCTL_BOUNDARY,DCTL_OFF);
      GpiSetDrawControl(pwi->hps,DCTL_DISPLAY, DCTL_ON);
      GpiQueryBoundaryData(pwi->hps,prcl);
}
/*-----------------------------------------------[ private ]--------------*/
/*  splSadhow.                                                            */
/*------------------------------------------------------------------------*/
static void splShadow(pSpline pSpl,HPS hps, WINDOWINFO *pwi)
{
   BOOL bArea = FALSE;

   GpiSetLineType(hps,pSpl->bt.line.LineType);
   GpiSetColor(hps,pSpl->bt.Shade.lShadeColor);

   if ( pSpl->bt.lPattern != PATSYM_DEFAULT )
   {
      GpiSetPattern(hps, PATSYM_SOLID);
      bArea = TRUE;
      GpiBeginArea(hps,BA_NOBOUNDARY | BA_ALTERNATE);
   }

   PolyDraw(pwi->hps,pSpl,pwi,(double)0,(POINTL *)0,TRUE);

   if ( bArea )
      GpiEndArea(hps);
}
/*-----------------------------------------------[ public ]---------------*/
/*  splDrawOutline.                                                       */
/*------------------------------------------------------------------------*/
void splDrawOutline(POBJECT pObj,WINDOWINFO *pwi, BOOL bIgnorelayer)
{
   pSpline pSpl = (pSpline)pObj;

   if (pwi->usdrawlayer == pSpl->bt.usLayer || bIgnorelayer)
      PolyDraw(pwi->hps,(pSpline )pObj,pwi,(double)0,(POINTL *)0,FALSE);
   return;
}
/*------------------------------------------------------------------------*/
/*  SplinDragHandle                                                       */
/*                                                                        */
/*  Description : Drags one handle of the spline. When the WM_BUTTON1DOWN */
/*                founds a hit in one of the handles which are on the line*/
/*                joins, the window procedure will go directly into the   */
/*                selected object with the same mouse position.           */
/*                                                                        */
/*                When a button1down message is received and the mp2      */
/*                parameter contains a KC_CTRL the function verifies if   */
/*                the click is on one of the polygons lines and if so the */
/*                point will be inserted at that mouse position.          */
/*                                                                        */
/*  Parameters  : POBJECT - pointer to a spline object.                   */
/*                WINDOWINFO * - pointer to our windowinfo struct.        */
/*                POINTL  ptlMouse - mouse position.                      */
/*                ULONG   ulMode contains the window message:             */
/*                        WM_BUTTON1DOWN initialize...                    */
/*                        WM_MOUSEMOVE   Show changes while dragging.     */
/*                        WM_BUTTON1DOWN Store new position in object     */
/*                MPARAM mp2 : Contains the keyboard state (shift,ctrl or */
/*                        alt).                                           */
/*                                                                        */
/*  Return : BOOL TRUE on succes. Action should be canceled in windowproc */
/*           if retvalue is FALSE.                                        */
/*------------------------------------------------------------------------*/
BOOL SplinDragHandle(POBJECT pObj, WINDOWINFO *pwi, POINTL ptlMouse, ULONG ulMode, MPARAM mp2)
{
   POINTL ptl;
   int iVertex;
   USHORT p;
   PPOINTLF pptl;
   POINTLF  ptf;
   RECTL rcl;
   BOOL  bFound;

   pSpline pSpl = (pSpline)pObj;

   if (!pSpl || !pSpl->nrpts)
      return FALSE;

   pptl = pSpl->pptl;

   switch (ulMode)
   {
      case WM_BUTTON1DOWN:
         bFound = FALSE;
         pObj->iHandle = -1;
         for ( p = 0; (p < pSpl->nrpts && !bFound ); p++)
         {
            ptl.x = (LONG)(pptl->x * pwi->usFormWidth );
            ptl.y = (LONG)(pptl->y * pwi->usFormHeight);
            pptl++;

            rcl.xLeft   = ptl.x - HANDLESIZE;
            rcl.xRight  = ptl.x + HANDLESIZE;
            rcl.yBottom = ptl.y - HANDLESIZE;
            rcl.yTop    = ptl.y + HANDLESIZE;

            if ((ptlMouse.x >= rcl.xLeft && ptlMouse.x <= rcl.xRight) &&
               (ptlMouse.y >= rcl.yBottom && ptlMouse.y <= rcl.yTop))
            {
               pObj->iHandle = p;
               bFound = TRUE;
            }
         }

         if (!bFound && (SHORT2FROMMP(mp2) == KC_CTRL))
         {
            iVertex = ptOnLine(pwi,&ptlMouse,pSpl);
            if (iVertex > 0)
            {
               ptf.x = (float)ptlMouse.x;
               ptf.y = (float)ptlMouse.y;
               ptf.x /=(float)pwi->usFormWidth;
               ptf.y /=(float)pwi->usFormHeight;
               bFound = insVertex(pSpl,&ptf,iVertex);
               pObj->iHandle = -1;
            }
         }
         return bFound;

      case WM_MOUSEMOVE:
         if (pObj->iHandle >= 0 && pObj->iHandle < pSpl->nrpts)
         {
             ptf.x = (float)ptlMouse.x;
             ptf.x /=(float)pwi->usFormWidth;
             ptf.y = (float)ptlMouse.y;
             ptf.y /=(float)pwi->usFormHeight;

             pptl = (POINTLF *)(pSpl->pptl+pObj->iHandle);
             pptl->x = ptf.x;
             pptl->y = ptf.y;
             PolyDraw(pwi->hps,pSpl,pwi,(double)0, (POINTL *)0,FALSE);
         }
         return TRUE;

      case WM_BUTTON1UP:
         if (pObj->iHandle >= 0 && pObj->iHandle < pSpl->nrpts)
         {
            (pSpl->pptl+pObj->iHandle)->x = (float)ptlMouse.x;
            (pSpl->pptl+pObj->iHandle)->y = (float)ptlMouse.y;
            (pSpl->pptl+pObj->iHandle)->x /= (float)pwi->usFormWidth;
            (pSpl->pptl+pObj->iHandle)->y /= (float)pwi->usFormHeight;

         }
         return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/* SplinMoveOutLine                                                       */
/*                                                                        */
/* Description   : Move the outline of the spline.                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID SplinMoveOutLine (POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   POINTL ptl;
   USHORT i;
   PPOINTLF pptl;
   pSpline pSpl = (pSpline)pObj;

   if (!pSpl || !pSpl->nrpts)
      return;

   pptl = pSpl->pptl;

   ptl.x = (LONG)(pptl->x * pwi->usFormWidth );
   ptl.y = (LONG)(pptl->y * pwi->usFormHeight);
   ptl.x += dx;
   ptl.y += dy;
   ptl.x +=(LONG)(pwi->fOffx);
   ptl.y +=(LONG)(pwi->fOffy);


   GpiMove(pwi->hps, &ptl);
   /*
   ** Use our points buffer defined at the top of
   ** this file to put in the calculated points...
   */
   for ( i = 0; i < pSpl->nrpts - 1; i++)
   {
      pptl++;
      splineptl[i].x =(LONG)(pptl->x * pwi->usFormWidth );
      splineptl[i].y =(LONG)(pptl->y * pwi->usFormHeight);
      splineptl[i].x += dx;
      splineptl[i].y += dy;
      splineptl[i].x +=(LONG)(pwi->fOffx);
      splineptl[i].y +=(LONG)(pwi->fOffy);
   }

   if (pSpl->ulState & SPL_CLOSED )
   {
      pptl = pSpl->pptl;
      splineptl[i].x =(LONG)(pptl->x * pwi->usFormWidth );
      splineptl[i].y =(LONG)(pptl->y * pwi->usFormHeight);
      splineptl[i].x += dx;
      splineptl[i].y += dy;
      splineptl[i].x +=(LONG)(pwi->fOffx);
      splineptl[i].y +=(LONG)(pwi->fOffy);
      i++;
   }

   if ((pSpl->ulState & SPL_SPLINE))
   {
      GpiPolyFillet(pwi->hps,(LONG)i,splineptl);
   }
   else
   {
      GpiPolyLine(pwi->hps,(LONG)i,splineptl);
   }

   if (!(pSpl->ulState & SPL_CLOSED) && i > 1 )
   {
      drwStartPt(pwi,pSpl->bt.arrow,ptl,splineptl[0]);
      drwEndPt(  pwi,pSpl->bt.arrow,splineptl[pSpl->nrpts-3],splineptl[pSpl->nrpts-2]);
   }
   else if (!(pSpl->ulState & SPL_CLOSED))
   {
      drwEndPoints(pwi,pSpl->bt.arrow,ptl,splineptl[0]);
   }
}
/*------------------------------------------------------------------------*/
/*  SplineCopy                                                            */
/*                                                                        */
/*  Description : Make a copy of the given spline segment and returns it. */
/*                                                                        */
/*  Parameters : pSpline pSpl - pointer to original spline.               */
/*                                                                        */
/*  Returns:  a new spline object.                                        */
/*------------------------------------------------------------------------*/
POBJECT copySpline(POBJECT pObj)
{
    ULONG    ulBytes;
    PPOINTLF pptl;
    USHORT   x;
    pSpline  pCopy;
    pSpline  pOrg;

    pCopy = (pSpline)pObjNew(NULL,pObj->usClass);

    memcpy(pCopy,pObj,ObjGetSize(CLS_SPLINE));

    pOrg = (pSpline)pObj;

    ulBytes = pOrg->nrpts * sizeof(POINTL);

    pCopy->pptl = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

    pptl = pOrg->pptl;

    for (x=0; x < pOrg->nrpts; x++)
    {
      pCopy->pptl[x].x = pptl[x].x;
      pCopy->pptl[x].y = pptl[x].y;
    }
    return (POBJECT)pCopy;
}
/*------------------------------------------------------------------------*/
void DrawSplineSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl)
{
   RECTL rcl,rclDest;
   ULONG  flOptions = 0;
   BOOL   bArea = FALSE;
   pSpline pSpl = (pSpline)pObj;

   GpiSetMix(hps,FM_DEFAULT);

   if (pwi->usdrawlayer == pSpl->bt.usLayer)
   {
      if (prcl)
      {
         SplineOutLine(pObj,&rcl,pwi,TRUE);
         if (!WinIntersectRect(hab,&rclDest,prcl,&rcl) || rcl.yTop <= 0)
         return;
      }

      if (pSpl->bt.Shade.lShadeType != SHADE_NONE)
         splShadow(pSpl,hps,pwi);

      GpiSetPattern(hps, pSpl->bt.lPattern); /*set filling pattern.  */
      GpiSetColor(hps,pSpl->bt.fColor);      /*Set the filling color.*/

      if ( pSpl->bt.lPattern != PATSYM_DEFAULT &&
           pSpl->bt.lPattern != PATSYM_GRADIENTFILL &&
           pSpl->bt.lPattern != PATSYM_FOUNTAINFILL )
      {
         /*
         ** Alright here we go. If the outlinecolor is
         ** the same as the filling color than take the
         ** BA_BOUNDARY in the calculation...??
         ** else exclusive outline.
         */
         if (pSpl->bt.line.LineColor == pSpl->bt.fColor)
            flOptions = BA_BOUNDARY;
         else
            flOptions = BA_NOBOUNDARY;

         GpiBeginArea(hps,flOptions | BA_ALTERNATE);
         bArea = TRUE;
      }
      else if (pSpl->bt.lPattern == PATSYM_GRADIENTFILL ||
               pSpl->bt.lPattern == PATSYM_FOUNTAINFILL )
      {
         bArea = TRUE;

         if (pwi->ulUnits == PU_PELS)
         {
            HPS hpsTemp = pwi->hps;
            SIZEL  sizlx;
            sizlx.cx = sizlx.cy = 2000L;
            /*
            ** export as bitmap!
            */          
            pwi->hps = GpiCreatePS( hab , (HDC)0, &sizlx, PU_PELS | GPIA_NOASSOC | GPIT_NORMAL );
            splCalcOutline(pSpl,&rcl,pwi);
            GpiDestroyPS(pwi->hps);
            pwi->hps = hpsTemp;            
         }
         else
         {
            if (prcl)
               SplineOutLine(pObj,&rcl,pwi,FALSE);
            else
               SplGetClippingRect(pObj,pwi,&rcl);
         }
         GpiBeginPath( hps, 1L);  /* define a clip path    */
      }

      if (bArea)
      {
         /*
         ** Only draw an invisible outline when an area is
         ** under construction.
         */
         GpiSetLineType(hps,LINETYPE_INVISIBLE);
         PolyDraw(hps,pSpl,pwi,(SHORT)0,(POINTL *)0,FALSE);
         GpiCloseFigure(hps);

      }

      if (pSpl->bt.lPattern != PATSYM_DEFAULT &&
          pSpl->bt.lPattern != PATSYM_GRADIENTFILL &&
          pSpl->bt.lPattern != PATSYM_FOUNTAINFILL )
      {
         GpiEndArea(hps);
      }
      else if (pSpl->bt.lPattern == PATSYM_GRADIENTFILL )
      {
         GpiEndPath(hps);
         GpiSetClipPath(hps,1L,SCP_AND);
         GpiSetPattern(hps,PATSYM_SOLID);
         GradientFill(pwi,hps,&rcl,&pSpl->bt.gradient);
         GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
      }
      else if (pSpl->bt.lPattern == PATSYM_FOUNTAINFILL )
      {
         GpiEndPath(hps);
         GpiSetClipPath(hps,1L,SCP_AND);
         GpiSetPattern(hps,PATSYM_SOLID);
         FountainFill(pwi,hps,&rcl,&pSpl->bt.fountain);
         GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
      }

      /*
      ** Start working on the outline.
      */
      GpiSetColor(hps,pSpl->bt.line.LineColor);
      GpiSetLineType(hps,pSpl->bt.line.LineType);

      if (pSpl->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         LONG lLineWidth = pSpl->bt.line.LineWidth; 

         if (pwi->ulUnits == PU_PELS)
         {
            lLineWidth = (lLineWidth * pwi->xPixels)/10000;
         }
         GpiSetPattern(hps, PATSYM_SOLID);
         GpiSetLineJoin(hps,pSpl->bt.line.LineJoin);
         GpiSetLineEnd(hps,pSpl->bt.line.LineEnd);
         GpiSetLineWidthGeom(hps,lLineWidth);
         GpiBeginPath( hps, 1L);  /* define a clip path    */
      }

      PolyDraw(hps,pSpl,pwi,(SHORT)0,(POINTL *)0,FALSE); /* Draw the outline.....*/

      if (pSpl->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
      {
         GpiEndPath(hps);
         GpiStrokePath (hps, 1, 0);
      }
   }     /*layer stuff */
}
/*------------------------------------------------------------------------*/
VOID * SplineSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   pSpline pSpl = (pSpline)pObj;
   long lRet;
   if (pSpl->bt.usLayer == pwi->uslayer || pwi->bSelAll)
   {
      if (pwi->bOnArea)
      {
         if (WinPtInRect((HAB)0,&pObj->rclOutline,&ptl))
            return (void *)pSpl;
      }
      else
      {
         lRet = PolyDraw(pwi->hps,pSpl,pwi,(double)0,(POINTL *)0,FALSE);
         if (lRet == GPI_HITS)
            return (void *)pSpl;
      }
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
void SplineOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc)
{
   pSpline pSpl = (pSpline)pObj;

   if (!pSpl) return;

   if (bCalc)
   {
      splCalcOutline(pSpl,&pObj->rclOutline,pwi);

      if (pSpl->bt.lPattern == PATSYM_GRADIENTFILL ||
          pSpl->bt.lPattern == PATSYM_FOUNTAINFILL )
      {
         /*
         ** See dlg_cir.c for comments on this piece of code.
         ** CirOutline function.
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
void SplineInvArea(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc)
{
   pSpline pSpl = (pSpline)pObj;

   SplineOutLine((POBJECT)pSpl,rcl,pwi,TRUE);

   rcl->yTop    += pSpl->bt.line.LineWidth;
   rcl->yBottom -= pSpl->bt.line.LineWidth;
   rcl->xLeft   -= pSpl->bt.line.LineWidth;
   rcl->xRight  += pSpl->bt.line.LineWidth;

   if (bInc)
   {
      rcl->yTop    += (HANDLESIZE * 4);
      rcl->yBottom -= (HANDLESIZE * 4);
      rcl->xLeft   -= (HANDLESIZE * 4);
      rcl->xRight  += (HANDLESIZE * 4);
   }
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);

   if (!bInc)
   {
      rcl->yTop    += 2;
      rcl->xRight  += 2;
   }

}
/*------------------------------------------------------------------------*/
/* LineMoving function, if we are selected via the mouse .....            */
/*------------------------------------------------------------------------*/
VOID SplMoveSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy;
   float fcx,fcy;
   USHORT i;
   pSpline pSpl = (pSpline)pObj;

   fdx = (float)dx;
   fdy = (float)dy;
   fcx = pwi->usFormWidth;
   fcy = pwi->usFormHeight;

   for (i=0; i < pSpl->nrpts; i++)
   {
      pSpl->pptl[i].x += (float)( fdx / fcx );
      pSpl->pptl[i].y += (float)( fdy / fcy );
   }
   pSpl->bt.ptlfCenter.x += (float)( fdx / fcx );
   pSpl->bt.ptlfCenter.y += (float)( fdy / fcy );
 }
/*------------------------------------------------------------------------*/
static void AddPoint(PPOINTL pptl)
{
   if (nr < 0)
      nr=0;
   splineptl[nr].x = pptl->x;
   splineptl[nr].y = pptl->y;
   nr++;
}
/*------------------------------------------------------------------------*/
/*  SplineDelLastLine.                                                    */
/*                                                                        */
/*  Description : Deletes the last drawn line in the polyline if the      */
/*                user presses on the minus key while in splinedrawing    */
/*                mode.                                                   */
/*                                                                        */
/*  Parameters : SHORT smode - this comes directly from the WM_CHAR       */
/*                             from the main module.                      */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
void DelLastPoint(WINDOWINFO *pwi,USHORT usAction)
{
   if (nr > 1  && usAction == (USHORT)'-')
   {
      if (nr > 1)
      {
         if (ulType == POLYSPLINE)
         {
            GpiMove(pwi->hps,&splineptl[0]);
            GpiPolyFillet(pwi->hps,nr,splineptl);
            GpiSetMix(pwi->hps,FM_INVERT);
         }
         else
         {
            GpiMove(pwi->hps,&splineptl[0]);
            GpiPolyLine(pwi->hps,nr,splineptl);
            GpiSetMix(pwi->hps,FM_INVERT);
         }
      }
      nr--;
      if (nr > 1)
      {
         if (ulType == POLYSPLINE)
         {
            GpiMove(pwi->hps,&splineptl[0]);
            GpiPolyFillet(pwi->hps,nr,splineptl);
            GpiSetMix(pwi->hps,FM_INVERT);
         }
         else
         {
            GpiMove(pwi->hps,&splineptl[0]);
            GpiPolyLine(pwi->hps,nr,splineptl);
            GpiSetMix(pwi->hps,FM_INVERT);
         }
      }
   }
   else
      DosBeep(880,20);
}

void SplineSetup(ULONG ultype)
{
   nr=-1;
   ulType = ultype;

   if (ulType == POLYSPLINE)
      ulState = SPL_SPLINE;
   else
      ulState = 0;

   return;
}
/*------------------------------------------------------------------------*/
/*  CreateSpline.                                                         */
/*                                                                        */
/*  Description : When the operationmode of the main part of the program  */
/*                is SPLINEDRAW this function is called via the mouse     */
/*                messages.                                               */
/*                                                                        */
/*  Concepts : When all parametes are zero the function resets itself to  */
/*             the beginning of the array of points. On every mousebutton */
/*             up an additional point is created. Only once the mouse -   */
/*             button1down creates a new point. At the mouse button1up the*/
/*             linsegment is drawn in its final color and the mix mode is */
/*             inverted to leave the presentation space                   */
/*             in the right state.                                        */
/*                                                                        */
/*                                                                        */
/*  Parameters : WINDOWINFO *wi - for color info and PS.                  */
/*               PPOINTL   pptl - mouse coordinate's.                     */
/*               USHORT    mode - WM_BUTTON1DOWN, WM_BUTTON1UP and        */
/*                                WM_MOUSEMOVE.                           */
/*                                                                        */
/*  USES       : Globals  USHORT nr - index in gplobal points array.      */
/*                        POINTL splineptl[50] - array of points.         */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT CreateSpline(WINDOWINFO *pwi, POINTL *ptl,ULONG ulMsg)
{
   static RECTL rclVertex; /* Starting point */
   POINTL pBox;
   static button1down;
   static pSpline p;

   switch (ulMsg)
   {
      case WM_BUTTON1DOWN:
         if (nr > 1)
         {
            if (ulType == POLYSPLINE)
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyFillet(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
            else
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyLine(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
         }

         AddPoint(ptl);

         if (!button1down)
         {
            rclVertex.xLeft   = ptl->x - HANDLESIZE;
            rclVertex.xRight  = ptl->x + HANDLESIZE;
            rclVertex.yTop    = ptl->y + HANDLESIZE;
            rclVertex.yBottom = ptl->y - HANDLESIZE;
            pBox.x = ptl->x - HANDLESIZE;
            pBox.y = ptl->y + HANDLESIZE;
            GpiMove(pwi->hps,&pBox);
            pBox.x = ptl->x + HANDLESIZE;
            pBox.y = ptl->y - HANDLESIZE;
            GpiSetMix(pwi->hps,FM_OVERPAINT);
            GpiSetColor(pwi->hps,pwi->ulColor);
            GpiBox(pwi->hps,DRO_FILL,&pBox,0,0);
         }

         if (nr > 1)
         {
            if (ulType == POLYSPLINE)
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyFillet(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
            else
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyLine(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
         }

         button1down = TRUE;
         return (MRESULT)0;

      case WM_MOUSEMOVE:
         if (nr > 1)
         {
            if (ulType == POLYSPLINE)
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyFillet(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
               splineptl[nr-1].x = ptl->x;
               splineptl[nr-1].y = ptl->y;
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyFillet(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
            else
            {
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyLine(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
               splineptl[nr-1].x = ptl->x;
               splineptl[nr-1].y = ptl->y;
               GpiMove(pwi->hps,&splineptl[0]);
               GpiPolyLine(pwi->hps,nr,splineptl);
               GpiSetMix(pwi->hps,FM_INVERT);
            }
         }
         else
         {
            splineptl[nr].x = ptl->x;
            splineptl[nr].y = ptl->y;
         }
         return (MRESULT)0;

      case WM_BUTTON1UP:
         if (nr > 2)
         {
            if (WinPtInRect((HAB)0,&rclVertex,ptl))
            {
               button1down = FALSE;
               /*
               ** If there is a hit, close the current spline or poly
               ** line and init for the next one.
               */
               nr--;
               p = CloseSpline(pwi);
               if (!p) return (MRESULT)0;
               p->ulState  = ulState;
               p->ulState |= SPL_CLOSED;
               nr = -1;
               return (MRESULT)0;
            }
         }
         return (MRESULT)0;

      case WM_CHAR: /* A user pressed enter during creation */
         if (nr > 2)
         {
            button1down = FALSE;
            p = CloseSpline(pwi);
            if (!p) return (MRESULT)0;
            p->ulState  = ulState;
            p->ulState |= SPL_CLOSED;
            nr = -1;
            return (MRESULT)0;
         }
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/* Here we show the spline/polyline dialog                                */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY PolyLineDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
   static pSpline pS;
   static HWND    hClient;
   static BOOL    bInit;
   SWP    swp;            /* Screen Window Position Holder     */
   ULONG ulStorage[2];    /* To get the vals out of the spins  */
   PVOID pStorage;        /* idem spinbutton.                  */
   WINDOWINFO *pwi;

   switch (msg)
   {
      case WM_INITDLG:
         bInit = TRUE;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

        pwi = (WINDOWINFO *)mp2;
        hClient = pwi->hwndClient;    /* Needed for a refresh */
        pS = (pSpline)pwi->pvCurrent; /* Our spline           */

        /* setup the layer spin button */

        WinSendDlgItemMsg( hwnd, ID_SPLINELAYER, SPBM_SETLIMITS,
                           MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

        WinSendDlgItemMsg( hwnd, ID_SPLINELAYER, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)pS->bt.usLayer), NULL);

        if ((pS->ulState & SPL_CLOSED))
           WinSendDlgItemMsg(hwnd,ID_SPLINECLOSE,BM_SETCHECK,MPFROMSHORT(1),(MPARAM)0);
        else
           WinSendDlgItemMsg(hwnd,ID_SPLINEDEF,BM_SETCHECK,MPFROMSHORT(1),(MPARAM)0);

        if ((pS->ulState & SPL_SPLINE))
           WinSendDlgItemMsg(hwnd,ID_SPLINEFILLET,BM_SETCHECK,MPFROMSHORT(1),(MPARAM)0);
        else
           WinSendDlgItemMsg(hwnd,ID_SPLINELINE,BM_SETCHECK,MPFROMSHORT(1),(MPARAM)0);
        bInit = FALSE;
        return 0;

     case WM_CONTROL:
        if (bInit)
           return 0;
        switch(LOUSHORT(mp1))
        {
           case ID_SPLINECLOSE:
              pS->ulState |= SPL_CLOSED;
              break;
           case ID_SPLINEDEF:
              pS->ulState &= ~SPL_CLOSED;
              break;
           case ID_SPLINEFILLET:
              pS->ulState |= SPL_SPLINE;
              break;
           case ID_SPLINELINE:
              pS->ulState &= ~SPL_SPLINE;
              break;
        }
        return 0;
     case WM_COMMAND:
        switch(LOUSHORT(mp1))
	{
           case DID_OK:
              /*-- get our layer info out of the spin --*/

              pStorage = (PVOID)ulStorage;

              WinSendDlgItemMsg(hwnd,
                                ID_SPLINELAYER,
                                SPBM_QUERYVALUE,
                                (MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= 1 && ulStorage[0] <= 10 )
                 pS->bt.usLayer = (USHORT)ulStorage[0];
              WinPostMsg(hClient,UM_ENDDIALOG,(MPARAM)pS,(MPARAM)0);
              WinDismissDlg(hwnd,TRUE);
              return 0;
           case DID_HELP:
              ShowDlgHelp(hwnd);
              return 0;
           case DID_CANCEL:
              WinDismissDlg(hwnd,FALSE);
              return 0;
        }
        WinDismissDlg(hwnd,TRUE);
        return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  Name: RegularPolyCreate                                               */
/*                                                                        */
/*  Description : Functions for creation of regular polypoints.           */
/*                                                                        */
/*  Parameters  : ULONG  ulMsg  contains the mouse message.                */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT RegularPolyPoint(ULONG ulMsg, POINTL mouse, WINDOWINFO *pwi)
{
   int     i;
   float   fdx,fdy;
   static  POINTLF  ptlf[54];
   double  alfa,straal;
   double  startAngle,sweep;
   pSpline p;
   POINTL  ptl;

   switch (ulMsg)
   {
      case WM_BUTTON1DOWN:
         ptlf[0].x = (float)mouse.x;
         ptlf[0].y = (float)mouse.y;
         nr = sPolyStar;
         return (MRESULT)0;

      case WM_MOUSEMOVE:
         ptlf[1].x = (float)mouse.x;
         ptlf[1].y = (float)mouse.y;

         fdx = ptlf[1].x - ptlf[0].x;
         fdy = ptlf[1].y - ptlf[0].y;

         if (fdx)
         {
            alfa = (double)(fdy / fdx);
         }
         else
         {
            alfa = (double)0;
         }
         /*
         ** Get starting anle.
         */
         startAngle = atan(alfa);
         /*
         ** Get the straal of the polypoint.
         */
         if (!startAngle)
            straal = fdx;
         else
         {
            straal = (double)fdy;
            straal /= (double)sin(startAngle);
         }

         sweep = (2 * PI ) / nr;

         for(i = 2; i <= nr + 1; i++)
         {
            startAngle -= sweep;
            ptlf[i].y = ptlf[0].y + (straal * sin(startAngle));
            ptlf[i].x = ptlf[0].x + (straal * cos(startAngle));
         }
         /*
         ** Show it!!
         ** Point [0] contains the center of the figure so all is drawn
         ** relative to [0].
         */
         ptl.x =(LONG)ptlf[1].x;
         ptl.y =(LONG)ptlf[1].y;

         GpiMove(pwi->hps,&ptl);

         for(i = 2; i <= nr + 1; i++)
         {
            ptl.x = (LONG)ptlf[i].x;
            ptl.y = (LONG)ptlf[i].y;
            GpiLine(pwi->hps,&ptl);
         }
         ptl.x = (LONG)ptlf[1].x;
         ptl.y = (LONG)ptlf[1].y;

         GpiLine(pwi->hps,&ptl);
         return (MRESULT)0;

      case WM_BUTTON1UP:
         /*
         ** Make the point absolute and put it in the spline struct.
         */

         for(i = 1; i <= nr + 1; i++)
         {
            splineptl[i-1].x = (LONG)ptlf[i].x;
            splineptl[i-1].y = (LONG)ptlf[i].y;
         }
         p = CloseSpline(pwi);
         if (!p) return (MRESULT)0;
         p->ulState  = 0;
         p->ulState |= SPL_CLOSED;
         return (MRESULT)0;
      default:
         break;
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: RegularPolyStar                                                 */
/*                                                                        */
/*  Description : Functions for creation of regular polypoints.           */
/*                                                                        */
/*  Parameters  : ULONG  ulMsg  contains the mouse message.               */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT RegularPolyStar(ULONG ulMsg, POINTL mouse, WINDOWINFO *pwi)
{
   int     i;
   float   fdx,fdy;
   static  POINTLF  ptlf[100];
   static  int  nrpts;
   double  alfa,straal;
   double  startAngle,sweep;
   pSpline p;
   POINTL  ptl;

   switch (ulMsg)
   {
      case WM_BUTTON1DOWN:
         nr = sPolyStar;
         nrpts = nr * 2;
         ptlf[0].x = (float)mouse.x;
         ptlf[0].y = (float)mouse.y;
         return (MRESULT)0;

      case WM_MOUSEMOVE:
         ptlf[1].x = (float)mouse.x;
         ptlf[1].y = (float)mouse.y;

         fdx = ptlf[1].x - ptlf[0].x;
         fdy = ptlf[1].y - ptlf[0].y;

         if (fdx)
         {
            alfa = (double)(fdy / fdx);
         }
         else
         {
            alfa = (double)0;
         }
         /*
         ** Get starting anle.
         */
         startAngle = atan(alfa);
         /*
         ** Get the straal of the polypoint.
         */
         if (!startAngle)
            straal = fdx;
         else
         {
            straal = (double)fdy;
            straal /= (double)sin(startAngle);
         }

         sweep = (2 * PI ) / nrpts;

         for(i = 2; i <= nrpts; i++)
         {
            startAngle -= sweep;
            if (i % 2)
            {
               ptlf[i].y = ptlf[0].y + (straal * sin(startAngle));
               ptlf[i].x = ptlf[0].x + (straal * cos(startAngle));
            }
            else
            {
               ptlf[i].y = ptlf[0].y + (straal * sin(startAngle)/2);
               ptlf[i].x = ptlf[0].x + (straal * cos(startAngle)/2);
            }
         }
         /*
         ** Show it!!
         ** Point [0] contains the center of the figure so all is drawn
         ** relative to [0].
         */
         ptl.x =(LONG)ptlf[1].x;
         ptl.y =(LONG)ptlf[1].y;

         GpiMove(pwi->hps,&ptl);

         for(i = 2; i <= nrpts; i++)
         {
            ptl.x = (LONG)ptlf[i].x;
            ptl.y = (LONG)ptlf[i].y;
            GpiLine(pwi->hps,&ptl);
         }
         ptl.x = (LONG)ptlf[1].x;
         ptl.y = (LONG)ptlf[1].y;

         GpiLine(pwi->hps,&ptl);
         return (MRESULT)0;

      case WM_BUTTON1UP:
         /*
         ** Make the point absolute and put it in the spline struct.
         */

         for(i = 1; i <= nrpts; i++)
         {
            splineptl[i-1].x = (LONG)ptlf[i].x;
            splineptl[i-1].y = (LONG)ptlf[i].y;
         }
         i = nr;
         nr = nrpts;
         p = CloseSpline(pwi);
         nr = i;
         p->ulState  = 0;
         p->ulState |= SPL_CLOSED;
         return (MRESULT)0;
      default:
         break;
   }
   return (MRESULT)0;
}

/*------------------------------------------------------------------------*/
/*  RegularPpointDlgProc.                                                 */
/*                                                                        */
/*  Description : Shows a dialog with a spinbutton. This is used to       */
/*                let the user define the number ofd point of the polypt  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY RegularPointDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   ULONG ulStorage[2];    /* To get the vals out of the spins  */
   PVOID pStorage;        /* idem spinbutton.                  */
   SWP   swp;

   switch(msg)
   {
      case WM_INITDLG:

		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx)),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy)),
		       0, 0, SWP_MOVE);


         WinSendDlgItemMsg( hwnd, ID_NRPOINTS, SPBM_SETLIMITS,
                            MPFROMLONG(MAXPTS),MPFROMLONG(MINPTS));
         WinSendDlgItemMsg( hwnd, ID_NRPOINTS, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)MINPTS), NULL);
         sPolyStar = 3;
         return (MRESULT)0;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
	 {
            case DID_OK:
               bDialog = FALSE;
               WinDismissDlg(hwnd,TRUE);
               break;
         }
         return (MRESULT)0;
      case WM_CONTROL:
         if (LOUSHORT(mp1) == ID_NRPOINTS)
         {
            if (HIUSHORT(mp1) == SPBN_CHANGE)
            {
              pStorage = (PVOID)ulStorage;
              WinSendDlgItemMsg(hwnd,
                                ID_NRPOINTS,
                                SPBM_QUERYVALUE,
                                (MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= MINPTS && ulStorage[0] <= MAXPTS )
                 sPolyStar = (USHORT)ulStorage[0];
            }
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}

void StartupPolyDlg(WINDOWINFO *pwi)
{
   if (!bDialog)
   {
      bDialog = TRUE;
      WinLoadDlg(HWND_DESKTOP,pwi->hwndClient,
                (PFNWP)RegularPointDlgProc,(HMODULE)0,
	        ID_POLYPOINT,NULL);
   }
}

MRESULT RegularPolyCreate(ULONG ulMsg, POINTL mouse, WINDOWINFO *pwi, ULONG type)
{
   if (type == REGPOLYDRAW)
      return RegularPolyPoint (ulMsg,mouse,pwi);
   else
      return RegularPolyStar(ulMsg,mouse,pwi);
}
/*------------------------------------------------------------------------*/
/*  SplPutInGroup.                                                        */
/*                                                                        */
/*  Description : Here we put the SPL in the given rectl of the group.    */
/*                In fact the original parent is replaced by a new one.   */
/*                When the SPL is not part of a group than the parent is  */
/*                the paper.                                              */
/*                                                                        */
/*  Parameters  : POBJECT pObj-pointer to a SPL object which is put in the*/
/*                             group.                                     */
/*                RECTL * - pointer to the rect of the group.             */
/*                WINDOWINFO * - pointer to the original windowinfo.      */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void SplPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pSpline pSpl = (pSpline)pObj;
   USHORT  usGrpWidth,usGrpHeight;
   PPOINTLF pptl;
   int i;

   if (!pSpl) return;
   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);
   /*
   ** Setup the new rotation center.
   */
   pSpl->bt.ptlfCenter.x = (float)(prcl->xLeft + (prcl->xRight - prcl->xLeft)/2);
   pSpl->bt.ptlfCenter.y = (float)(prcl->yBottom + (prcl->yTop - prcl->yBottom)/2);
   pSpl->bt.ptlfCenter.x /= (float)usGrpWidth;
   pSpl->bt.ptlfCenter.y /= (float)usGrpHeight;

   pptl = pSpl->pptl;

   for ( i = 0; i < pSpl->nrpts; i++)
   {
      pptl->x = (float)(pptl->x * pwi->usFormWidth );
      pptl->y = (float)(pptl->y * pwi->usFormHeight);
      pptl->x -= (float)prcl->xLeft;
      pptl->y -= (float)prcl->yBottom;
      pptl->x /= (float)usGrpWidth ;
      pptl->y /= (float)usGrpHeight;
      pptl++;
   }
}
/*------------------------------------------------------------------------*/
/*  SplRemFromGroup.                                                      */
/*                                                                        */
/*  Remove the object from the group.                                     */
/*------------------------------------------------------------------------*/
void SplRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pSpline pSpl = (pSpline)pObj;
   USHORT  usGrpWidth,usGrpHeight;
   RECTL rcl;
   PPOINTLF pptl;
   int  i;

   if (!pSpl) return;
   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   pptl = pSpl->pptl;

   for ( i = 0; i < pSpl->nrpts; i++)
   {
      pptl->x = (float)(pptl->x * usGrpWidth );
      pptl->y = (float)(pptl->y * usGrpHeight);
      pptl->x += (float)prcl->xLeft;
      pptl->y += (float)prcl->yBottom;
      pptl->x /= (float)pwi->usFormWidth;
      pptl->y /= (float)pwi->usFormHeight;
      pptl++;
   }
   /*
   ** Start restoring the centerpoint for rotation.
   */
   SplineOutLine((POBJECT)pSpl,&rcl,pwi,FALSE);
   pSpl->bt.ptlfCenter.x = (float)rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
   pSpl->bt.ptlfCenter.y = (float)rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   pSpl->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
   pSpl->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
   return;
}
/*------------------------------------------------------------------------*/
/*  SplStretch.                                                           */
/*                                                                        */
/*  Description : Stretches the spline. Called during the following msg's.*/
/*                WM_BUTTON1DOWN,WM_BUTTON1UP and WM_MOUSEMOVE.           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void SplStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg)
{
   static USHORT usWidth,usHeight;
   USHORT usOldWidth,usOldHeight;
   static float fOffx,fOffy;
   float fOldx,fOldy;
   int   i;
   PPOINTLF pptl;

   pSpline pSpl = (pSpline)pObj;

   if (!pSpl) return;

   usWidth  =(USHORT)(prcl->xRight - prcl->xLeft);
   usHeight =(USHORT)(prcl->yTop - prcl->yBottom);

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         pptl = pSpl->pptl;
         for ( i = 0; i < pSpl->nrpts; i++)
         {
            pptl->x = (float)(pptl->x * pwi->usFormWidth );
            pptl->y = (float)(pptl->y * pwi->usFormHeight);
            pptl->x -= (float)prcl->xLeft;
            pptl->y -= (float)prcl->yBottom;
            pptl->x /= (float)usWidth ;
            pptl->y /= (float)usHeight;
            pptl++;
         }
         pSpl->bt.ptlfCenter.x = (float)(pSpl->bt.ptlfCenter.x * pwi->usFormWidth );
         pSpl->bt.ptlfCenter.y = (float)(pSpl->bt.ptlfCenter.y * pwi->usFormHeight);
         pSpl->bt.ptlfCenter.x -= (float)prcl->xLeft;
         pSpl->bt.ptlfCenter.y -= (float)prcl->yBottom;
         pSpl->bt.ptlfCenter.x /= (float)usWidth ;
         pSpl->bt.ptlfCenter.y /= (float)usHeight;

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
         SplinMoveOutLine((POBJECT)pSpl,pwi,0,0);
         pwi->usFormWidth  = usOldWidth;
         pwi->usFormHeight = usOldHeight;
         pwi->fOffx = fOldx;
         pwi->fOffy = fOldy;
         return;

      case WM_BUTTON1UP:
         pptl = pSpl->pptl;
         for ( i = 0; i < pSpl->nrpts; i++)
         {
            pptl->x = (float)(pptl->x * usWidth );
            pptl->y = (float)(pptl->y * usHeight);
            pptl->x += (float)prcl->xLeft;
            pptl->y += (float)prcl->yBottom;
            pptl->x /= (float)pwi->usFormWidth;
            pptl->y /= (float)pwi->usFormHeight;
            pptl++;
         }
         pSpl->bt.ptlfCenter.x = (float)(pSpl->bt.ptlfCenter.x * usWidth );
         pSpl->bt.ptlfCenter.y = (float)(pSpl->bt.ptlfCenter.y  * usHeight);
         pSpl->bt.ptlfCenter.x += (float)prcl->xLeft;
         pSpl->bt.ptlfCenter.y += (float)prcl->yBottom;
         pSpl->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
         pSpl->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
         return;
   }
   return;
}
/*---------------------------------------------------------------------------*/
void SplDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg,POINTL *pt)
{
   POINTL ptlCenter;
   USHORT i;
   PPOINTLF pptl;

   pSpline pSpl = (pSpline)pObj;

   if (!pObj) return;

   switch (ulMsg)
   {
      case WM_BUTTON1UP:
         pptl = pSpl->pptl;
         for ( i = 0; i < pSpl->nrpts; i++)
         {
            splineptl[i].x = (LONG)(pptl->x * pwi->usFormWidth );
            splineptl[i].y = (LONG)(pptl->y * pwi->usFormHeight);
            splineptl[i].x +=(LONG)(pwi->fOffx);
            splineptl[i].y +=(LONG)(pwi->fOffy);
            pptl++;
         }

         if (!pt)
         {
            ptlCenter.x = (LONG)(pSpl->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pSpl->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter = *pt;

         RotateSqrSegment(lRotate,ptlCenter,splineptl,i);
         pptl = pSpl->pptl;
         for ( i = 0; i < pSpl->nrpts; i++)
         {
            splineptl[i].x -=(LONG)(pwi->fOffx);
            splineptl[i].y -=(LONG)(pwi->fOffy);
            pptl->x = (float)splineptl[i].x;
            pptl->y = (float)splineptl[i].y;
            pptl->x /= (float)pwi->usFormWidth;
            pptl->y /= (float)pwi->usFormHeight;
            pptl++;
         }
         break;
      case WM_MOUSEMOVE:
         PolyDraw(pwi->hps,pSpl,pwi,lRotate,pt,FALSE);
         break;
   }
   return;
}
/*------------------------------------------------------------------------*/
void splineSetLineEnd(POBJECT pObj, WINDOWINFO *pwi)
{
   pSpline pSpl = (pSpline)pObj;

   if (!pObj) 
      return;

   pSpl->bt.arrow.lEnd    = pwi->arrow.lEnd;
   pSpl->bt.arrow.lStart  = pwi->arrow.lStart;
   pSpl->bt.arrow.lSize   = pwi->arrow.lSize;
   return;
}
/*------------------------------------------------------------------------*/
/* splMakeClipPath                                                        */
/*                                                                        */
/* Description  : Makes a clippath of the given spline object.            */
/*                                                                        */
/* Parameters   : POBJECT pObj - reference to an object of type spline.   */
/*                WINDOWINFO *pwi - reference to the windowinformation.   */
/*                RECTL rcl    - The rectangle wherein the clipping should*/
/*                take place. Most important is that the spline area      */
/*                fall in this rectangle. Else the function returns FALSE */
/*                                                                        */
/* Returns      : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
void * splMakeClipPath(POBJECT pObj,WINDOWINFO *pwi, RECTL rcl,ULONG *plPoints, ULONG * ulType)
{
   pSpline pSpl = (pSpline)pObj;
   RECTL   rclSpline;
   POINTLF *pptlf;

   if (pSpl->nrpts < 4 )
      return NULL;
   /*
   ** 1 - Get area size in rclSpline.
   ** 2 - Check if spline falls inside given rect.
   ** 3 - Convert spline points to the given rect.
   ** 4 - Make a copy of the points and let pptl point to it.
   ** 5 - Finally give the number of points and return TRUE on success.
   */
   SplineOutLine(pObj,&rclSpline,pwi,FALSE);

   if (rclSpline.xLeft < rcl.xLeft || rclSpline.xRight > rcl.xRight)
      return NULL;

   if (rclSpline.yBottom < rcl.yBottom || rclSpline.yTop > rcl.yTop)
      return NULL;

   SplPutInGroup(pObj,&rcl,pwi);

   pptlf = (POINTLF *)calloc((pSpl->nrpts * sizeof(POINTLF)),sizeof(char));
   if (!pptlf)
      return NULL;

   memcpy((void *)pptlf,(void *)pSpl->pptl,(pSpl->nrpts * sizeof(POINTLF)));

   *plPoints = pSpl->nrpts;

   if ((pSpl->ulState & SPL_SPLINE))
      *ulType = SPL_SPLINE;
   else
      *ulType = 0;
   ObjDelete(pObj); /* Remove spline from drawing. */
   return (void *)pptlf;
}
/*------------------------------------------------------------------------*/
POBJECT openSqrSegment(POINTL ptl, WINDOWINFO *pwi)
{
   pSpline pSpl;
   POBJECT pObj;
   ULONG   ulBytes;

   ulBytes = 4 * sizeof(POINTLF);

   pSpl                   = (pSpline)pObjCreate(pwi, CLS_SPLINE);
   pSpl->ustype           = CLS_SPLINE;
   pSpl->LinDeleted       = FALSE;
   pSpl->LinMultiSelected = FALSE;
   pSpl->pptl             = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));
   /*
   ** Remember the first point of the square....
   */
   pSpl->pptl->x  = (float)ptl.x;
   pSpl->pptl->y  = (float)ptl.y;
   pObj = (POBJECT)pSpl;
   pObj->moveOutline = SplinMoveOutLine;
   pObj->paint       = DrawSplineSegment;
   pObj->getInvalidationArea    = SplineInvArea;
   return (POBJECT)pSpl;
}
/*---------------------------------------------------------------------------*/
POBJECT closeSqrSegment(POBJECT pObj, POINTL ptlEnd, WINDOWINFO *pwi)
{
   float fx,fy;
   int   i;
   POINTLF *pptl;
   pSpline pSpl;
   RECTL rcl;

   pSpl = (pSpline)pObj;

   pptl = pSpl->pptl;

   fx  = (float)ptlEnd.x;
   fy  = (float)ptlEnd.y;
   /*
   ** Get upperleft corner.
   **    0 ÚÄÄÄÄÄÄÄÄÄÄ¿1
   **      ³          ³
   **    3 ÀÄÄÄÄÄÄÄÄÄÄÙ2
   */
   if (fx < pptl->x)
   {
      fx = pptl[0].x;               /* save large X-value...*/
      pptl[0].x = (float)ptlEnd.x;
   }
   pptl[3].x = pptl[0].x;
   pptl[1].x = fx;
   pptl[2].x = fx;

   if (fy > pptl->y)
   {
      fy = pptl[0].y;               /* save small Y-value...*/
      pptl[0].y = (float)ptlEnd.y; 
   }
   pptl[3].y = fy;
   pptl[1].y = pptl[0].y;
   pptl[2].y = fy;

   for (i=0; i < 4; i++)
   {
      pptl->x /= (float)pwi->usFormWidth;
      pptl->y /= (float)pwi->usFormHeight;
      pptl++;
   }
   pSpl->nrpts   = 4;
   pSpl->ulState = SPL_CLOSED;
   SplineOutLine((POBJECT)pSpl,&rcl,pwi,TRUE);

   pSpl->bt.ptlfCenter.x = (float)rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
   pSpl->bt.ptlfCenter.y = (float)rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
   pSpl->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
   pSpl->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
   ObjRefresh((POBJECT)pSpl,pwi);
   return (POBJECT)pSpl;
}
