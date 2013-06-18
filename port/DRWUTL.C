/*---------------------------------------------------------------------------*/
/*  Name: drwutl.c                                                           */
/*                                                                           */
/* DrawMovePointHandle: Draws an open handle for point moving...             */
/* DrawHandle: Draws a closed handle (normal selection handle)               */
/* pObjCreate(USHORT usClass) : creates an object of a certain class.        */
/* pObjAppend : Appends an object to the list of objects.                    */
/* ObjGetBoxCorners : returns the coord of the oposite corner.....  .        */
/* ObjSetOutLineClr: Set outlinecolor of the given object.                   */
/* ObjSetFillClr: Set the filling color of a given object.                   */
/* ObjSetMltFillClr : Set the filling color of multiple selected obj's       */
/* ObjDrawSelected: Draws only the (multi)selected obj in the given PS.      */
/* ObjInterChange : Change the position of an object in the chain.....       */
/* ObjLntypeChange: Changes the linetype of the selected objects.            */
/* ObjLnWidthChange: Changes the line width of the given object.             */
/* ObjMultiLntypeChange: Changes the linetype of multiple selected obj's     */
/* ObjMultiLnWidthChange: Changes the linewidth of multiple selected obj's   */
/* ObjPatternChange:Changes the filling pattern of the selected object.      */
/* ObjMultiPatternChange: Changes the filling pattern of multiple sel obj    */
/* ObjMultiDelete  : Deletes the 'multi' selected objects from the chain.    */
/* ObjMultiDrawOutline : Draws the outlines of all objects.                  */
/* ObjDrawOutline      : Draws the outlines of a single object               */
/* ObjSetMltOutLineClr: Multiple Object outline color change.                */
/* ObjQuerySelect  : Are there objects selected?                             */
/* ObjMove    : move the selected object over a given amount.                */
/* ObjMoveOutLine : Shows the dragging object during WM_MOUSEMOVE.           */
/* ObjGroup   : Groups objects together ... calls directly GroupCreate       */
/* ObjUnGroup : DeGroups a given group.                                      */
/* ObjGetSize : Returns the size of a given class.                           */
/* ObjShiftSelect : Selection by WM_BUTTON1DOWN + ShiftKey......             */
/* ObjBoundingRect: Gives the bounding rectangle of all selected objs.       */
/* ObjAlignBottom: Align selected objects using their bottom outline.        */
/* ObjShowMovePoints: Show handles on the linejoins of objects.              */
/* ObjDragHandle : HeHe last function for changing the shape of an obj..     */
/* ObjInvArea    : Calculates the invarea in pixels on the screen .          */
/* ObjSetInvArea : Sets the invarea for objects which use DCTL_BOUNDARY      */
/* ObjRefresh    : Uses objinvarea to do a wininvalidaterect.                */
/* ObjLnJoinChange: Set the linejoin type for the given object.              */
/* ObjDrawRotLine: Shows the rotation line plus the object is rotated here   */
/* ObjectsAvail  : Are there objects to save??? Used for the filemenu.       */
/* ObjMoveRelCenter: Moves the rotation center relative                      */
/* ObjRemFromGroup: (public) removes object from group.                      */
/* ObjSetRounding:  (public) Sets the rounding of the selected box           */
/* ObjSetMltRound:  (public) Sets the rounding of multiple boxes.            */
/* ObjDropFillClr:  (public) Changes color after color drop event.           */
/* ObjMultiLineEndChange: (public) changes the lineends.                     */
/* ObjMakeImgClipPath :   (public) Makes a clippath on an img. should be obj */
/* ObjChangeRopCode   :   (public) Sets the image rop_code.                  */
/* ObjGetFillType     :   (public) Get the filling pattern type of selected  */
/* ObjEditText        :   (public) Edit selected textobject (block also!)    */
/* ObjPreparePrinting :   (public) Prepare printing.                         */
/* ObjGetLineSpace    :   (public) Blocktext: get the line spacing.          */
/* ObjMoveHandle      :   (public) Move a selected handle in polygon/circle  */
/*---------------------------------------------------------------------------*/
/* 1    221298    JDK   GroupCopy did not function correct.                  */
/*---------------------------------------------------------------------------*/
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
#include "dlg_sqr.h"
#include "dlg_cir.h"
#include "dlg_txt.h"
#include "dlg_lin.h"
#include "dlg_img.h"
#include "drwtrian.h"  /* triangle stuff NOT USED! */
#include "drwsplin.h"  /* polylines & splines      */
#include "drwmeta.h"   /* Metafile...              */
#include "drwbtext.h"  /* Blocktext...             */
#include "drwutl.h"
#include "drwgrp.h"    /* Grouping of objects...   */
#include "drwcanv.h"   /* Saving of form info      */
#include "drwfount.h"
#include "dlg_clr.h"
#include "resource.h"
#define HANDLESIZE    10

/*
** All bits and pieces for  rotation.
*/

#define PI 3.1415926
#define MINDEG 1           /* Minimum snap on degrees value */
#define MAXDEG 45          /* Maximum snap on degrees value */
static LONG  lSnapDeg = 1;
static HWND  hSnapRot;
static BOOL  bDialog = FALSE;

/*EOF ROTATE */

static ULONG ObjectSize[20];  /* Relates class ID to class size*/

static POBJECT pBaseObj,pObjEnd;
/*
** BASECLASS DEFINITION!!
*/
typedef struct _baseobj
{
   OBJECT   obj;
   USHORT   ustype;      /* type of object          */
   BATTR    bt;          /* Base attributes.        */
} BASEOBJ, *PBASEOBJ;


static void errMoveProc(POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   return; /* Used for the CLS_GROUPEND .... */
}
static void errPaintProc(HPS hps,WINDOWINFO *pwi,POBJECT pBegin,RECTL *prcl){}

static void errGetInvalidationArea(POBJECT pObject,RECTL *prcl,WINDOWINFO *pwi, BOOL bInc)
{
   /* Just do nothing - default processing..*/
}
/*------------------------------------------------------------------------*/
/*  ObjGetSize.                                                           */
/*                                                                        */
/*  Description : Returns the size of a given class.                      */
/*                                                                        */
/*------------------------------------------------------------------------*/
LONG ObjGetSize(USHORT usClass)
{
   return ObjectSize[usClass];
}
/*------------------------------------------------------------------------*/
/* Name : RotateSegment.                                                  */
/*                                                                        */
/* Description : Rotates a square around a givent center point.           */
/*                                                                        */
/* USHORT Deg : Number of degrees to rotate.                              */
/* POINTL ptlCenter : Center to rotate around.                            */
/* *PPOINT ptl : a pointer array of pointl structutes containing the      */
/* points to rotate.                                                      */
/* USHORT nr: Number of elements in the array of points.                  */
/*------------------------------------------------------------------------*/
void RotateSegment(USHORT Deg, POINTL ptlCenter, POINTL *ptls, USHORT points )
{
   USHORT i;
   POINTL tmpptl[25];  /* pointer to a pointer of pointl structs */

   /* some checks */
   if (!ptls || !Deg)
      return;

   if (Deg > 359)
      Deg = Deg % 360;

  for ( i = 0; i < points; i++)
  {
  tmpptl[i].x = (ptls[i].x - ptlCenter.x)*COSTAB[Deg]-
                (ptls[i].y - ptlCenter.y)*SINTAB[Deg]+ ptlCenter.x;
  tmpptl[i].y = (ptls[i].x - ptlCenter.x)*SINTAB[Deg]+
                (ptls[i].y - ptlCenter.y)*COSTAB[Deg]+ ptlCenter.y;
  }
   for ( i = 0; i < points; i++)
   {
       ptls[i].x = tmpptl[i].x;
       ptls[i].y = tmpptl[i].y;
   }
   return;
}
/*------------------------------------------------------------------------*/
/*  Name: ObjPtrAboveCenter                                               */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ObjPtrAboveCenter(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl)
{
   BOOL       bret = FALSE;
   PBASEOBJ   pBaseType;
   RECTL rcl;

   if (!pObj) return FALSE;

   switch(pObj->usClass)
   {
      case CLS_GROUP:
         bret = GrpPtrAboveCenter(pwi,pObj,ptl);
         break;
      default:
         pBaseType    = (PBASEOBJ)pObj;
         rcl.xLeft    = (LONG)(pBaseType->bt.ptlfCenter.x * pwi->usFormWidth);
         rcl.xLeft   -= HANDLESIZE;
         rcl.xRight   = rcl.xLeft + (2 * HANDLESIZE );
         rcl.yBottom  = (LONG)(pBaseType->bt.ptlfCenter.y * pwi->usFormHeight);
         rcl.yBottom -= HANDLESIZE;
         rcl.yTop     = rcl.yBottom + (2 * HANDLESIZE);
         return WinPtInRect((HAB)0,&rcl,&ptl);
   }
   return bret;
}
/*------------------------------------------------------------------------*/
/*  Name: ObjIsOnCorner                                                   */
/*                                                                        */
/*  Description : Tels wether the mouspointer resides on one of the       */
/*                corners of the selected text object.                    */
/*                                                                        */
/*  Concepts : Sets the right pointer. There are two different ponters,   */
/*             SPTR_SIZENESW = Upward sloping double-headed arrow,        */
/*             SPTR_SIZENWSE = Downward sloping double-headed arrow.      */
/*                                                                        */
/*  API's : WinSetPointer                                                 */
/*          WinQuerySysPointer                                            */
/*                                                                        */
/*  Parameters : POINTL ptl (mouse position)                              */
/*               RECTL  rcl square..                                      */
/*                                                                        */
/*  Returns:  TRUE : if the pointer is on a corner.                       */
/*            FALSE: if the pointer is not on a corner.                   */
/*            USHORT: Corner number.                                      */
/*                                                                        */
/*           1ÚÄÄÄÄÄÄÄ¿2                                                  */
/*            ³   6   ³5 = rotation handle                                */
/*           4ÀÄÄÄÄÄÄÄÙ3                                                  */
/*------------------------------------------------------------------------*/
BOOL ObjIsOnCorner(WINDOWINFO *pwi,POBJECT pObj,POINTL ptl,RECTL *rcl, USHORT *cnr)
{

   /*
   ** right bottom
   */
   if (ptl.x >= (rcl->xRight  - HANDLESIZE) &&
       ptl.x <= (rcl->xRight  + HANDLESIZE) &&
       ptl.y >= (rcl->yBottom - HANDLESIZE) &&
       ptl.y <= (rcl->yBottom + HANDLESIZE))
   {
      *cnr = 3;
      WinSetPointer(HWND_DESKTOP,
                    WinQuerySysPointer(HWND_DESKTOP,
                                       SPTR_SIZENWSE,FALSE));
      return TRUE;
   }

   /*
   ** left bottom
   */
   if (ptl.x >= (rcl->xLeft   - HANDLESIZE) &&
       ptl.x <= (rcl->xLeft   + HANDLESIZE) &&
       ptl.y >= (rcl->yBottom - HANDLESIZE) &&
       ptl.y <= (rcl->yBottom + HANDLESIZE))
   {
      *cnr = 4;
      WinSetPointer(HWND_DESKTOP,
                    WinQuerySysPointer(HWND_DESKTOP,
                                       SPTR_SIZENESW,FALSE));
      return TRUE;
   }

   if (ptl.x >= (rcl->xLeft   - HANDLESIZE) &&
       ptl.x <= (rcl->xLeft   + HANDLESIZE) &&
       ptl.y >= (rcl->yTop    - HANDLESIZE) &&
       ptl.y <= (rcl->yTop    + HANDLESIZE))
   {
      *cnr = 1;
      WinSetPointer(HWND_DESKTOP,
                    WinQuerySysPointer(HWND_DESKTOP,
                                       SPTR_SIZENWSE,FALSE));
      return TRUE;
   }

   if (ptl.x >= (rcl->xRight  - HANDLESIZE) &&
       ptl.x <= (rcl->xRight  + HANDLESIZE) &&
       ptl.y >= (rcl->yTop    - HANDLESIZE) &&
       ptl.y <= (rcl->yTop    + HANDLESIZE))
   {
      *cnr = 2;
      WinSetPointer(HWND_DESKTOP,
                    WinQuerySysPointer(HWND_DESKTOP,
                                       SPTR_SIZENESW,FALSE));
      return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/* Are there objects to save? Used for the filemenu. See drwmenu.c        */
/*------------------------------------------------------------------------*/
BOOL ObjectsAvail(void)
{
   return (BOOL)(pBaseObj !=NULL);
}
/*------------------------------------------------------------------------*/
void ObjOutLine(POBJECT pObject, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc)
{
   pGroup     pGrp;

   if (!pObject)
      return;

   switch (pObject->usClass)
   {
      case  CLS_TXT:
         TextOutLine(pObject,rcl,pwi,bCalc);
         break;
      case CLS_CIR:
         CircleOutLine(pObject,rcl,pwi,bCalc);
         break;
      case CLS_LIN:
         LineOutline(pObject,rcl,pwi);
         break;
      case CLS_SPLINE:
         SplineOutLine(pObject,rcl,pwi,bCalc);
         break;
      case CLS_IMG:
         ImgOutLine(pObject,rcl,pwi);
         break;
      case CLS_GROUP:
         pGrp = (pGroup)pObject;
         GroupOutLine(pGrp,rcl,pwi);
         break;
      case CLS_META:
         MetaPictOutLine(pObject,rcl,pwi);
         break;
      case CLS_BLOCKTEXT:
         BlockTextOutLine(pObject,rcl,pwi);
         break;
    }
}
/*------------------------------------------------------------------------*/
/*  ObjShowMovePoints                                                     */
/*                                                                        */
/*  Description : Shows the handles of an object which can be use to      */
/*                change its normal shape.                                */
/*                                                                        */
/*                                                                        */
/*                                                                        */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ObjShowMovePoints(POBJECT pObj, WINDOWINFO *pwi)
{
   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         SplinShowHandles(pObj,pwi);
         return TRUE;
      case CLS_CIR:
         CirShowHandles(pObj,pwi);
         return TRUE;
      default:
         return FALSE;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjDragHandle                                                         */
/*                                                                        */
/*  Description : Drags one handle of an object.  When the WM_BUTTON1DOWN */
/*                founds a hit in one of the handles which are on the line*/
/*                joins, the window procedure will go directly into the   */
/*                selected object with the same mouse position.           */
/*                                                                        */
/*  Parameters  : POBJECT - pointer to a spline object.                   */
/*                WINDOWINFO * - pointer to our windowinfo struct.        */
/*                POINTL  ptlMouse - mouse position.                      */
/*                ULONG   ulMode contains the window message:             */
/*                        WM_BUTTON1DOWN initialize...                    */
/*                        WM_MOUSEMOVE   Show changes while dragging.     */
/*                        WM_BUTTON1DOWN Store new position in object     */
/*                                                                        */
/*                MPARAM  mp2 - Second message parameter to get the       */
/*                        the shift,control or alt key.                   */
/*  Return : BOOL TRUE on succes. Action should be canceled in windowproc */
/*           if retvalue is FALSE.                                        */
/*------------------------------------------------------------------------*/
BOOL ObjDragHandle(POBJECT pObj, WINDOWINFO *pwi, POINTL ptlMouse, ULONG ulMode, MPARAM mp2)
{
   if (!pObj)
      return FALSE;

   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         return  SplinDragHandle(pObj,pwi,ptlMouse,ulMode,mp2);
      case CLS_CIR:
         return  CirDragHandle(pObj,pwi,ptlMouse,ulMode,mp2);
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/* name : ObjMoveHandle                                                   */
/*                                                                        */
/* Description : Moves the selected handle in a polygon or circle.        */
/*               Called from drwmain.c whenever the program is in the     */
/*               OBJFORMCHANGE mode. The object zelf will move its        */
/*               selected point with dx,dy.  Depending on which cursor    */
/*               key is pressed.....                                      */
/*                                                                        */
/* returns : BOOL - always true...                                        */
/*------------------------------------------------------------------------*/
BOOL ObjMoveHandle(POBJECT pObj, WINDOWINFO *pwi, long ldx,long ldy)
{
   RECTL rcl;
   BOOL  bDone = FALSE;

   if (!pObj)
      return FALSE;


   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         bDone = splineMoveHandle(pObj,pwi,ldx,ldy);
         break;
   }


   if ( bDone )
   {
      ObjInvArea(pObj,&rcl,pwi,TRUE);
      rcl.xLeft  -= HANDLESIZE * 2;
      rcl.xRight += HANDLESIZE * 2;
      rcl.yTop   += HANDLESIZE * 2;
      rcl.yBottom-= HANDLESIZE * 2;
      WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Old rect .. */
   }
   return TRUE;   
}
/*------------------------------------------------------------------------*/
/*  ObjDelHandle                                                          */
/*                                                                        */
/*  Description : Deletes the selected handle of an object. In this case  */
/*                only for polylines.                                     */
/*                                                                        */
/*  Parameters  : POBJECT - pointer to a spline object.                   */
/*                                                                        */
/*  Return : BOOL TRUE on succes.                                         */
/*------------------------------------------------------------------------*/
BOOL ObjDelHandle(POBJECT pObj)
{
   if (!pObj)
      return FALSE;

   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         return  SplinDelHandle(pObj);
   }
   return FALSE;
}

/*------------------------------------------------------------------------*/
void DrawMovePointHandle(HPS hps, LONG x, LONG y)
{
   POINTL ptl;

   ptl.x = x - HANDLESIZE;
   ptl.y = y + HANDLESIZE;
   GpiMove(hps,&ptl);
   ptl.x = x + HANDLESIZE;
   ptl.y = y - HANDLESIZE;
   GpiBox(hps,DRO_OUTLINE,&ptl,0L,0L);
}
/*------------------------------------------------------------------------*/
void DrawHandle(HPS hps, LONG x, LONG y)
{
   POINTL ptl;

   ptl.x = x - HANDLESIZE;
   ptl.y = y + HANDLESIZE;
   GpiMove(hps,&ptl);
   ptl.x = x + HANDLESIZE;
   ptl.y = y - HANDLESIZE;
   GpiBox(hps,DRO_FILL,&ptl,0L,0L);
}
/*------------------------------------------------------------------------*/
/* ObjDrawRotCenter. Draw the rotation center of the given object.        */
/* and yes the selection handle to do the rotation.                       */
/*                                                                        */
/* Parameters   : BOOL bInvert - is false when called after an ordinary   */
/*                               paint. But true when dragging the center */
/*------------------------------------------------------------------------*/
BOOL ObjDrawRotCenter(WINDOWINFO *pwi,POBJECT pObj, BOOL bInvert)
{
   BOOL bRet = FALSE;
   POINTL ptlCenter;
   PBASEOBJ pBs;
   RECTL rcl;
   ARCPARAMS arcp = { HANDLESIZE,HANDLESIZE,0,0};

   if (!pObj) return FALSE;

   switch(pObj->usClass)
   {
      case CLS_GROUP:
         bRet = GrpGetCenter(pwi,pObj,&ptlCenter);
         break;
      default:
         pBs  = (PBASEOBJ)pObj;
         bRet = TRUE;
         ptlCenter.x = (LONG)(pBs->bt.ptlfCenter.x * pwi->usFormWidth);
         ptlCenter.y = (LONG)(pBs->bt.ptlfCenter.y * pwi->usFormHeight);
         break;
   }

   if (bRet)
   {
      LONG oldPattern;
      oldPattern = GpiQueryPattern(pwi->hps);
      GpiSetPattern(pwi->hps,PATSYM_SOLID);
      if (bInvert)
         GpiSetMix(pwi->hps,FM_INVERT);
      ObjOutLine(pObj,&rcl,pwi,FALSE);
      GpiSetCurrentPosition(pwi->hps,&ptlCenter);
      GpiSetArcParams(pwi->hps,&arcp);
      GpiFullArc(pwi->hps,DRO_OUTLINEFILL,MAKEFIXED(1,0));
      GpiFullArc(pwi->hps,DRO_OUTLINE,MAKEFIXED(2,0));
      rcl.yBottom += (rcl.yTop - rcl.yBottom)/2;
      DrawHandle(pwi->hps,rcl.xRight,rcl.yBottom);
      GpiSetPattern(pwi->hps,oldPattern);
   }
   return bRet;
}
/*------------------------------------------------------------------------*/
/* showSelectHandles: Draws a box arround a selected textobject              */
/*------------------------------------------------------------------------*/
VOID showSelectHandles (POBJECT pObject,WINDOWINFO *pwi)
{
   RECTL  rcl;

   LONG   oldPattern;
   pLines pLin;         /* For our exceptiom */

   if (!pObject)
      return;

   ObjOutLine(pObject,&rcl,pwi,FALSE);

   /*
   ** Since a single does not need a box arrond itself we must make one
   ** exception, e.g test!
   */

   oldPattern = GpiQueryPattern(pwi->hps);
   GpiSetPattern(pwi->hps,PATSYM_SOLID);
   GpiSetMix(pwi->hps,FM_INVERT);

      /* the four courners */
   if (pObject->usClass != CLS_LIN)
   {
      DrawHandle(pwi->hps,rcl.xLeft,rcl.yTop);
      DrawHandle(pwi->hps,rcl.xLeft,rcl.yBottom);
      DrawHandle(pwi->hps,rcl.xRight,rcl.yBottom);
      DrawHandle(pwi->hps,rcl.xRight,rcl.yTop);
   }
   else
   {
      /*
      ** Yes we are a line!!
      */
      pLin = (pLines)pObject;
      DrawHandle(pwi->hps,(pLin->ptl1.x * pwi->usFormWidth ),
                          (pLin->ptl1.y * pwi->usFormHeight));

      DrawHandle(pwi->hps,(pLin->ptl2.x * pwi->usFormWidth),
                          (pLin->ptl2.y * pwi->usFormHeight));
   }
   GpiSetPattern(pwi->hps,oldPattern);
}
/*------------------------------------------------------------------------*/
/* ObjMultiPaintHandles - called during the paint while op_mode is        */
/*                        MULTISELECT.                                    */
/*------------------------------------------------------------------------*/
void ObjMultiPaintHandles(WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)
      return;

   pBegin = pBaseObj;

   do
   {
      if (pBegin->bMultiSel)
         showSelectHandles (pBegin,pwi);
      pBegin = pBegin->Next;
   } while (pBegin);
}
/*------------------------------------------------------------------------*/
POBJECT ObjGroup(WINDOWINFO *pwi)
{
   char  szBuf[80];
   POBJECT pBegin;
   POBJECT pGrp = NULL;

   if (pBaseObj == NULL)  /*Initial true */
      return NULL;

   pBegin = pBaseObj;

   do
   {
      if (pBegin->bMultiSel &&
          (pBegin->usClass == CLS_IMG || pBegin->usClass == CLS_META))
      {
         WinLoadString(hab, (HMODULE)0,IDS_IMAGENOTINGROUP,
                       sizeof(szBuf),szBuf);

         WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
		       (PSZ)szBuf,
                       (PSZ)"Error",0,
                       MB_OK |
                       MB_APPLMODAL |
                       MB_MOVEABLE |
                       MB_ICONEXCLAMATION);
         return NULL;
      }
      pBegin = pBegin->Next;
   }while (pBegin);

   if ((pGrp = GroupCreate(pBaseObj,pwi)) == NULL)
      return NULL;

   /*
   ** Update end pointer... point again to end of chain.
   */
   pObjEnd = pBaseObj;
   while (pObjEnd->Next) pObjEnd = pObjEnd->Next;

   if (pGrp)
   {
      ObjRefresh(pGrp,pwi);
      return (POBJECT)pGrp;
   }
   return NULL;
}
/*--------------------------------------------------[ public ]------------*/
/*  ObjRemFromGroup.                                                      */
/*                                                                        */
/*  Description : Removes the given object from the group.                */
/*                                                                        */
/*  Parameters  : POBJECT pObj - pointer to the object to be removed.     */
/*                PRECTL       - pointer to the group rectangle.          */
/*                WINDOWINFO * - pointer to the WINDOWINFO structure.     */
/*                                                                        */
/*                                                                        */
/*  Called      : Whenever a user ungroups a group....                    */
/*                and when a group is rotated this function is called.    */
/*                see drwgrp.c and this file.                             */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
POBJECT ObjRemFromGroup(POBJECT pObj, PRECTL prcl, WINDOWINFO *pwi)
{
   POBJECT pEnd = NULL;

   switch (pObj->usClass)
   {
      case CLS_TXT:
         TxtRemFromGroup(pObj,prcl,pwi);
         break;
      case CLS_LIN:
         LinRemFromGroup(pObj,prcl,pwi);
         break;
      case CLS_CIR:
         CirRemFromGroup(pObj,prcl,pwi);
         break;
      case CLS_SPLINE:
         SplRemFromGroup(pObj,prcl,pwi);
         break;
      case CLS_GROUPEND:
         pEnd = pObj;
         break;
      default:
         break;
   }
   return pEnd;
}
/*------------------------------------------------------------------------*/
/*  ObjUnGroup.                                                           */
/*                                                                        */
/*  Description : UnGroups a given group.                                 */
/*                Called from drawit.c (main module) directly from menu   */
/*                action UNGROUP.                                         */
/*                                                                        */
/*  Parameters  : POBJECT pObj - pointer to the selected CLS_GROUP.       */
/*                                                                        */
/*                POBJECT pBase- pointer to the beginning of the main     */
/*                chain. We need this when the CLS_GROUP has no previous  */
/*                pointer. Than the pBase must be updated to point to     */
/*                the first child in the group.                           */
/*                                                                        */
/*                POBJECT pEnd   pointer to the last element of the main  */
/*                chain. Only updated when the group is the last element  */
/*                in the main chain.                                      */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void ObjUnGroup(POBJECT pObj,WINDOWINFO *pwi)
{
   POBJECT pNext,pPrevious,pChild,pEnd,p;
   RECTL   rcl;

   if (pBaseObj == NULL)   /* Initial true */
      return;

   if (!pObj && pObj->usClass != CLS_GROUP)
      return;

   ObjOutLine(pObj,&rcl,pwi,FALSE);

   pNext     = pObj->Next;
   pPrevious = pObj->Previous;
   pChild    = pObj->NextChild;
   /*
   ** Forget about the group parent..
   */
   pChild->PrevChild = NULL;

   if (pPrevious)
   {
      pPrevious->Next  = pChild;
      pChild->Previous = pPrevious;
   }
   else
   {
      /*
      ** We seem to be the first element in the main chain
      ** So update the pBase pointer.
      */
      pBaseObj = pChild;
   }

   pEnd = NULL;
   p    = NULL;
   /*
   ** Tell the kids that daddy is going away...
   */
   while (pChild)
   {

      p = ObjRemFromGroup(pChild,&rcl,pwi);

      if (p && !pEnd)
         pEnd = p;

      pChild = pChild->Next;
   }

   pChild = pEnd->Previous;
   /*
   ** Now delete our groupend marker.
   */
   free(pEnd); /* remove the pend, CLS_ENDGROUP */
   pChild->Next = NULL; /* remove CLS_ENDGROUP  */

   if (pNext)
   {
      pChild->Next    = pNext;
      pNext->Previous = pChild;
   }
   else
   {
      /*
      ** We are the last element in the mainchain
      ** So update our chain end pointer.
      */
      pObjEnd = pChild;
   }
   /*
   ** Say goodbye to our group marker.
   */
   free(pObj);
   pObj = NULL;
   return;
}
/*------------------------------------------------------------------------*/
/*  ObjMoveOutLine.                                                       */
/*                                                                        */
/*  Description : Called during WM_MOUSEMOVE messages. Shows the ouline   */
/*                of an selected object which is dragged by the mouse.    */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjMoveOutLine(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy)
{
   if (!pObj)
      return;
   pObj->moveOutline(pObj,pwi,dx,dy);
}
/*-------------------------------------------------------------------------*/
void ObjMoveMultiOutline(WINDOWINFO *pwi,SHORT dx, SHORT dy)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         pBegin->moveOutline(pBegin,pwi,dx,dy);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/* ObjMove...                                                             */
/*------------------------------------------------------------------------*/
void ObjMove(POBJECT pObj,SHORT dx,SHORT dy, WINDOWINFO *pwi)
{
   if (!pObj)
      return;

   if (!dx && !dy)
      return;

   pObj->rclOutline.xLeft  += (LONG)dx;
   pObj->rclOutline.xRight += (LONG)dx;
   pObj->rclOutline.yBottom+= (LONG)dy;
   pObj->rclOutline.yTop   += (LONG)dy;

   pwi->bFileHasChanged = TRUE;

   switch (pObj->usClass)
   {
      case  CLS_TXT:
         MoveTextSegment(pObj,dx,dy,pwi);
         break;
      case CLS_CIR:
         MoveCirSegment(pObj,dx,dy,pwi);
         break;
      case CLS_LIN:
         MoveLinSegment(pObj,dx,dy,pwi);
         break;
      case CLS_SPLINE:
         SplMoveSegment(pObj,dx,dy,pwi);
         break;
      case CLS_IMG:
         ImgMoveSegment(pObj,dx,dy,pwi);
         break;
      case CLS_GROUP:
         GroupMove(pObj,dx,dy,pwi);
         break;
      case CLS_META:
         MetaPictMove(pObj,dx,dy,pwi);
         break;
      case CLS_BLOCKTEXT:
         MoveBlockText(pObj,dx,dy,pwi);
         break;
   }
   pObj->bMultiSel=FALSE;

   return;
}
/*------------------------------------------------------------------------*/
/* MoveObject... after a cursor key, and force a redraw.                  */
/*------------------------------------------------------------------------*/
void MoveObject(POBJECT pObj, LONG dx, LONG dy, WINDOWINFO *pwi)
{
   RECTL      rcl;
   POINTL     ptl;
 if (!pObj)
    return;

  ObjInvArea(pObj,&rcl,pwi,TRUE);
  ObjMove(pObj,(SHORT)dx,(SHORT)dy,pwi);

  rcl.xLeft  -= HANDLESIZE * 2;
  rcl.xRight += HANDLESIZE * 2;
  rcl.yTop   += HANDLESIZE * 2;
  rcl.yBottom-= HANDLESIZE * 2;
  WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Old rect .. */

  ptl.x = dx;
  ptl.y = dy;
  GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptl);
  rcl.xLeft  += ptl.x;
  rcl.xRight += ptl.x;
  rcl.yTop   += ptl.y;
  rcl.yBottom+= ptl.y;
  WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Repaint new */
  return;
}
/*------------------------------------------------------------------------*/
/* Register in our objectsize array the sizes of the different classes.   */
/*------------------------------------------------------------------------*/
void ObjNotify()
{
    ObjectSize[CLS_SQR]       = sizeof(Square);
    ObjectSize[CLS_CIR]       = sizeof(Circle);
    ObjectSize[CLS_TXT]       = sizeof(Text);
    ObjectSize[CLS_IMG]       = sizeof(Image);
    ObjectSize[CLS_LIN]       = sizeof(Lines);
    ObjectSize[CLS_META]      = sizeof(Metaimg);
    ObjectSize[CLS_SPLINE]    = sizeof(Spline);
    ObjectSize[CLS_GROUP]     = sizeof(Group);
    ObjectSize[CLS_GROUPEND]  = sizeof(GroupEnd);
    ObjectSize[CLS_BLOCKTEXT] = sizeof(blocktext); 
}
/*------------------------------------------------------------------------*/
/* Appends a single object to the existing list.                          */
/*------------------------------------------------------------------------*/
BOOL pObjAppend(POBJECT pObj)
{
  POBJECT pTmpObj;

  if (!pObj)
     return FALSE;

  pObj->Next     = NULL;
  pObj->Previous = NULL;

  if (pBaseObj == NULL)		                  /*Initial true*/
  {
      pBaseObj          = pObj;
      pObjEnd           = pBaseObj;              /*Start of list */
      pObjEnd->Next     = NULL;
      pObjEnd->Previous = NULL;
   }
   else if (pObjEnd->Next==NULL)
   {
      /*
      ** Append an element to the end of the list.
      */
      pObjEnd->Next = pObj;
      pTmpObj = pObjEnd;
      pObjEnd = pObjEnd->Next;
      pObjEnd->Previous = pTmpObj;
      pObjEnd->Next     = NULL;
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* Appends a chain of objects to the existing list.                       */
/*------------------------------------------------------------------------*/
BOOL AppendChain(POBJECT pObj)
{
  POBJECT pTmpObj;

  if (!pObj)
     return FALSE;

  if (pBaseObj == NULL)		                  /*Initial true*/
  {
      pBaseObj          = pObj;
      pObjEnd           = pBaseObj;              /*Start of list */
      /*
      ** Update end pointer... point again to end of chain.
      */
      while (pObjEnd->Next) pObjEnd = pObjEnd->Next;
   }
   else
   {
      /*
      ** Append an element to the end of the list.
      */
      pObjEnd->Next = pObj;
      pTmpObj = pObjEnd;
      pObjEnd = pObjEnd->Next;
      pObjEnd->Previous = pTmpObj;
      /*
      ** Update end pointer... point again to end of chain.
      */
      while (pObjEnd->Next) pObjEnd = pObjEnd->Next;
   }
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/* setBaseAttributes.                                                     */
/*------------------------------------------------------------------------*/
static void setBaseAttributes(PBASEOBJ pBs, WINDOWINFO *pwi)
{
   pBs->bt.line.LineColor = pwi->ulOutLineColor;
   pBs->bt.line.LineType  = pwi->lLntype;
   pBs->bt.line.LineWidth = pwi->lLnWidth;
   pBs->bt.line.LineJoin  = pwi->lLnJoin;
   pBs->bt.line.LineEnd   = pwi->lLnEnd;
   /*
   ** Get the gradient stuff...
   */
   pBs->bt.gradient.ulStart = pwi->Gradient.ulStart;
   pBs->bt.gradient.ulSweep = pwi->Gradient.ulSweep;
   pBs->bt.gradient.ulSaturation = pwi->Gradient.ulSaturation;
   pBs->bt.gradient.ulDirection  = pwi->Gradient.ulDirection;
   /*
   ** Fountain fill stuff....
   */
   pBs->bt.fountain.ulStartColor = pwi->fountain.ulStartColor;
   pBs->bt.fountain.ulEndColor   = pwi->fountain.ulEndColor;
   pBs->bt.fountain.lHorzOffset  = pwi->fountain.lHorzOffset;
   pBs->bt.fountain.lVertOffset  = pwi->fountain.lVertOffset;
   pBs->bt.fountain.ulFountType  = pwi->fountain.ulFountType;
   pBs->bt.fountain.lAngle       = pwi->fountain.lAngle;

   pBs->bt.lPattern   = pwi->ColorPattern;  /* filling pattern */
   pBs->bt.usLayer    = pwi->uslayer;       /* Drawing layer.  */
   pBs->bt.fColor     = pwi->ulColor;       /* Filling color   */
   pBs->bt.ShadeColor = pwi->ulColor;       /* Shading color   */

   pBs->bt.arrow.lEnd    = pwi->arrow.lEnd;
   pBs->bt.arrow.lStart  = pwi->arrow.lStart;
   pBs->bt.arrow.lSize   = pwi->arrow.lSize;
   /*
   ** Shading
   */
   pBs->bt.Shade.lShadeType  = pwi->Shade.lShadeType;
   pBs->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
   pBs->bt.Shade.lUnits      = pwi->Shade.lUnits;
   return;
}
/*------------------------------------------------------------------------*/
/* Creates a single standalone object.                                    */
/*------------------------------------------------------------------------*/
POBJECT pObjNew(WINDOWINFO *pwi, USHORT usClass)
{
  POBJECT  pObj;

  if ((pObj = (POBJECT)calloc(ObjectSize[usClass],1))==NULL)
     ErrorBox("Error","Not enough memory");
  else
  {
     pObj->Next     = NULL;
     pObj->Previous = NULL;
     pObj->usClass  = usClass;
  }

  if (pwi)
  {
     switch (usClass)
     {  
        case CLS_IMG:
           break;
        case CLS_META:
           break;
        case CLS_GROUP:
           break;
		case CLS_GROUPEND:
           break;
        default:
           setBaseAttributes((PBASEOBJ)pObj,pwi);
           break;
     }
  }
  pObj->moveOutline = errMoveProc;
  pObj->paint       = errPaintProc;
  pObj->getInvalidationArea = errGetInvalidationArea;
  return pObj;
}
/*------------------------------------------------------------------------*/
POBJECT pObjCreate(WINDOWINFO *pwi , USHORT usClass)
{
  POBJECT pTmpObj;

  if (pBaseObj == NULL)		                  /*Initial true*/
   {
      if ((pBaseObj =(POBJECT)calloc(ObjectSize[usClass],1))==NULL)
	 ErrorBox("Error","Not enough memory");

      pObjEnd           = pBaseObj;              /*Start of list */
      pObjEnd->Next     = NULL;
      pObjEnd->Previous = NULL;
      pObjEnd->usClass  = usClass;
   }
   else if (pObjEnd->Next==NULL)
   {
      /*
      ** Add an element to the end of the list.
      */

      if ((pObjEnd->Next = (POBJECT)calloc(ObjectSize[usClass],1))==NULL)
	 ErrorBox("Error","Not enough memory");
      pTmpObj = pObjEnd;
      pObjEnd = pObjEnd->Next;
      pObjEnd->Previous = pTmpObj;
      pObjEnd->Next     = NULL;
      pObjEnd->usClass  = usClass;
   }
   /*
   ** Setup default routines which will be overriden by
   ** the individual implementations.
   */
   pObjEnd->moveOutline = errMoveProc; /* local empty routine */
   pObjEnd->paint       = errPaintProc;
   pObjEnd->getInvalidationArea = errGetInvalidationArea;

   if (pwi)
   {
      switch (usClass)
      {
         case CLS_IMG:
            break;
         case CLS_META:
            break;
         case CLS_GROUP:
            break;
         default:
            setBaseAttributes((PBASEOBJ)pObjEnd,pwi);
            break;
      }
   }
   return pObjEnd;
}
/*------------------------------------------------------------------------*/
void ObjDrawSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   if (!pObj)
      pBegin = pBaseObj;  /* Point to the starting point of the chain */
   else
      pBegin = pObj;

   while (pBegin!= NULL)
   {
      pBegin->paint(hps,pwi,pBegin,prcl);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
void ObjMultiDrawOutline(WINDOWINFO *pwi,POBJECT pObj, BOOL bIgnoreLayer)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   if (!pObj)
      pBegin = pBaseObj;  /* Point to the starting point of the chain */
   else
      pBegin = pObj;

   while (pBegin!= NULL)
   {
      switch (pBegin->usClass)
      {
         case CLS_CIR:
            cirDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
         case CLS_LIN:
            linDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
         case CLS_IMG:
            DrawImgSegment(pwi->hps,pwi,pBegin,(RECTL *)0);
            break;
         case CLS_TXT:
            txtDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
         case CLS_SPLINE:
            splDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
         case CLS_META:
            DrawMetaSegment(pwi->hps,pwi,pBegin,(RECTL *)0);
            break;
         case CLS_GROUP:
            grpDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
         case CLS_BLOCKTEXT:
            BlockTextDrawOutline(pBegin,pwi,bIgnoreLayer);
            break;
      }
      pBegin =  pBegin->Next;
   }

   return;
}
/*------------------------------------------------------------------------*/
/* Get currentObject                                                      */
/*------------------------------------------------------------------------*/
POBJECT ObjCurrent(void)
{
   return (POBJECT)pObjEnd;
}
/*------------------------------------------------------------------------*/
/* Extract the given object from the chain, used by grouping! (drwgrp.c)  */
/* and deleting objects...                                                */
/*------------------------------------------------------------------------*/
void ObjExtract(POBJECT pObj)
{
   POBJECT TmpPrev,TmpNext;

   if (!pObj)
      return;

   TmpNext = pObj->Next;
   TmpPrev = pObj->Previous;

   if (TmpPrev && TmpNext)
   {
      TmpPrev->Next     = TmpNext;
      TmpNext->Previous = TmpPrev;
   }
   else if (TmpPrev && !TmpNext)
   {
      /*
      ** End of chain so bring back the endof chain pointer.
      */
      pObjEnd = pObjEnd->Previous;
      TmpPrev->Next = NULL;
   }
   else if (!TmpPrev && TmpNext)
   {
      TmpNext->Previous = NULL;
      pBaseObj = TmpNext;
   }
   else if (!TmpPrev && !TmpNext)
   {
      pBaseObj = NULL;
   }
   return;
}
/*------------------------------------------------------------------------*/
/* Delete a Square segment.                                               */
/*------------------------------------------------------------------------*/
void ObjDelete(POBJECT pObj)
{
   pSpline pSpl;        /*delete on POLYLINES    */


   if (!pObj)
      return;

   ObjExtract(pObj);

   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         pSpl = (pSpline)pObj;
         free(pSpl->pptl);       /* Delete the allocated points.*/
         break;
      case CLS_IMG:
         free_imgobject(pObj);
         break;
      case CLS_GROUP:
        GroupDelete(pObj->NextChild);
        break;
      case CLS_BLOCKTEXT:
        free_BlockText(pObj);
        return;
   }
   free(pObj);

   return;
}
/*------------------------------------------------------------------------*/
void ObjDeleteAll()
{
   while (pBaseObj)
      ObjDelete(pBaseObj);
}
/*------------------------------------------------------------------------*/
/*  ObjSelect.                                                            */
/*                                                                        */
/*  Description : Function is directly called after a WM_BUTTON1DOWN      */
/*                                                                        */
/*  Parameters  : POINTL ptl mouse position.                              */
/*                WINDOWINFO - pointer to our windowinfo struct.          */
/*                                                                        */
/*  Returns     : POBJECT pointer to obj if success else NULL.            */
/*------------------------------------------------------------------------*/
POBJECT ObjSelect(POINTL ptl,WINDOWINFO *pwi)
{
   POBJECT pBegin;
   POBJECT pObjRet = NULL;

   if (pBaseObj == NULL)       /*Initial true*/
      return NULL;

   GpiSetDrawControl(pwi->hps, DCTL_CORRELATE, DCTL_ON);
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
   GpiSetPickAperturePosition(pwi->hps, &ptl);

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin->Next)          /* Go to the end... */
      pBegin = pBegin->Next;

   while (pBegin!= NULL)
   {
      switch (pBegin->usClass)
      {
         case CLS_CIR:
            pObjRet = (POBJECT)CircleSelect(ptl,pwi,pBegin);
            break;
         case CLS_LIN:
            pObjRet = (POBJECT)LineSelect(ptl,pwi,pBegin);
            break;
         case CLS_IMG:
            pObjRet = (POBJECT)ImageSelect(ptl,pwi,pBegin);
            break;
         case CLS_TXT:
            pObjRet = (POBJECT)TextSelect(ptl,pwi,pBegin);
            break;
         case CLS_GROUP:
            pObjRet = (POBJECT)GroupSelect(ptl,pwi,pBegin);
            break;
         case CLS_SPLINE:
            pObjRet = (POBJECT)SplineSelect(ptl,pwi,pBegin);
            break;
         case CLS_META:
            pObjRet = (POBJECT)MetaPictSelect(ptl,pwi,pBegin);
            break;
         case CLS_BLOCKTEXT:
            pObjRet = (POBJECT)BlockTextSelect(ptl,pwi,pBegin);
            break;
      }

      if (pObjRet)
      {
         GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
         GpiSetDrawControl(pwi->hps, DCTL_CORRELATE, DCTL_OFF);

         return pObjRet;
      }
      pBegin =  pBegin->Previous;
   }
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
   GpiSetDrawControl(pwi->hps, DCTL_CORRELATE, DCTL_OFF);

   return pObjRet;
}
/*---------------------------------------------------[ private ]----------*/
/*  Name: ObjGroupCopy                                                    */
/*                                                                        */
/*  Description : Private function for copying groups.                    */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void ObjGroupCopy(POBJECT pObj, POBJECT pChain, BOOL bDown)
{
   POBJECT pTmp;
   POBJECT pCopy;

   if (!bDown)
   {
   /*
   ** Create empty object with the size of the source object.
   */
   pTmp =(POBJECT)calloc(ObjectSize[pObj->usClass],1);
   /*
   ** copy source to target.
   */
   memcpy(pTmp,pObj,ObjectSize[pObj->usClass]);

   /*
   ** Link it in the main chain..
   */
   pChain->Next = pTmp;
   pTmp->Previous = pChain;
   pTmp->Next = NULL;
   /*
   ** Step to the last object in main chain.
   */
   pChain = pChain->Next;
   pObj  = pObj->NextChild;
   }
   /*
   ** Go one down in chain...
   */

   pTmp  =(POBJECT)calloc(ObjectSize[pObj->usClass],1);
   memcpy(pTmp,pObj,ObjectSize[pObj->usClass]);

   /*
   ** Hang the new obj under the chain end.
   */
   pChain->NextChild = pTmp;
   pTmp->Next     = NULL;
   pTmp->Previous = NULL;
   pTmp->PrevChild= pChain;
   /*
   ** Go down
   */
   pChain = pChain->NextChild;


   if (pObj->usClass == CLS_GROUP)
      bDown = TRUE;
   else
   {
      pObj  = pObj->Next;
      bDown = FALSE;
   }

   do
   {
      switch(pObj->usClass)
      {
         case CLS_GROUP:
            if (bDown)
            {
               printf ("Recurse with [%d] \n",pObj->usClass);
               ObjGroupCopy(pObj->NextChild,pChain,TRUE);
               printf("After recursion... copy [%d]\n",pObj->usClass);
               bDown = FALSE;

            }
            else
            {
               printf("Copy [%d] \n",pObj->usClass);
               pCopy =(POBJECT)calloc(ObjectSize[pObj->usClass],1);
               memcpy(pCopy,pObj,ObjectSize[pObj->usClass]);
               pChain->Next    = pCopy;
               pCopy->Previous = pChain;
               pChain = pChain->Next;
               printf ("Recurse with [%d] \n",pObj->NextChild->usClass);
               ObjGroupCopy(pObj->NextChild,pChain,TRUE);
               bDown = FALSE;
            }
            break;
         case CLS_SPLINE:
            pCopy = copySpline(pObj);
            pChain->Next    = pCopy;
            pCopy->Previous = pChain;
            pChain = pChain->Next;
            break;
         case CLS_BLOCKTEXT:
            DosBeep(440,50);
            DosBeep(880,50);
            break;
         default:
            printf("Just copy [%d]\n",pObj->usClass);
            pCopy =(POBJECT)calloc(ObjectSize[pObj->usClass],1);
            memcpy(pCopy,pObj,ObjectSize[pObj->usClass]);
            pChain->Next    = pCopy;
            pCopy->Previous = pChain;
            pChain = pChain->Next;
            break;
      }
      pObj = pObj->Next;
   }while(pObj);
   printf("Return...\n");
}
/*------------------------------------------------------------------------*/
/* makeCopy                                                               */
/*                                                                        */
/* Description : Only makes a copy of the given object. Does NOT add it   */
/*               to any chain!                                            */
/*                                                                        */
/* Returns     : Reference to the copied object.                          */
/*------------------------------------------------------------------------*/
POBJECT makeCopy(POBJECT pOrg)
{
   POBJECT pCopy = NULL;

   switch (pOrg->usClass)
   {
      case CLS_SPLINE:
         pCopy = copySpline(pOrg);
         break;
      case CLS_IMG:
         pCopy = copyImageObject(pOrg);
         break;
      case CLS_BLOCKTEXT:
         pCopy = copyBlockTextObject(pOrg);
         break;
      case CLS_META:
         pCopy = copyMetaFileObject(pOrg);
         break;
      case CLS_GROUP:
         pCopy = copyGroup(pOrg);
         break;
      default:
         pCopy =  pObjNew(NULL,pOrg->usClass);
         memcpy(pCopy,pOrg,ObjectSize[pOrg->usClass]);
         break;
   }

   if (pCopy)
   {
      pCopy->bMultiSel= FALSE; 
      pCopy->Next     = NULL;
      pCopy->Previous = NULL;
   }
   pOrg->bMultiSel = FALSE; 
   return pCopy;
}
/*------------------------------------------------------------------------*/
POBJECT ObjectCopy(POBJECT pObjOrg)
{
   POBJECT pCopy;

   pCopy =  makeCopy(pObjOrg);

   if (pCopy)
   {
      pObjAppend(pCopy);
      return pCopy;
   }
   return pObjOrg;
}
/*------------------------------------------------------------------------*/
/*                BOOL         - When true the obj area is including the  */
/*                               size boxes etc.                          */
/*                               FALSE the area is taken as small as poss */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjInvArea(POBJECT pObject,RECTL *prcl,WINDOWINFO *pwi, BOOL bInc)
{
   PBASEOBJ pBs;
   POINTL   ptl;

   pObject->getInvalidationArea(pObject,prcl,pwi, bInc);
   /*
   ** Add space for shading!
   */
   pBs = (PBASEOBJ)pObject;
   if (pBs->bt.Shade.lShadeType != SHADE_NONE)
   {
   ptl.x = 0;
   ptl.y = 0;

   setShadingOffset(pwi,pBs->bt.Shade.lShadeType, 
                    pBs->bt.Shade.lUnits,&ptl,1);
   prcl->xLeft  -= abs(ptl.x);
   prcl->yTop   += abs(ptl.y);
   prcl->xRight += abs(ptl.x);
   prcl->yBottom-= abs(ptl.y);
   }
}
/*------------------------------------------------------------------------*/
/*  ObjRefresh                                                            */
/*                                                                        */
/*  Description : Uses the objinvarea to invalidate the clientwindow in   */
/*                the windowinfo structure.                               */
/*------------------------------------------------------------------------*/
void ObjRefresh(POBJECT pObj, WINDOWINFO *pwi)
{
   RECTL rcl;

   if (!pObj) return;

   ObjInvArea(pObj,&rcl,pwi,TRUE);
   WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   return;
}
/*-----------------------------------------------[ public ]---------------*/
/*  ObjBoundingRect.                                                      */
/*                                                                        */
/*  Description : Function calculated the bounding rect of the            */
/*                multiselection in pixels. This is later used for        */
/*                the invalidation of the windowrectangle.                */
/*                                                                        */
/* Parameters   : WINDOWINFO * - pointer to the info struct.  [ IN  ]     */
/*                RECTL      * - pointer to a rectl struct    [ OUT ]     */
/*                BOOL         - When true the obj area is including the  */
/*                               size boxes etc.                          */
/*                               FALSE the area is taken as small as poss */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjBoundingRect(WINDOWINFO *pwi, RECTL *rcl, BOOL bInclusive)
{
   POBJECT pObj;
   RECTL   rcltmp;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   rcl->xLeft   = 5000; /* make big */
   rcl->xRight  = 0;
   rcl->yTop    = 0;
   rcl->yBottom = 5000;

   pObj = pBaseObj;  /* Point to the starting point of the chain */

   while (pObj!= NULL)
   {
      if (pObj->bMultiSel)
      {
         ObjInvArea(pObj,&rcltmp,pwi,bInclusive);
         if (rcltmp.xLeft < rcl->xLeft)
            rcl->xLeft = rcltmp.xLeft;
         if (rcltmp.xRight > rcl->xRight)
            rcl->xRight = rcltmp.xRight;
         if (rcltmp.yBottom < rcl->yBottom)
            rcl->yBottom = rcltmp.yBottom;
         if (rcltmp.yTop > rcl->yTop)
            rcl->yTop = rcltmp.yTop;
      }
      pObj =  pObj->Next;
   }
   return;
}
/*------------------------------------------------------------------------*/
/*  ObjQuerySelect                                                        */
/*                                                                        */
/*  Description : This function is called at the WM_BUTTON1UP when        */
/*                a multiselection box is drawn.                          */
/*                                                                        */
/*  Returns     : TRUE when selection is there...                         */
/*------------------------------------------------------------------------*/
BOOL ObjQuerySelect(void)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return FALSE;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         return TRUE;
      pBegin =  pBegin->Next;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/*  ObjShiftSelect.                                                       */
/*                                                                        */
/*  Description : Function is directly called from the WM_BUTTON1DOWN.    */
/*                if the program is in NO_SELECT state the WM_BUTTON1..   */
/*                allows the user to make a multiselection with the mouse */
/*                and the shiftkey.                                       */
/*                So here we introduce another method of selecting objects*/
/*                                                                        */
/*  Concept     : We use the mouse position and pass this to the ObjSelect*/
/*                function which returns us a pointer to an object if     */
/*                the pointer is above an object. If we got something back*/
/*                we put the multiselect bool on true.                    */
/*                If the object was selected we unselect it.              */
/*                Every selected object draws its selection state.        */
/*                                                                        */
/*  Parameters  : POINTL ptl - mouse pointer position.                    */
/*                WINDOWINFO * pointer to our windowinfo.                 */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void ObjShiftSelect(POINTL ptl, WINDOWINFO *pwi)
{
   POBJECT pObj;

   pObj = ObjSelect(ptl,pwi);

   if (pObj && !pObj->bMultiSel)
   {
      pObj->bMultiSel = TRUE;
      showSelectHandles((void*)pObj,pwi);
   }
   else if (pObj && pObj->bMultiSel)
   {
      pObj->bMultiSel = FALSE;
      showSelectHandles((void*)pObj,pwi);
   }
}
/*------------------------------------------------------------------------*/
/*  ObjSelectAll.                                                         */
/*                                                                        */
/*  Description : Function is called when the user presses the Ctrl+/     */
/*                key combination. All objects on the current layer are   */
/*                selected. When the other layers are unlocked, all       */
/*                objectes will be selected.                              */
/*                                                                        */
/*  Parameters  : WINDOWINFO * pwi                                        */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
BOOL ObjSelectAll(WINDOWINFO *pwi)
{
   PBASEOBJ   pBaseType;
   POBJECT    pBegin;
   BOOL       bResult = FALSE;

   if (pBaseObj == NULL)       /*Initial true*/
      return FALSE;

   pBegin = pBaseObj;

   while (pBegin!= NULL)
   {
      pBaseType = (PBASEOBJ)pBegin;

      if (pBaseType->bt.usLayer == pwi->uslayer || pwi->bSelAll)
      {
         pBegin->bMultiSel = TRUE;
         showSelectHandles((void*)pBegin,pwi);
         bResult = TRUE;
      }
      pBegin = pBegin->Next;
   }
   return bResult;
}
/*------------------------------------------------------------------------*/
/*  ObjMultiSelect                                                        */
/*                                                                        */
/*  Description : Directly called from the WM_MOUSEMOVE message. Sets the */
/*                objects in their selected state if they fall in the     */
/*                multiselectbox. If they fall outside the box and where  */
/*                selected than the multiselect boolean is set back to    */
/*                false and the selection handles are switched off.       */
/*                                                                        */
/*  Returns     : TRUE on success.                                        */
/*------------------------------------------------------------------------*/
BOOL ObjMultiSelect(POINTL ptlSt, POINTL ptlEnd, WINDOWINFO *pwi)
{
   pMetaimg pMeta;
   pGroup  pGrp;
   pImage  pImg;
   RECTL   rcl;
   POINTL ptl;          /* used for checking the squares */
   PBASEOBJ pBs;
   BOOL   bDrawSelect=FALSE;
   BOOL   bUnSel=FALSE;
   POBJECT pBegin;


   if (pBaseObj == NULL)       /*Initial true*/
      return FALSE;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   /*
   ** check on text segments first!!
   ** First off all, be sure that ptlSt is upperleft corner & ptlEnd
   ** lowerright corner.
   */
   if (ptlSt.x > ptlEnd.x )
   {
      ptl.x = ptlEnd.x;
      ptlEnd.x = ptlSt.x;
      ptlSt.x = ptl.x;
   }

   if (ptlSt.y < ptlEnd.y)
   {
      ptl.y = ptlEnd.y;
      ptlEnd.y = ptlSt.y;
      ptlSt.y = ptl.y;
   }

   while (pBegin!= NULL)
   {
      ObjOutLine(pBegin,&rcl,pwi,FALSE);

      switch (pBegin->usClass)
      {
         case CLS_GROUP:
            pGrp = (pGroup)pBegin;
            if (pGrp->uslayer == pwi->uslayer || pwi->bSelAll )
            {
               if ((ptlSt.x < rcl.xLeft && ptlEnd.x > rcl.xRight) &&
                   (ptlSt.y > rcl.yTop && ptlEnd.y < rcl.yBottom))
               {
                  if (!pBegin->bMultiSel)
                     bDrawSelect = TRUE;
               }
               else if (pBegin->bMultiSel)
                  bUnSel = TRUE;

            }
            break;
         case CLS_META:
            pMeta = (pMetaimg)pBegin;
            if (pMeta->uslayer == pwi->uslayer || pwi->bSelAll)
            {
               if ((ptlSt.x < rcl.xLeft && ptlEnd.x > rcl.xRight) &&
                   (ptlSt.y > rcl.yTop && ptlEnd.y < rcl.yBottom))
               {
                  if (!pBegin->bMultiSel)
                     bDrawSelect = TRUE;
               }
               else if (pBegin->bMultiSel)
                  bUnSel = TRUE;
            }
            break;
         case CLS_IMG:
            pImg = (pImage)pBegin;
            if (pImg->uslayer == pwi->uslayer || pwi->bSelAll)
            {
               if ((ptlSt.x < rcl.xLeft && ptlEnd.x > rcl.xRight) &&
                   (ptlSt.y > rcl.yTop && ptlEnd.y < rcl.yBottom))
               {
                  if (!pBegin->bMultiSel)
                     bDrawSelect = TRUE;
               }
               else if (pBegin->bMultiSel)
                  bUnSel = TRUE;
            }
            break;

         default:
            pBs = (PBASEOBJ)pBegin;
            if (pBs->bt.usLayer == pwi->uslayer || pwi->bSelAll)
            {
               if ((ptlSt.x <= rcl.xLeft && ptlEnd.x >= rcl.xRight) &&
                   (ptlEnd.y <= rcl.yBottom && ptlSt.y >= rcl.yTop))
               {
                  if (!pBegin->bMultiSel)
                     bDrawSelect = TRUE;
               }
               else if (pBegin->bMultiSel)
                  bUnSel = TRUE;
            }
            break;
      }/* endof switch class */

      if (!pBegin->bMultiSel && bDrawSelect)
      {
         showSelectHandles((void*)pBegin,pwi);
         pBegin->bMultiSel=TRUE;
      }
      else if (pBegin->bMultiSel && bUnSel)
      {
         showSelectHandles((void*)pBegin,pwi);
         pBegin->bMultiSel=FALSE;
      }

      bDrawSelect = FALSE;
      bUnSel      = FALSE;

      pBegin =  pBegin->Next;

   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
void remSelectHandles( WINDOWINFO *pwi )
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         showSelectHandles((void*)pBegin,pwi);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  Name: ObjMultiDelete.                                                 */
/*                                                                        */
/*  Description : Deletes all objects which are selected via the selection*/
/*                square.                                                 */
/*                                                                        */
/*  Parameters  : POBJECT - pointer to the starting point of a chain.     */
/*                Initially this function will be called with a null      */
/*                but as soon a group class is found the function recurses*/
/*                into the subtree.                                       */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ObjMultiDelete(POBJECT pObj)
{
   BOOL RetValue = FALSE;
   BOOL bRet     = FALSE;
   POBJECT pBegin,pTmp;

   if (pBaseObj == NULL)       /*Initial true*/
      return RetValue;

   if (!pObj)
      pBegin = pBaseObj;  /* Point to the starting point of the mainchain */
   else
      pBegin = pObj;

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         RetValue = TRUE;

      pTmp = pBegin;

      pBegin =  pBegin->Next;

      if (RetValue)
      {
         RetValue = FALSE;
         bRet = TRUE;
         ObjDelete(pTmp);
      }
   }
   return bRet;
}
/*------------------------------------------------------------------------*/
void ObjMultiUnSelect()
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      pBegin->bMultiSel = FALSE;
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjDrawSelected.                                                      */
/*                                                                        */
/*  Description : Draws only the selected element into the given hps.     */
/*                Specially written to be able to export selection into   */
/*                a Metafile or Bimapfile. Hence this means also that we  */
/*                should give the object which are selected the origin of */
/*                the selected area.... Moving, drawing and moving back   */
/*                seems to be the solution.                               */
/*                                                                        */
/*                Not drawn are CLS_META !!!!!                            */
/*                                                                        */
/*  Parameters : HPS hps - The presentation space to draw in.             */
/*               RECTL rcl-The selection ractangle, in window coords      */
/*               so internally we translate them to paper coords....      */
/*               WINDOWINFO *pwi.                                         */
/*               BOOL bMove - Tells if the objects should be moved        */
/*               relative to the given square. bMove == FALSE when called */
/*               from a group (recursive call).                           */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
void ObjDrawSelected(POBJECT pObj, HPS hps,WINDOWINFO *pwi,RECTL *prcl,BOOL bMove)
{
   POBJECT pBegin;
   SHORT  dy1,dy2;
   SHORT  dx1,dx2;

   if (pBaseObj == NULL)       /*Initial true*/
      return;
   /*
   ** Translate our rectangle coords into paper coords.....
   */
   if (bMove && prcl)
   {
      dy1 = -prcl->yBottom;
      dx1 = -prcl->xLeft;
      dy2 =  prcl->yBottom;
      dx2 =  prcl->xLeft;
   }
   if (!pObj)
      pBegin = pBaseObj;  /* Point to the starting point of the chain */
   else
      pBegin = pObj;

   /*
   ** bMove is always true when traversing the main chain. But when
   ** this function is called during a recursive call from drwgrp.c
   ** bMove is false and the objects within the group do not have
   ** the selection boolean set. So here we can savely say, if the
   ** bMove is false we have to paint all objects.
   */
   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel || !bMove)
      {
         switch (pBegin->usClass)
         {
            case CLS_TXT:
               if (bMove)
                  MoveTextSegment(pBegin,dx1,dy1,pwi);
               DrawTextSegment(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  MoveTextSegment(pBegin,dx2,dy2,pwi);
               break;
            case CLS_BLOCKTEXT:
               if (bMove)
                  MoveBlockText(pBegin,dx1,dy1,pwi);
               drawBlockText(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  MoveBlockText(pBegin,dx2,dy2,pwi);
               break;
            case CLS_CIR:
               if (bMove)
                  MoveCirSegment(pBegin,dx1,dy1,pwi);
               DrawCircleSegment(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  MoveCirSegment(pBegin,dx2,dy2,pwi);
               break;
            case CLS_LIN:
               if (bMove)
                  MoveLinSegment(pBegin,dx1,dy1,pwi);
               DrawLineSegment(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  MoveLinSegment(pBegin,dx2,dy2,pwi);
               break;
            case CLS_SPLINE:
               if (bMove)
                  SplMoveSegment(pBegin,dx1,dy1,pwi);
               DrawSplineSegment(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  SplMoveSegment(pBegin,dx2,dy2,pwi);
               break;
            case CLS_GROUP:
               if (bMove)
                  GroupMove(pBegin,dx1,dy1,pwi);
               GroupDraw(hps,pwi,pBegin,(RECTL *)0,TRUE);
               if (bMove)
                  GroupMove(pBegin,dx2,dy2,pwi);
               break;
            case CLS_IMG:
               if (bMove)
                  ImgMoveSegment(pBegin,dx1,dy1,pwi);
               DrawImgSegment(hps,pwi,pBegin,(RECTL *)0);
               if (bMove)
                  ImgMoveSegment(pBegin,dx2,dy2,pwi);
               break;
         } /*switch*/
      } /* endif */
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjDrawMetaSelected                                                   */
/*                                                                        */
/*  Description : Draws only the selected element into the given hps.     */
/*                Specially written to be able to export selection into   */
/*                a Metafile.                                             */
/*                                                                        */
/*                Not drawn are CLS_META !!!!!                            */
/*                                                                        */
/*  Parameters : HPS hps - The presentation space to draw in.             */
/*               RECTL rcl-The selection ractangle, in window coords      */
/*               so internally we translate them to paper coords....      */
/*               WINDOWINFO *pwi.                                         */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
void ObjDrawMetaSelected(POBJECT pObj, HPS hps,WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   if (!pObj)
      pBegin = pBaseObj;  /* Point to the starting point of the chain */
   else
      pBegin = pObj;

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         switch (pBegin->usClass)
         {
            case CLS_META:
               break;
            default:
               pBegin->paint(hps,pwi,pBegin,(RECTL *)0);
               break;
         } /*switch*/
      } /* endif */
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/* ObjFontChange.                                                         */
/*                                                                        */
/* Description : changes the font of the given object.                    */
/*                                                                        */
/* Parameters  : char * FontName - Null terminating font name.            */
/*               POBJECT pObj.                                            */
/*               LONG lPointsize.    12,13,14 etc                         */
/*                                                                        */
/* Returns     : NONE                                                     */
/*------------------------------------------------------------------------*/
void ObjFontChange(char * FontName, POBJECT pObj)
{
   Textstruct  pStart;  /*check on text segments*/

   if (pObj == NULL)       /*Initial true*/
      return;

   if (pObj->usClass == CLS_TXT)
   {
      pStart = (Textstruct)pObj;
      strcpy(pStart->fattrs.szFacename,FontName);
   }
   else if (pObj->usClass == CLS_BLOCKTEXT)
      BlockTextFaceName(pObj,FontName);
}
/*------------------------------------------------------------------------*/
void ObjMultiFontChange(char * FontName)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
          ObjFontChange(FontName,pBegin);
      pBegin->bMultiSel=FALSE;
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/* ChangeClrSelItems:                                                     */
/* Changes the fillingcolor of the selected objects.                      */
/*------------------------------------------------------------------------*/
BOOL ObjSetFillClr(ULONG ulNewColor, POBJECT pObj)
{
   PBASEOBJ   pBasetype;

   if (!pObj)
      return FALSE;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
        pBasetype = ( PBASEOBJ )pObj;
        pBasetype->bt.fColor = ulNewColor;
        return TRUE;
   } /*switch*/
   return FALSE;
}
/*------------------------------------------------------------------------*/
static BOOL ObjHasPattern(POBJECT pObj)
{
   PBASEOBJ pBs;

   if (!pObj) return FALSE;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_BLOCKTEXT:
         break;
      case CLS_TXT:
         return TRUE;
      case CLS_LIN:
         break;
      default:
        pBs = (PBASEOBJ)pObj;
        return (BOOL)(pBs->bt.lPattern != PATSYM_DEFAULT);
   } /*switch*/
   return FALSE;
}
/*------------------------------------------------------------------------*/
/* ChangeClrSelItems:                                                     */
/* Changes the fillingcolor of the selected objects after a color drop!   */
/*------------------------------------------------------------------------*/
BOOL ObjDropFillClr(WINDOWINFO *pwi, ULONG ulNewColor, POBJECT pObj)
{
   if (!ObjSetFillClr(ulNewColor,pObj))
      return FALSE;

   if (!ObjHasPattern(pObj))
      ObjPatternChange(PATSYM_SOLID,pObj,pwi,FALSE);
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* OutLine color change of given object.                                  */
/*------------------------------------------------------------------------*/
void ObjSetOutLineClr(ULONG ulNewColor, POBJECT pObj)
{
   PBASEOBJ pBs;

   if (!pObj)
      return;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         pBs = (PBASEOBJ)pObj;
         pBs->bt.line.LineColor = ulNewColor;
         break;
   } /*switch*/
}
/*------------------------------------------------------------------------*/
/* Changes the filling color of the selected objects.                     */
/*------------------------------------------------------------------------*/
void ObjSetMltFillClr(ULONG ulNewColor)
{
   POBJECT pObject;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pObject = pBaseObj;  /* Point to the starting point of the chain */

   while (pObject!= NULL)
   {
      if (pObject->bMultiSel)
      {
         ObjSetFillClr(ulNewColor,pObject);
         pObject->bMultiSel = FALSE;
      }
      pObject =  pObject->Next;
   }
}

/*------------------------------------------------------------------------*/
/* Changes the outline color of the selected objects.                     */
/*------------------------------------------------------------------------*/
void ObjSetMltOutLineClr(ULONG ulNewColor)
{
   POBJECT pObject;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pObject = pBaseObj;  /* Point to the starting point of the chain */

   while (pObject!= NULL)
   {
      if (pObject->bMultiSel)
      {
         ObjSetOutLineClr(ulNewColor,pObject);
         pObject->bMultiSel = FALSE;
      }
      pObject =  pObject->Next;
   }
}
/*------------------------------------------------------------------------*/
/* Change the linewidth of the given object.                              */
/*------------------------------------------------------------------------*/
void ObjLnWidthChange(ULONG ulLnWidth, POBJECT pObj)
{
   PBASEOBJ pBs;

   if (!pObj) return;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_BLOCKTEXT:
         break;
      case CLS_GROUP:
         break;
      default:
         pBs = (PBASEOBJ)pObj;
         pBs->bt.line.LineWidth = ulLnWidth;
   } /*switch*/
}
/*------------------------------------------------------------------------*/
/*  ObjInterChange.                                                       */
/*                                                                        */
/*  Description : When two objects are selected the user can via a button */
/*                put on of the object in front of the other.             */
/*   ÚÄÄÄÄÄÄÄÄ¿             ÚÄÄÄÄÄÄÄÄ¿                                    */
/*   ³        ÃÄÄÄÄ¿        ³      ÚÄÁÄÄÄÄ¿                               */
/*   ³        ³    ³ becomes³      ³      ³                               */
/*   ÀÄÄÄÄÄÄÂÄÙ    ³        ÀÄÄÄÄÄÄ´      ³                               */
/*          ÀÄÄÄÄÄÄÙ               ÀÄÄÄÄÄÄÙ                               */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjInterChange(WINDOWINFO *pwi)
{
   POBJECT pObject,pObjA,pObjB;
   POBJECT NextA,PrevA;
   POBJECT NextB,PrevB;
   RECTL   rcl;
   USHORT  usCount=0;
   BOOL    bPrevious = FALSE;    /* Previous pointer repaired? */


   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pObject = pBaseObj;  /* Point to the starting point of the chain */
   pObjB   = NULL;

   while (pObject!= NULL)
   {
      if (pObject->bMultiSel)
      {
         if (!usCount)
            pObjA = pObject; /* Get first  selected */
         else if (usCount == 1)
         {
            pObjB = pObject; /* Get second selected */
            break;
         }
         usCount++;
      }
      pObject =  pObject->Next;
   }

   if (usCount != 1 || !pObjB)
      return;

   ObjBoundingRect(pwi,&rcl,TRUE);

   if (!pObjA->Previous)
   {
      /*
      ** pObjA seems to the first object in the row...
      ** So the basepointer must be updated and point
      ** to the second objects pObjb. pObjB will take
      ** the place of pObjA.
      */
      pBaseObj = pObjB;

   }

   PrevA = pObjA->Previous;
   NextA = pObjA->Next;

   PrevB = pObjB->Previous;
   NextB = pObjB->Next;


   if (pObjA == PrevB)
   {
      /*
      ** O,o pObjB is the next neighbour of pObjA so it
      ** does not make any sense to give the previous
      ** pointer of pObjB to pObA since this causes pObjA pointing
      ** to itself.
      */

      pObjB->Previous = PrevA;
      pObjB->Next     = pObjA;
      pObjA->Previous = pObjB;
      pObjA->Next     = NextB;

      if (PrevA)
      {
         PrevA->Next = pObjB;
      }

      if (NextB)
      {
         NextB->Previous = pObjA;
      }
      bPrevious = TRUE;
   }

   if (!bPrevious )
   {
      pObjB->Previous = PrevA;
      pObjB->Next     = NextA;

      if (PrevA)
      {
         PrevA->Next = pObjB;
      }

      if (NextA)
      {
         NextA->Previous = pObjB;
      }


      pObjA->Previous = PrevB;
      pObjA->Next     = NextB;

      if (PrevB)
      {
         PrevB->Next = pObjA;
      }

      if (NextB)
      {
         NextB->Previous = pObjA;
      }
      bPrevious = TRUE;
   }

   /*
   ** Update end pointer... point again to end of chain.
   */
   pObjEnd = pBaseObj;
   while (pObjEnd->Next) pObjEnd = pObjEnd->Next;

   ObjMultiUnSelect(); /* Unselelct all objects. */

   WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   return;

}
/*------------------------------------------------------------------------*/
/* Change the linejoin type of the given object.                          */
/*------------------------------------------------------------------------*/
void ObjLnJoinChange(ULONG ulLnJointype,POBJECT pObj)
{
   PBASEOBJ pBs;

   if (!pObj)
      return;

   switch (pObj->usClass)
   {
      case CLS_SPLINE:
         pBs = (PBASEOBJ)pObj;
         pBs->bt.line.LineJoin = ulLnJointype;
         break;
   } /*switch*/
}
/*------------------------------------------------------------------------*/
void ObjMltLnJoinChange(ULONG ulLnJointype)
{
   POBJECT pBegin;

   if (pBaseObj == NULL) return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjLnJoinChange(ulLnJointype,pBegin);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
void ObjLntypeChange(ULONG ulLntype,POBJECT pObj)
{
   PBASEOBJ pBs;

   if (!pObj) return;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         pBs = (PBASEOBJ)pObj;
         pBs->bt.line.LineType = ulLntype;
   } /*switch*/
}
/*------------------------------------------------------------------------*/
void ObjMultiLnWidthChange(ULONG ulLnWidth)
{
  POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjLnWidthChange(ulLnWidth,pBegin);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
void ObjMultiLntypeChange(ULONG ulLntype)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjLntypeChange(ulLntype,pBegin);
      pBegin =  pBegin->Next;
   }
}
/*-----------------------------------------------[ public ]------------------*/
BOOL ObjLineEndChange(POBJECT pObj, WINDOWINFO *pwi)
{
   BOOL    bRet = FALSE;
   PBASEOBJ pBs;

   if (!pObj)
      return FALSE;

   switch (pObj->usClass)
   {
      case CLS_TXT:
         break;
      case CLS_IMG:
         break;
      default:
        pBs = (PBASEOBJ)pObj;
        pBs->bt.arrow.lEnd    = pwi->arrow.lEnd;
        pBs->bt.arrow.lStart  = pwi->arrow.lStart;
        pBs->bt.arrow.lSize   = pwi->arrow.lSize;
        bRet = TRUE;
   } /*switch*/
   return bRet;
}
/*-------------------------------------------------------------------------*/
void ObjMultiLineEndChange(WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjLineEndChange(pBegin,pwi);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
static BOOL SetNewPattern(WINDOWINFO *pwi, PBASEOBJ pBs, 
                          ULONG ulPattern,BOOL bDialog)
{
   switch(ulPattern)
   {
      case PATSYM_GRADIENTFILL:
         if (bDialog)
         {
            if (WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)GradientDlgProc,
                          0L,ID_GRADIENT,(PVOID)&pBs->bt.gradient) == DID_OK)
            {
               pBs->bt.lPattern = ulPattern;
               return TRUE;
            }
            else
               return FALSE;
         }
         else
         {
            pBs->bt.lPattern = ulPattern;
            pBs->bt.gradient.ulStart      = pwi->Gradient.ulStart;
            pBs->bt.gradient.ulSweep      = pwi->Gradient.ulSweep;
            pBs->bt.gradient.ulSaturation = pwi->Gradient.ulSaturation;
            pBs->bt.gradient.ulDirection  = pwi->Gradient.ulDirection;
         }
         return TRUE;
      case PATSYM_FOUNTAINFILL:
         if (bDialog)
         {
            if (WinDlgBox(HWND_DESKTOP,pwi->hwndClient,
                         (PFNWP)FountainDlgProc,0L,
                         ID_DLGFOUNTAIN,(PVOID)&pBs->bt.fountain) == DID_OK)
            {
               pBs->bt.lPattern = ulPattern;
               return TRUE;
            }
            else
               return FALSE;
         }
         else
         {
            pBs->bt.lPattern = ulPattern;
            memcpy(&pBs->bt.fountain,&pwi->fountain,sizeof(FOUNTAIN));
         }
         break;
      default:
         pBs->bt.lPattern = ulPattern;
         break;
   }
   return TRUE;
}
/*---------------------------------------------------------------------------*/
BOOL ObjPatternChange(ULONG ulNewPattern, POBJECT pObj, WINDOWINFO *pwi, BOOL bDialog)
{
   pImage  pImg;
   BOOL    bRet = TRUE;

   if (!pObj)
      return FALSE;

   switch (pObj->usClass)
   {
      case CLS_IMG:
         pImg = (pImage)pObj;
         pImg->lPattern = ulNewPattern;
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         bRet = SetNewPattern(pwi,(PBASEOBJ)pObj,ulNewPattern,bDialog);
   } /*switch*/
   return bRet;
}
/*------------------------------------------------------------------------*/
void ObjMultiPatternChange(ULONG ulNewPattern,WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjPatternChange(ulNewPattern,pBegin,pwi,FALSE);
      pBegin =  pBegin->Next;
   }
}
/*---------------------------------------------------------------------------*/
BOOL ObjShadeChange(ULONG ulShadeType, POBJECT pObj, WINDOWINFO *pwi, BOOL bDialog)
{
   PBASEOBJ pBs;

   pBs = (PBASEOBJ)pObj;

   if (ulShadeType == SHADE_CHANGE)
   {
      if (bDialog)
      {
         if (WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)ShadingDlgProc,
            (HMODULE)0,DLG_SHADING,(void *)&pBs->bt.Shade) == DID_OK)
         {
            return TRUE;
         }
         else
            return FALSE;
      }
      else
      {
         pBs->bt.Shade  = pwi->Shade;
      }
   }
   else 
   {
      /*
      ** User simply wants to have the shadow in another place.
      */
      pBs->bt.Shade.lShadeType = ulShadeType;
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
void ObjMultiShadeChange(ULONG ulShade, WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;
   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         ObjShadeChange(ulShade,pBegin,pwi,FALSE);
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
void ObjMultiMoveCopy(WINDOWINFO *pwi, LONG dx, LONG dy, USHORT op_mode)
{
   POBJECT pBegin,pObjTmp;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         if (op_mode == MULTICOPY)
         {
            pObjTmp = ObjectCopy(pBegin);
            ObjMove(pObjTmp,dx,dy,pwi);
         }
         else
            ObjMove(pBegin,dx,dy,pwi);
      }
      pBegin->rclOutline.xLeft  += dx;
      pBegin->rclOutline.xRight += dx;
      pBegin->rclOutline.yTop   += dy;
      pBegin->rclOutline.yBottom+= dy;

      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
void ObjPutFile(pLoadinfo pli, POBJECT pChain,WINDOWINFO *pwi)
{
   ULONG       ulBytes;
   PBYTE       p;
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   if (!pChain)
   {
      pBegin = pBaseObj;  /* Point to the starting point of the chain */
      /*
      ** Write the forminfo to file.
      */
      fileWrtForm(pli,pwi); /* See drwcanv.c */
   }
   else
      pBegin = pChain;



   while (pBegin)
   {
      ulBytes = ObjectSize[pBegin->usClass] - sizeof(OBJECT);
      p = (PBYTE)pBegin;
      p += sizeof(OBJECT); /* Jump over object struct itself */

      /*
      ** Write the object struct to disk
      */
      _write(pli->handle,(PVOID)p,ulBytes);

      switch (pBegin->usClass)
      {
         case CLS_IMG:
            FilePutImageData(pli,(pImage)pBegin);
            break;
         case CLS_META:
            FilePutMetaData(pli,pBegin);
            break;
         case CLS_SPLINE:
            FilePutSpline(pli,(pSpline)pBegin);
            break;
         case CLS_BLOCKTEXT:
            FilePutBlockText(pli,pBegin);
            break;
         case CLS_GROUP:
            /*
            ** Here the group obj is saved now it's
            ** child chain...recurse into it.
            */
            ObjPutFile(pli,pBegin->NextChild,pwi);
            break;
      }
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/* ObjHorzAllign                                                          */
/*------------------------------------------------------------------------*/
void ObjHorzAlign(WINDOWINFO *pwi)
{
   pCircle pCir;        /*check on cir  segments*/

   RECTL   rcl;

   POBJECT pBegin;
   LONG    yRef,p;      /* reference point      */
   float   fref=0.0;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   yRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */


   while (pBegin!= NULL)
   {
      switch (pBegin->usClass)
      {
         case CLS_TXT:
         case CLS_SPLINE:
            if (pBegin->bMultiSel)
            {
               if (!yRef)
               {
                  ObjOutLine(pBegin,&rcl,pwi,FALSE);
                  yRef = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
                  fref = (float)yRef;
                  fref /= pwi->usFormHeight;
               }
               ObjOutLine(pBegin,&rcl,pwi,FALSE);
               p = rcl.yBottom + (rcl.yTop - rcl.yBottom)/2;
               if (pBegin->usClass == CLS_TXT)
                  MoveTextSegment(pBegin,0,(SHORT)(yRef - p),pwi);
               else
                  SplMoveSegment(pBegin,0,(SHORT)(yRef - p),pwi);
            }
            break;
         case CLS_CIR:
            pCir = (pCircle )pBegin;
            if (pBegin->bMultiSel)
            {
               if (!yRef)
               {
                  yRef = (LONG)(pCir->ptlPosn.y * pwi->usFormHeight);
                  fref = pCir->ptlPosn.y;
               }
               pCir->ptlPosn.y = fref;
            }
            break;
      } /*switch*/
      pBegin->bMultiSel=FALSE;
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/* ObjVertAllign     vertical over the center.                            */
/*------------------------------------------------------------------------*/
void ObjVertAlign(WINDOWINFO *pwi)
{
   pCircle pCir;        /*check on cir  segments*/
   POBJECT pBegin;
   float   fref = 0.0;
   LONG    xRef,p;      /* reference point      */
   RECTL   rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   xRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      switch (pBegin->usClass)
      {
         case CLS_TXT:
         case CLS_SPLINE:
            if (pBegin->bMultiSel)
            {
               if (!xRef)
               {
                  ObjOutLine(pBegin,&rcl,pwi,FALSE);
                  xRef = rcl.xLeft + (rcl.xRight - rcl.xLeft)/2;
                  fref = (float)xRef;
                  fref /= pwi->usFormWidth;
               }
               ObjOutLine(pBegin,&rcl,pwi,FALSE);
               p = rcl.xLeft + (rcl.xRight - rcl.xLeft)/2;
               if ( pBegin->usClass == CLS_TXT)
                  MoveTextSegment(pBegin,(SHORT)(xRef - p),0,pwi);
               else
                  SplMoveSegment(pBegin,(SHORT)(xRef - p),0,pwi);
            }
            break;
         case CLS_CIR:
            pCir = (pCircle )pBegin;
            if (pBegin->bMultiSel)
            {
               if (!xRef)
               {
                  xRef = (LONG)(pCir->ptlPosn.x * pwi->usFormWidth );
                  fref = pCir->ptlPosn.x;
               }
               pCir->ptlPosn.x = fref;
            }
            break;
      } /*switch*/
      pBegin->bMultiSel=FALSE;
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjAlnVertR                                                           */
/*                                                                        */
/*  Description : Vertical alignment with the rightside as ref point      */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjAlnVertR(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
   LONG       xRef;   /* reference point      */
   RECTL      rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   xRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         ObjOutLine(pBegin,&rcl,pwi,FALSE);
         if (!xRef)
            xRef = rcl.xRight;
         ObjMove(pBegin,(SHORT)xRef - rcl.xRight,(SHORT)0,pwi);
      }
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjAlnVertL                                                           */
/*                                                                        */
/*  Description : Vertical alignment with the leftside as ref point       */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjAlnVertL(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
   LONG       xRef;   /* reference point      */
   RECTL      rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   xRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         ObjOutLine(pBegin,&rcl,pwi,FALSE);
         if (!xRef)
            xRef = rcl.xLeft;
         ObjMove(pBegin,(SHORT)xRef - rcl.xLeft,(SHORT)0,pwi);
      }
      pBegin =  pBegin->Next;
   }
}

/*------------------------------------------------------------------------*/
/*  ObjAlnHorzB                                                           */
/*                                                                        */
/*  Description : Horizontal alignment with the bottom as ref point.      */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjAlnHorzB(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
   LONG       cyRef;   /* reference point      */
   RECTL      rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   cyRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         ObjOutLine(pBegin,&rcl,pwi,FALSE);
         if (!cyRef)
            cyRef = rcl.yBottom;
         ObjMove(pBegin,(SHORT)0,(SHORT)cyRef - rcl.yBottom,pwi);
      }
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjAlnHorzT                                                           */
/*                                                                        */
/*  Description : Horizontal alignment with the top as ref point.         */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjAlnHorzT(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
   LONG       cyRef;   /* reference point      */
   RECTL      rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   cyRef = 0;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
      {
         ObjOutLine(pBegin,&rcl,pwi,FALSE);
         if (!cyRef)
            cyRef = rcl.yTop;
         ObjMove(pBegin,(SHORT)0,(SHORT)cyRef - rcl.yTop,pwi);
      }
      pBegin =  pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjGetBoxCorners.                                                     */
/*                                                                        */
/*  Description : Someone clicked with it's mouse on on of the corners    */
/*                of a selected object. Because.... he/she wants to       */
/*                stretch it. At that we wanna known the coords of the    */
/*                oposite corner. Since we use the GpiBox stuff to show   */
/*                a stretching box.                                       */
/*                                                                        */
/*  Parameters : POBJECT pObj : pointer to the selected object.           */
/*               PPOINTL ptl  : pointer the pointl struct where we put in */
/*                              our calculated coords.                    */
/*               USHORT  cnr  : Corner number you clicked on.             */
/*               WINDOWINFO pwi: You calculate to screen coords etc...    */
/*                                                                        */
/*           1ÚÄÄÄÄÄÄÄ¿2                                                  */
/*            ³       ³                                                   */
/*           4ÀÄÄÄÄÄÄÄÙ3                                                  */
/*                                                                        */
/*  Returns : NONE.                                                       */
/*------------------------------------------------------------------------*/
BOOL ObjGetBoxCorners(POBJECT pObj,PPOINTL ptl,USHORT cnr, WINDOWINFO *pwi)
{
   RECTL rcl;

   if (!pObj)
      return FALSE;

   ObjOutLine(pObj,&rcl,pwi,FALSE);

   if (cnr == 1)
   {
      ptl->x = rcl.xRight;
      ptl->y = rcl.yBottom;
   }
   else if (cnr == 2)
   {
      ptl->x = rcl.xLeft;
      ptl->y = rcl.yBottom;
   }
   else if (cnr == 3)
   {
      ptl->x = rcl.xLeft;
      ptl->y = rcl.yTop;
   }
   else if (cnr == 4)
   {
      ptl->x = rcl.xRight;
      ptl->y = rcl.yTop;
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
/*  ObjStretch                                                            */
/*                                                                        */
/*  Description : Stretches the object a certain factor. Uses the old     */
/*                objectoutline and the new rectangle to define the factor*/
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
BOOL ObjStretch(POBJECT pObj,PPOINTL ptlStart,PPOINTL ptlEnd,WINDOWINFO *pwi,ULONG ulMsg)
{
   RECTL rclNew;


   if (!pObj)
      return FALSE;

   /*
   ** first of all make a rectl of our square corner points
   ** ptlEnd & Start...
   */

   if (ptlStart->x <= ptlEnd->x)
   {
      rclNew.xLeft = ptlStart->x;
      rclNew.xRight= ptlEnd->x;
   }
   else
   {
      rclNew.xLeft = ptlEnd->x;
      rclNew.xRight= ptlStart->x;
   }

   if (ptlStart->y <= ptlEnd->y)
   {
      rclNew.yBottom = ptlStart->y;
      rclNew.yTop  = ptlEnd->y;
   }
   else
   {
      rclNew.yBottom = ptlEnd->y;
      rclNew.yTop  = ptlStart->y;
   }

   switch (pObj->usClass)
   {
      case CLS_META:
         MetaStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      case CLS_IMG:
         ImageStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      case CLS_TXT:
         TextStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      case CLS_GROUP:
         GrpStretch(pObj,&rclNew,pwi);
         break;
      case CLS_CIR:
         CirStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      case CLS_SPLINE:
         SplStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      case CLS_BLOCKTEXT:
         BlockTextStretch(pObj,&rclNew,pwi,ulMsg);
         break;
      default:
         DosBeep(880,40);
         DosBeep(440,40);
         DosBeep(220,40);
         DosBeep(440,40);
         break;
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
/*  ObjSetInvArea.                                                        */
/*                                                                        */
/*  Description : Called when zooming is done. Some objects, in this case */
/*                only the circle. Calculate their inv area by drawing    */
/*                the circle while GPI dawing mode is set to DCTL_BOUNDARY*/
/*                                                                        */
/*------------------------------------------------------------------------*/
void ObjSetInvArea(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
   RECTL      rcl;

   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      switch(pBegin->usClass)
      {
         case CLS_CIR:
            CircleInvArea(pBegin,&rcl,pwi,TRUE);
            break;
         case CLS_SPLINE:
            SplineInvArea((pSpline)pBegin,&rcl,pwi,TRUE);
            break;
      }
      pBegin = pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  ObjSuppressfil.                                                       */
/*                                                                        */
/*  Description : Switches the filling of the objects off. This to speed  */
/*                updrawing.                                              */
/*------------------------------------------------------------------------*/
void ObjSuppressfill(POBJECT pObj,BOOL bSuppress)
{
   if (pObj == NULL)
      return;

   pObj->bSuppressfill = bSuppress;
   return;
}
/*------------------------------------------------------------------------*/
/*  ObjRotation...                                                        */
/*                         ROTATION STARTS HERE.........                  */
/*  Description : Under development.                                      */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY RotSnapDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
   static WINDOWINFO *pwi; /* Pointer to the windowinfo         */
   SWP    swp;             /* Screen Window Position Holder     */
   ULONG ulStorage[2];     /* To get the vals out of the spins  */
   PVOID pStorage;         /* idem spinbutton.                  */

   switch (msg)
   {
      case WM_INITDLG:
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx)),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy)),
		       0, 0, SWP_MOVE);

         /* setup the snap on degrees spinbutton. */

         WinSendDlgItemMsg( hwnd, ID_SPINROTSNAP, SPBM_SETLIMITS,
                            MPFROMLONG(MAXDEG), MPFROMLONG(MINDEG));

         WinSendDlgItemMsg( hwnd, ID_SPINROTSNAP, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)1), NULL);

         hSnapRot = hwnd;

         pwi  = (WINDOWINFO *)mp2;

         return (MRESULT)0;

      case WM_COMMAND:
         bDialog = FALSE;
	 switch(LOUSHORT(mp1))
         {
            case DID_CANCEL:
               if (pwi)
               {
                  /*
                  ** Set the program back in selectmode.
                  */
                  WinPostMsg(pwi->hwndClient,WM_CHAR,MPFROM2SHORT(KC_VIRTUALKEY,1),
                             MPFROM2SHORT(0,VK_ESC));
                  pwi = (WINDOWINFO *)0;
               }
               WinDismissDlg(hwnd,DID_CANCEL);
               break;
         }
         return (MRESULT)0;

      case WM_CONTROL:
         if (LOUSHORT(mp1) == ID_SPINROTSNAP)
         {
            if (HIUSHORT(mp1) == SPBN_CHANGE)
            {
              pStorage = (PVOID)ulStorage;
              WinSendDlgItemMsg(hwnd,ID_SPINROTSNAP,
                                SPBM_QUERYVALUE,(MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= 1 && ulStorage[0] <= 45 )
                 lSnapDeg = (USHORT)ulStorage[0];
            }
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}

static float GetAngle(POINTL ptlMouse, POINTL ptlCenter)
{
   double  alfa,piRad;
   float   fx,fy;

   ptlMouse.x -= ptlCenter.x;
   ptlMouse.y -= ptlCenter.y;

   fx = (float)ptlMouse.x;
   fy = (float)ptlMouse.y;

   if (ptlMouse.x)
   {
      alfa  = (double)fy/fx;
      piRad = atan(alfa);              /* Angle in pi radialen */

      if (ptlMouse.x < 0 && ptlMouse.y >=0 )          /* 2e kwadrant */
         piRad += (double)(PI);

      else if (ptlMouse.x < 0 && ptlMouse.y < 0 )     /* 3e kwadrant */
         piRad -= (double)(PI);
   }
   else
   {
      alfa  = (double)0;
      if (ptlMouse.y >=0)
         piRad = (double)(PI/2);
      else if (ptlMouse.y < 0)
         piRad = (double)(1.75 * PI);
   }
   return (float)piRad;
}

static BOOL ptInRotateHandle(WINDOWINFO *pwi,POINTL ptl,POBJECT pObj)
{
   RECTL rcl;

   ObjOutLine(pObj,&rcl,pwi,FALSE);

   rcl.yBottom += (rcl.yTop -  rcl.yBottom)/2 - HANDLESIZE;
   rcl.yTop     = rcl.yBottom + (HANDLESIZE * 2);

   rcl.xRight  += HANDLESIZE;
   rcl.xLeft    = rcl.xRight  - (HANDLESIZE *2);

   return WinPtInRect((HAB)0,&rcl,&ptl);

}
/*------------------------------------------------------------------------*/
/* ObjDrawRotate                                                          */
/*                                                                        */
/* description   : Draws the given object in the given angle.             */
/*                 Called for instance by the group object.               */
/*                 and by ObjDrawRotLine.                                 */
/*                                                                        */
/*  Parameters   : WINDOWINFO * - pointer to the well known info.         */
/*                 POBJECT        pointer to the object to draw during rot*/
/*                 double         Angle of rotation.                      */
/*                 umMsg        - Mouse message.                          */
/*                 POINTL       - Center of rotation. Filled in by group  */
/*                                Normally a NULL pointer!                */
/*------------------------------------------------------------------------*/
void ObjDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double Angle,ULONG ulMsg,POINTL *pt)
{
   switch(pObj->usClass)
   {
      case CLS_SPLINE:
         SplDrawRotate(pwi,pObj,Angle,ulMsg,pt);
         break;
      case CLS_CIR:
         CirDrawRotate(pwi,pObj,Angle,ulMsg,pt);
         break;
      case CLS_TXT:
         TxtDrawRotate(pwi,pObj,Angle,ulMsg,pt);
         break;
      case CLS_LIN:
         LinDrawRotate(pwi,pObj,Angle,ulMsg,pt);
         break;
      case CLS_GROUP:
         GrpDrawRotate(pwi,pObj,Angle,ulMsg,pt);
         break;
   }
   return;
}

MRESULT ObjDrawRotLine(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl,ULONG ulMsg)
{
   static  POINTL ptlCenter;
   static  float  Angle,StartAngle;
   static  BOOL bButton1down;
   char    szBuf[150];
   static  LONG deg,lSnap;
   PBASEOBJ pBs = (PBASEOBJ)pObj;

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         if (!ptInRotateHandle(pwi,ptl,pObj))
            return (MRESULT)0;

         bButton1down = TRUE;
         WinSetDlgItemText(hSnapRot,ID_TXTDEGREES,"0");
         switch(pObj->usClass)
         {
            case CLS_IMG:
               break;
            case CLS_META:
               break;
            case CLS_BLOCKTEXT:
               break;
            case CLS_GROUP:
               GrpGetCenter(pwi,pObj,&ptlCenter);
               break;
            default:
               ptlCenter.x = (LONG)(pBs->bt.ptlfCenter.x * pwi->usFormWidth);
               ptlCenter.y = (LONG)(pBs->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         StartAngle = GetAngle(ptl,ptlCenter);
         ObjDrawRotate(pwi,pObj,(float)0,ulMsg,(POINTL *)0);
         return (MRESULT)1;

      case WM_MOUSEMOVE:
         if (!bButton1down )
         {
            if (ptInRotateHandle(pwi,ptl,pObj))
            {
               WinSetPointer(HWND_DESKTOP,
                             WinQuerySysPointer(HWND_DESKTOP,
                                                   SPTR_SIZENS,FALSE));
            }
            else 
            {
               WinSetPointer(HWND_DESKTOP,WinQuerySysPointer(HWND_DESKTOP,
                             SPTR_ARROW,FALSE));
            }
            return (MRESULT)0;
         }
         Angle = GetAngle(ptl,ptlCenter);
         Angle -= StartAngle;
         deg = (Angle * 360)/(2 * PI);  /* Make it degrees      */

         lSnap = deg;

         if (lSnapDeg > 1)
         {
            lSnap = deg % lSnapDeg;
            deg  -= lSnap;
            Angle = ((float)deg) * ((2 * PI)/360);
         }
         itoa (deg, szBuf, 10);

         if (hSnapRot)
            WinSetDlgItemText(hSnapRot,ID_TXTDEGREES,szBuf);

         ObjDrawRotate(pwi,pObj,Angle,ulMsg,(POINTL *)0);

         return (MRESULT)1;

      case WM_BUTTON1UP:
         if (!bButton1down || !deg)
            break;

         ObjDrawRotate(pwi,pObj,Angle,ulMsg,(POINTL *)0);

         bButton1down = FALSE;
         Angle = (float)0;
         StartAngle = (float)0;
         return (MRESULT)1;
   }
   return (MRESULT)0;
}
/*--------------------------------------------------------------------------*/
static void ObjRestoreCenter(WINDOWINFO *pwi, POBJECT pObj)
{
   switch(pObj->usClass)
   {
      case CLS_GROUP:
         break;
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_BLOCKTEXT:
         break;
      case CLS_TXT:
         break;
      case CLS_CIR:
         cirRestoreCenter(pwi,pObj);
         break;
      case CLS_SPLINE:
         break;
   }
}
/*--------------------------------------------------------------------------*/
static BOOL OnRealCenter(WINDOWINFO *pwi, POBJECT pObj, POINTL ptlMouse)
{
   POINTL ptl;
   RECTL  rcl;
   
   switch(pObj->usClass)
   {
      case CLS_GROUP:
         return FALSE;
      case CLS_IMG:
         return FALSE;
      case CLS_META:
         return FALSE;
      case CLS_BLOCKTEXT:
         return FALSE;
      case CLS_TXT:
         return FALSE;
      case CLS_CIR:
         cirGetRealCenter(pwi,pObj,&ptl);
         break;
      case CLS_SPLINE:
         return FALSE;
   }
   rcl.xLeft    = ptl.x;
   rcl.xLeft   -= HANDLESIZE;
   rcl.xRight   = rcl.xLeft   + (2 * HANDLESIZE);
   rcl.yBottom  = ptl.y;
   rcl.yBottom -= HANDLESIZE;
   rcl.yTop     = rcl.yBottom + (2 * HANDLESIZE);
   return WinPtInRect((HAB)0,&rcl,&ptlMouse);
}
/*--------------------------------------------------------------------------*/
MRESULT ObjMoveCenter(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl,ULONG ulMsg)
{
   PBASEOBJ pBs   = (PBASEOBJ)pObj;
   ARCPARAMS arcp = { HANDLESIZE,HANDLESIZE,0,0};
   POBJECT  p;
   PBASEOBJ pBsnap; /* Snap to object... */
   RECTL    rcl;
   POINTL   ptBox;
   POINTL   ptArc;
   static   POINTL  ptMouse;  /* Mouse starting point!                  */
   static   POINTL  ptlCenter; /* Remember point of center drawing exact!*/

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         ptlCenter.x = (LONG)(pBs->bt.ptlfCenter.x * pwi->usFormWidth);
         ptlCenter.y = (LONG)(pBs->bt.ptlfCenter.y * pwi->usFormHeight);
         ptMouse     = ptl;
         return (MRESULT)0;

      case WM_MOUSEMOVE:
         /*
         ** Calculate exactly the position of the arc to avoid
         ** staying circles.
         */
         ptArc.x = ptlCenter.x + ( ptl.x - ptMouse.x);
         ptArc.y = ptlCenter.y + ( ptl.y - ptMouse.y);
         GpiSetCurrentPosition(pwi->hps,&ptArc);
         GpiSetArcParams(pwi->hps,&arcp);
         GpiFullArc(pwi->hps,DRO_OUTLINEFILL,MAKEFIXED(1,0));
         GpiFullArc(pwi->hps,DRO_OUTLINE,MAKEFIXED(2,0));
         /*
         ** If the rose moves over a center of another object
         ** show the user a small square!
         */
         p = pBaseObj;  /* Point to the starting point of the chain */
         while (p)
         {
           if ( p != pObj && ObjPtrAboveCenter(pwi,p,ptl))
           {
              ptBox.x = ptl.x - (HANDLESIZE * 3);
              ptBox.y = ptl.y - (HANDLESIZE * 3);
              GpiMove(pwi->hps,&ptBox);
              ptBox.x = ptl.x + (HANDLESIZE * 3);
              ptBox.y = ptl.y + (HANDLESIZE * 3);
              GpiBox(pwi->hps,DRO_OUTLINE,&ptBox,0,0);
           }
           else if (p == pObj && OnRealCenter(pwi,pObj,ptl) )
           {
              ptBox.x = ptl.x - (HANDLESIZE * 3);
              ptBox.y = ptl.y - (HANDLESIZE * 3);
              GpiMove(pwi->hps,&ptBox);
              ptBox.x = ptl.x + (HANDLESIZE * 3);
              ptBox.y = ptl.y + (HANDLESIZE * 3);
              GpiBox(pwi->hps,DRO_OUTLINE,&ptBox,0,0);
           }
           p = p->Next;
         }
         return (MRESULT)0;

      case WM_BUTTON1UP:
         /*
         ** Calculate the inv area of the rose at is previous location.
         ** Than we can invalidate that area when we leave this routine
         ** and wipe the old one out.
         */
         rcl.xLeft = ptMouse.x - HANDLESIZE * 3;
         rcl.xRight= ptMouse.x + HANDLESIZE * 3;
         rcl.yBottom=ptMouse.y - HANDLESIZE * 3;
         rcl.yTop   =ptMouse.y + HANDLESIZE * 3;
         GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)&rcl);

         p = pBaseObj;  /* Point to the starting point of the chain */
         while (p)
         {
           if ( p != pObj && ObjPtrAboveCenter(pwi,p,ptl))
              break;
           else if ( p == pObj && OnRealCenter(pwi,pObj,ptl))
              break;
           p = p->Next;
         }
         
         if (p && p != pObj)
         {
            pBsnap = (PBASEOBJ)p;
            pBs->bt.ptlfCenter = pBsnap->bt.ptlfCenter;
            ObjRefresh(p,pwi);
            return (MRESULT)0;
         }
         else if (p && p == pObj)
         {
            ObjRestoreCenter(pwi,pObj);
            WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Old rect .. */
            return (MRESULT)0;
         }

         switch(pObj->usClass)
         {
            case CLS_GROUP:
               GrpSetCenter(pwi,pObj,&ptl);
               break;
            case CLS_IMG:
               break;
            case CLS_META:
               break;
            case CLS_BLOCKTEXT:
               break;
            default:
               pBs = (PBASEOBJ)pObj;
               pBs->bt.ptlfCenter.x = (float)ptl.x;
               pBs->bt.ptlfCenter.x /= pwi->usFormWidth;
               pBs->bt.ptlfCenter.y = (float)ptl.y;
               pBs->bt.ptlfCenter.y /= pwi->usFormHeight;
               WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Old rect .. */
               return (MRESULT)TRUE;
         }
         WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);   /* Old rect .. */
         return (MRESULT)0;
   }
   return (MRESULT)0;
}
/*---------------------------------------------------------------------------*/
void ObjRefreshRotCenter(WINDOWINFO * pwi, POBJECT pObj)
{
   RECTL    rcl;
   POINTL   ptCenter;
   PBASEOBJ pBs = (PBASEOBJ)pObj;

   ptCenter.x = (LONG)(pBs->bt.ptlfCenter.x * pwi->usFormWidth);
   ptCenter.y = (LONG)(pBs->bt.ptlfCenter.y * pwi->usFormHeight);

   rcl.xLeft = ptCenter.x - HANDLESIZE * 3;    
   rcl.xRight= ptCenter.x + HANDLESIZE * 3;
   rcl.yBottom=ptCenter.y - HANDLESIZE * 3;
   rcl.yTop   =ptCenter.y + HANDLESIZE * 3;
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)&rcl);
   WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
}
/*---------------------------------------------------------------------------*/
void StartupSnapRotDlg(WINDOWINFO *pwi,BOOL bStart)
{
   static HWND hwndDlg;

   if (!bDialog && bStart)
   {
      hwndDlg = WinLoadDlg(HWND_DESKTOP,pwi->hwndClient,(PFNWP)RotSnapDlgProc,
                 0L,ID_ROTSNAP,(VOID *)pwi);
      bDialog = TRUE;
      return;
   }

   if (bDialog && hwndDlg && !bStart)
   {
      bDialog = FALSE;
      WinDismissDlg(hwndDlg,DID_CANCEL);
      hwndDlg = (HWND)0;
   }
   return;
}
/*-----------------------------------------------[ public ]---------------*/
/* isClipPathSelected.                                                    */
/*                                                                        */
/* Description  : This function is called from the menu module, to check  */
/*                wetter the add clippath option in the image menu must   */
/*                be grayed out.                                          */
/*                                                                        */
/* Returns      : BOOL - TRUE when the clippath selection is correct.     */
/*------------------------------------------------------------------------*/
BOOL ObjIsClipPathSelected( void )
{
   POBJECT pBegin;
   POBJECT pObjImg    = NULL;
   POBJECT pObjSpl    = NULL;

   if (pBaseObj == NULL)       /*Initial true*/
      return FALSE;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL && !pObjImg)
   {
      if (pBegin->bMultiSel)
      {
         if (pBegin->usClass == CLS_IMG)
            pObjImg = pBegin;
      }
      pBegin =  pBegin->Next;
   }
   
   if (!pObjImg)
      return FALSE; /* No image selected */

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL && !pObjSpl)
   {
      if (pBegin->bMultiSel)
      {
         if (pBegin->usClass == CLS_SPLINE)
            pObjSpl = pBegin;
      }
      pBegin =  pBegin->Next;
   }

   if (!pObjSpl)
      return FALSE;  /* No Spline selected */
   
   return TRUE;
}
/*-----------------------------------------------[ public ]---------------*/
BOOL ObjMakeImgClipPath( WINDOWINFO *pwi )
{
   POBJECT pBegin;
   POBJECT pObjImg    = NULL;
   POBJECT pObjSpl    = NULL;

   if (pBaseObj == NULL)       /*Initial true*/
      return FALSE;
   /*
   ** Search for the selected image...
   */
   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL && !pObjImg)
   {
      if (pBegin->bMultiSel)
      {
         if (pBegin->usClass == CLS_IMG)
            pObjImg = pBegin;
      }
      pBegin =  pBegin->Next;
   }
   
   if (!pObjImg)
      return FALSE;    /* No image selected. */

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   while (pBegin!= NULL && !pObjSpl)
   {
      if (pBegin->bMultiSel)
      {
         if (pBegin->usClass == CLS_SPLINE)
            pObjSpl = pBegin;
      }
      pBegin =  pBegin->Next;
   }

   if (!pObjSpl)
      return FALSE;  /* No Spline selected */

   ObjRefresh(pObjImg,pwi);
   
   imgAddClipPath(pObjImg,pObjSpl,pwi);

   return TRUE;
}
/*------------------------------------------------------------------------*/
void ObjChangeRopCode(WINDOWINFO *pwi, POBJECT pObj,LONG lRop)
{
   switch(pObj->usClass)
   {
      case CLS_IMG:
         imgChangeRopCode(pObj,lRop);
         ObjRefresh(pObj,pwi);
         break;
   }
}
/*------------------------------------------------------------------------*/
ULONG ObjGetFillType(WINDOWINFO *pwi, POBJECT pObj)
{
   PBASEOBJ pBs;
   if (!pObj)
      return (ULONG)pwi->ColorPattern;

   pBs = (PBASEOBJ)pObj;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         return pBs->bt.lPattern;
   }
   return (ULONG)pwi->ColorPattern;
}
/*------------------------------------------------------------------------*/
ULONG ObjGetLineType(WINDOWINFO *pwi, POBJECT pObj)    /* Linetype        */
{
   PBASEOBJ pBs;

   if (!pObj)
      return (ULONG)pwi->lLntype;

   pBs = (PBASEOBJ)pObj;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         return pBs->bt.line.LineType;
   }
   return (ULONG)pwi->lLntype;
}
/*---------------------------------------------------------------------------*/
ULONG ObjGetLineJoin(WINDOWINFO *pwi, POBJECT pObj)    /* LineJoin           */
{
   PBASEOBJ pBs;

   pBs = (PBASEOBJ)pObj;

   if (!pObj)
      return (ULONG)pwi->lLnJoin;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         return pBs->bt.line.LineJoin;
   }
   return (ULONG)pwi->lLnJoin;
}
/*---------------------------------------------------------------------------*/
ULONG ObjGetLineWidth(WINDOWINFO *pwi, POBJECT pObj)    /* LineWidth         */
{
   PBASEOBJ pBs;

   pBs = (PBASEOBJ)pObj;

   if (!pObj)
      return (ULONG)pwi->lLnWidth;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         return pBs->bt.line.LineWidth;
   }
   return (ULONG)pwi->lLnWidth;
}
/*---------------------------------------------------------------------------*/
ULONG ObjGetAlignment(WINDOWINFO *pwi, POBJECT pObj)
{
   if (!pObj)
      return ALIGN_JUST;

   switch(pObj->usClass)
   {
      case CLS_BLOCKTEXT:
         return BlockTextGetAlign(pObj);
   }
   return ALIGN_JUST;   
}
/*---------------------------------------------------------------------------*/
ULONG ObjGetLineSpace(WINDOWINFO *pwi, POBJECT pObj)
{
   if (!pObj)
      return ALIGN_JUST;

   switch(pObj->usClass)
   {
      case CLS_BLOCKTEXT:
         return BlockTextGetSpacing(pObj);
   }
   return SPACE_SINGLE; 
}
/*------------------------------------------------------------------------*/
ULONG ObjGetShadeType(WINDOWINFO *pwi, POBJECT pObj)
{
   PBASEOBJ pBs;
   if (!pObj)
      return (ULONG)pwi->Shade.lShadeType;

   pBs = (PBASEOBJ)pObj;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         return pBs->bt.Shade.lShadeType;
   }
   return (ULONG)pwi->Shade.lShadeType;
}
/*--------------------------------------------------------------------------*/
BOOL ObjEditText(POBJECT pObj, WINDOWINFO *pwi)
{
   if (!pObj)
      return FALSE;

   switch(pObj->usClass)
   {
      case CLS_TXT:
         return txtEdit(pObj,pwi);
      case CLS_BLOCKTEXT:
         return BlockTextEdit(pObj,pwi);
   }
   return FALSE;
}
/*---------------------------------------------------------------------------*/
/* ObjFontDlg.                                                               */
/*                                                                           */
/* Description : Starts the font dialog for the given object.                */
/*                                                                           */
/* Returns     : BOOL - TRUE on success.                                     */
/*---------------------------------------------------------------------------*/
BOOL ObjectFontDlg(POBJECT pObj, WINDOWINFO *pwi)
{
   if (!pObj)
      return FALSE;

   switch(pObj->usClass)
   {
      case CLS_TXT:
         return txtSetFont(pObj,pwi);
      case CLS_BLOCKTEXT:
         return BlockTextFontDlg(pObj,pwi);
   }
   return FALSE;
}
/*---------------------------------------------------------------------------*/
BOOL ObjChangeAlignment(WINDOWINFO *pwi,POBJECT pObj,USHORT usAlign)
{

   if (!pObj)
      return FALSE;

   switch(pObj->usClass)
   {
      case CLS_BLOCKTEXT:
         BlockTextSetAlign(pObj,usAlign);
         return TRUE;
   }
   return FALSE;
}
/*---------------------------------------------------------------------------*/
BOOL ObjChangeSpacing(WINDOWINFO *pwi,POBJECT pObj,USHORT usSpace)
{
   if (!pObj)
      return FALSE;

   switch(pObj->usClass)
   {
      case CLS_BLOCKTEXT:
         BlockTextSetSpacing(pObj,usSpace);
         return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
void ObjPreparePrinting(WINDOWINFO *pwi)
{
   POBJECT    pBegin;
 
   if (pBaseObj == NULL)       /*Initial true*/
      return;

   pBegin = pBaseObj;  /* Point to the starting point of the chain */

   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);

   do
   {
      if (pBegin->usClass == CLS_BLOCKTEXT)
         BlockTextPrepPrint(pBegin,pwi);
      pBegin =  pBegin->Next;
   } while (pBegin);
   GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
}
/*---------------------------------------------------------------------------*/
/* copyAttributes                                                            */
/*                                                                           */
/* Description  : Copies the object attributes to the application ....       */
/*---------------------------------------------------------------------------*/
MRESULT copyAttributes(WINDOWINFO *pwi, POBJECT pObj)
{
   PBASEOBJ pBs;

   pBs = (PBASEOBJ)pObj;

   if (!pObj)
      return (MRESULT)0;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         pwi->ulOutLineColor = pBs->bt.line.LineColor;
         pwi->lLntype  = pBs->bt.line.LineType;
         pwi->lLnWidth = pBs->bt.line.LineWidth;
         pwi->lLnJoin  = pBs->bt.line.LineJoin;
         pwi->lLnEnd   = pBs->bt.line.LineEnd;
         /*
         ** Get the gradient stuff...
         */
         pwi->Gradient.ulStart      = pBs->bt.gradient.ulStart;
         pwi->Gradient.ulSweep      = pBs->bt.gradient.ulSweep;
         pwi->Gradient.ulSaturation = pBs->bt.gradient.ulSaturation;
         pwi->Gradient.ulDirection  = pBs->bt.gradient.ulDirection;
         /*
         ** Fountain fill stuff....
         */
         pwi->fountain      = pBs->bt.fountain;
         pwi->ColorPattern  = pBs->bt.lPattern;
         pwi->ulColor       = pBs->bt.fColor;
         pwi->arrow         = pBs->bt.arrow;
         pwi->Shade         = pBs->bt.Shade;
         WinPostMsg(pwi->hwndMain,UM_FORECOLORCHANGED,
                    MPFROMLONG(pwi->ulColor),
                    (MPARAM)0);
         WinPostMsg(pwi->hwndMain,UM_BACKCOLORCHANGED,
                    MPFROMLONG(pwi->ulOutLineColor),
                    (MPARAM)0);
         break;
   }
   return (MRESULT)0;
}
/*---------------------------------------------------------------------------*/
/* pasteAttributes                                                           */
/*                                                                           */
/* Description  : Copies the aplication attibutes into selected object.      */
/*---------------------------------------------------------------------------*/
MRESULT pasteAttributes(WINDOWINFO *pwi, POBJECT pObj)
{
   PBASEOBJ pBs;

   pBs = (PBASEOBJ)pObj;

   if (!pObj)
      return (MRESULT)0;

   switch(pObj->usClass)
   {
      case CLS_IMG:
         break;
      case CLS_META:
         break;
      case CLS_GROUP:
         break;
      default:
         setBaseAttributes(pBs,pwi);
         break;
   }
   return (MRESULT)0;
}
/*---------------------------------------------------------------------------*/
MRESULT multiPasteAttribs(WINDOWINFO *pwi)
{
   POBJECT pBegin;

   if (pBaseObj == NULL)       /*Initial true*/
      return (MRESULT)0;

   pBegin = pBaseObj;          /* Point to the starting point of the chain */

   while (pBegin!= NULL)
   {
      if (pBegin->bMultiSel)
         pasteAttributes(pwi,pBegin);
      pBegin =  pBegin->Next;
   }
   return (MRESULT)0;
}
