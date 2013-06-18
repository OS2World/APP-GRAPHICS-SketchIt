/*------------------------------------------------------------------------*/
/*  Name: drwgrp.c                                                        */
/*                                                                        */
/*  Description : Functions for handling grouping and ungrouping          */
/*                of objects.                                             */
/*                                                                        */
/* Functions  :                                                           */
/*  GroupCopy      :Copies the entire group..                             */
/*  GoupoutLine    :Returns the outline of the group in a rcl struct.     */
/*  GroupSelect    :Returns pGroup if mouse is clicked on our object.     */
/*  GroupMove      :Move the whole group...                               */
/*  GroupDraw      :Draw the whole group in the given hps.                */
/*  GroupCreate    :Create a group from multiselected objects.            */
/*  GroupDelete    :Deletes the group including it's children             */
/*  GroupUnGroup   :UnGroups a given group.                               */
/*  GroupReconstruct:Reconstruct the tree after we read a drawitfile.     */
/*  canPackGroup   : Can this group be packed to one single layer?        */
/*                                                                        */
/* Private:                                                               */
/*  PutObjectsInGroup : Puts all objects in the group and calcs outline   */
/*  sortElements      : Sorts all elements according to their layer.      */
/*  changeGroupLayer  : Change the layer of the group to the given one.   */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drwtypes.h"  /* Get our drawit types */
#include "dlg.h"
#include "drwutl.h"
#include "dlg_sqr.h"
#include "drwgrp.h"
#include "dlg_img.h"
#include "dlg_cir.h"
#include "drwsplin.h"
#include "dlg_txt.h"
#include "dlg_lin.h"

#define  CX_STARTVALUE 35000
#define  CY_STARTVALUE 35000

static char *StrClass[] = {"NONE","CLS_TXT","CLS_SQR",
                           "CLS_CIR","CLS_IMG","CLS_LIN",
                           "CLS_BOX","CLS_META",
                           "CLS_SPLINE","CLS_GROUP",
                           "CLS_GROUPEND"};

/*
** BASECLASS DEFINITION!!
*/
typedef struct _baseobj
{
   OBJECT   obj;
   USHORT   ustype;      /* type of object          */
   BATTR    bt;          /* Base attributes.        */
} BASEOBJ, *PBASEOBJ;

/*----------------------------------------------[ private ]---------------*/
/*  GoupSetOutline                                                        */
/*                                                                        */
/*  Description : Sets the outline sizes of a group after rotation.       */
/*                                                                        */
/*  Parameters  : pGroup - pointer to a groupobject.                      */
/*                PRECTL - pointer to a rectangle structure.              */
/*                WINDOWINFO * - pointer to the windowinfo....            */
/*                Windowinfo contains the parent info which is the paper  */
/*                or the group wherein this group lives.                  */
/*                                                                        */
/* Returns      : In the rectl structure the outline in window coords     */
/*------------------------------------------------------------------------*/
static void GroupSetOutline(pGroup pGrp, WINDOWINFO *pwi, RECTL *prcl)
{
   pGrp->rclf.xRight  = (float)prcl->xRight;
   pGrp->rclf.xLeft   = (float)prcl->xLeft;
   pGrp->rclf.yBottom = (float)prcl->yBottom;
   pGrp->rclf.yTop    = (float)prcl->yTop;
   pGrp->rclf.xRight  /= pwi->usFormWidth; 
   pGrp->rclf.xLeft   /= pwi->usFormWidth;
   pGrp->rclf.yBottom /= pwi->usFormHeight;
   pGrp->rclf.yTop    /= pwi->usFormHeight;
}
/*------------------------------------------------------------------------*/
/*  GrpPutInGroup.                                                        */
/*                                                                        */
/*  Description : Puts the group in a group. The position etc will be     */
/*                made relative to it's parent group.                     */
/*                Called from ObjDeGroup (drwutl.c).                      */
/*                                                                        */
/*  Parameters  : POBJECT pObj - pointer to the selected CLS_GROUP.       */
/*                                                                        */
/*                RECTL * - points to the rectangle containing the real   */
/*                world coords.                                           */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
static void GrpPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   pGroup  pGrp = (pGroup)pObj;
   USHORT  usGrpWidth,usGrpHeight;


   if (!pObj) return;

   usGrpWidth = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight= (USHORT)( prcl->yTop   - prcl->yBottom);

   /*
   ** Setup the new rotation center.
   */
   pGrp->ptlfCenter.x = (float)(prcl->xLeft + (prcl->xRight - prcl->xLeft)/2);
   pGrp->ptlfCenter.y = (float)(prcl->yBottom + (prcl->yTop - prcl->yBottom)/2);
   pGrp->ptlfCenter.x /= (float)usGrpWidth;
   pGrp->ptlfCenter.y /= (float)usGrpHeight;
   /*
   ** Make the value absolute.
   */
   pGrp->rclf.xRight  *= (float)pwi->usFormWidth; 
   pGrp->rclf.xLeft   *= (float)pwi->usFormWidth;
   pGrp->rclf.yBottom *= (float)pwi->usFormHeight;
   pGrp->rclf.yTop    *= (float)pwi->usFormHeight;

   /*
   ** Subtract x and y pos of our parent...
   ** So we live at an offset!!!
   */
   pGrp->rclf.xRight  -= (float)prcl->xLeft;
   pGrp->rclf.xLeft   -= (float)prcl->xLeft;
   pGrp->rclf.yBottom -= (float)prcl->yBottom;
   pGrp->rclf.yTop    -= (float)prcl->yBottom;

   pGrp->rclf.xRight  /= (float)usGrpWidth;
   pGrp->rclf.xLeft   /= (float)usGrpWidth;
   pGrp->rclf.yBottom /= (float)usGrpHeight;
   pGrp->rclf.yTop    /= (float)usGrpHeight;

   return;
}
/*
** singlechainOutline.....
*/
#if 0
static BOOL singlechainOutline(POBJECT pObj,WINDOWINFO *pwi)
{
  RECTL   rcl;
  pGroup  pGrp = (pGroup)pObj;
  POBJECT pChain;
  RECTL   rclMax;
  BOOL    bCalc;

  if (!pObj->NextChild)
     return FALSE;

  pChain = pObj->NextChild;

  rclMax.xRight = 0;     /* make as big as possible   */
  rclMax.xLeft  = 35000; /* make as small as possible */
  rclMax.yTop   = 0;     /* make as big as possible   */
  rclMax.yBottom= 35000; /* make as small as possible */

  while (pChain && pChain->usClass != CLS_GROUPEND)
  {
     if (pChain->usClass != CLS_GROUP)
     {
        ObjOutLine(pChain,&rcl,pwi,TRUE);
        bCalc = TRUE;
     }
     else
        bCalc = FALSE;

     pChain = pChain->Next;

     if (bCalc)
     {
        if (rcl.xRight > rclMax.xRight)
           rclMax.xRight = rcl.xRight;
        if (rcl.xLeft < rclMax.xLeft)
           rclMax.xLeft = rcl.xLeft;
        if (rcl.yTop > rclMax.yTop)
           rclMax.yTop = rcl.yTop;
        if (rcl.yBottom < rclMax.yBottom)
           rclMax.yBottom = rcl.yBottom;
     }
  }
  GroupSetOutline(pGrp,pwi,&rclMax);
  return TRUE;
}
#endif
/*------------------------------------------------------------------------*/
/* rem all from goup (not really)                                         */
/*------------------------------------------------------------------------*/
static void remAllFromGroup(WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl)
{
  pGroup  pGrp = (pGroup)pObj;
  RECTL   rcl;
  POBJECT pChain;

  if (!pObj->NextChild)
     return;

  printf("------------------------ENTER remAllFromGroup--------\n");
  pChain = pObj->NextChild;
  if (!prcl)
     GroupOutLine(pGrp,&rcl,pwi);
  else
     rcl = *prcl;

  while (pChain && pChain->usClass != CLS_GROUPEND)
  {
     if (pChain->usClass == CLS_GROUP)
        remAllFromGroup(pwi,pChain,&rcl);

     ObjRemFromGroup(pChain,&rcl,pwi);
     pChain = pChain->Next;
  }
  printf("------------------------LEAVE remAllFromGroup--------\n");

  return;
}
/*-----------------------------------------------[ private ]--------------*/
/* put all back in the  goups (not really)                                */
/*------------------------------------------------------------------------*/
static void putAllBack(WINDOWINFO *pwi,POBJECT pObj,RECTL *rcx,BOOL bRecurse)
{
   pGroup  pGrp = (pGroup)pObj;
   POBJECT pChain;

   if (!pObj->NextChild)
      return;

   GroupSetOutline(pGrp,pwi,rcx);

   pChain = pObj->NextChild;

   while (pChain && pChain->usClass != CLS_GROUPEND )
   {
      switch (pChain->usClass)
      {
         case CLS_TXT:
            TxtPutInGroup(pChain,rcx,pwi);
            break;
         case CLS_LIN:
            LinPutInGroup(pChain,rcx,pwi);
            break;
         case CLS_CIR:
            CirPutInGroup(pChain,rcx,pwi);
            break;
         case CLS_SPLINE:
            SplPutInGroup(pChain,rcx,pwi);
            break;
         case CLS_GROUP:
            GrpPutInGroup(pChain,rcx,pwi);
            break;
      }
      pChain = pChain->Next;
   }
}
/*
** Get outline of all removed objects
*/
static void outlineCalc(WINDOWINFO *pwi, POBJECT pObj, PRECTL prclmax)
{
//  pGroup  pGrp = (pGroup)pObj;
  POBJECT pChain;
  RECTL   rcl;
  BOOL    bCalc = FALSE;
  RECTL   rclGroup;

#ifdef DEBUG
   printf("------------------------ENTER outlineCalc----------------\n");
#endif

  if (!pObj->NextChild)
     return;

  pChain = pObj->NextChild;

  while (pChain && pChain->usClass != CLS_GROUPEND)
  {
     
     if (pChain->usClass != CLS_GROUP)
     {
        ObjOutLine(pChain,&rcl,pwi,TRUE);
        bCalc = TRUE;
#ifdef DEBUG
       printf("CLASS [%s]\n",StrClass[pChain->usClass]);
       printf("OBJ - CX[%d] - CY[%d]\n",(rcl.xRight-rcl.xLeft),(rcl.yTop - rcl.yBottom));
#endif
     }
     else
     {
        rclGroup.xRight = 0;             /* make as big as possible   */
        rclGroup.xLeft  = CX_STARTVALUE; /* make as small as possible */
        rclGroup.yTop   = 0;             /* make as big as possible   */
        rclGroup.yBottom= CY_STARTVALUE; /* make as small as possible */

        outlineCalc(pwi,pChain,&rclGroup);
        GroupSetOutline((pGroup)pChain,pwi,&rclGroup);
        printf(">>--SetGroupOutline....\n");
        printf("GROUP - CX[%d] - CY[%d]\n",(rclGroup.xRight-rclGroup.xLeft),(rclGroup.yTop - rclGroup.yBottom));
        printf(">>----\n");
        rcl = rclGroup;
        bCalc = TRUE;
     }
     pChain = pChain->Next;

     if (bCalc)
     {
        if (rcl.xRight > prclmax->xRight)
           prclmax->xRight = rcl.xRight;
        if (rcl.xLeft < prclmax->xLeft)
           prclmax->xLeft = rcl.xLeft;
        if (rcl.yTop > prclmax->yTop)
           prclmax->yTop = rcl.yTop;
        if (rcl.yBottom < prclmax->yBottom)
           prclmax->yBottom = rcl.yBottom;
     }
#ifdef DEBUG
       printf("PRCLMAX - CX[%d] - CY[%d]\n",(prclmax->xRight-prclmax->xLeft),(prclmax->yTop - prclmax->yBottom));
#endif

  }

#ifdef DEBUG
   printf("------------------------LEAVE outlineCalc----------------\n");
#endif

  return;
}
/*-----------------------------------------------[ private ]--------------*/
/*  GroupCalcOutline.                                                     */
/*                                                                        */
/*  Description : Runs throught the group object chain and calculates the */
/*                outline for the group. This is done after a rotation    */
/*                of the group.                                           */
/*                                                                        */
/*                1 - remove all objects from group but leave chain       */
/*                unmodified.                                             */
/*                2 - Calculate the boundary of all objects in the group  */
/*                chain.                                                  */
/*                3 - Tell objects again that they live in a group.       */
/*                                                                        */
/*  Parameters  : POBJECT pointer to the first element of the group chain.*/
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void GroupCalcOutline(POBJECT pObj, WINDOWINFO *pwi)
{
   RECTL   rclMax;
   rclMax.xRight = 0;             /* make as big as possible   */
   rclMax.xLeft  = CX_STARTVALUE; /* make as small as possible */
   rclMax.yTop   = 0;             /* make as big as possible   */
   rclMax.yBottom= CY_STARTVALUE; /* make as small as possible */

   if (!pObj->NextChild)
      return;
   /*
   ** Step 1
   */
   remAllFromGroup(pwi,pObj,(RECTL *)0);
   /*
   ** Step 2
   */
   outlineCalc(pwi,pObj,&rclMax);
   /*
   ** Step 3
   */
   putAllBack(pwi,pObj,&rclMax,FALSE);
}
/*------------------------------------------------------------------------*/
/*  PutObjectsInGroup.                                                    */
/*                                                                        */
/*  Description : Runs throught the group object chain and calculates the */
/*                outline for the group and puts the objects in the group.*/
/*                                                                        */
/*  Parameters  : POBJECT pointer to the first element of the group chain.*/
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
static void PutObjectsInGroup(POBJECT pObj, WINDOWINFO *pwi)
{
   RECTL   rclMax;
   RECTL   rcl;
   POBJECT pChain,pTmp;
   pGroup  pGrp = (pGroup)pObj;

   rclMax.xRight = 0;     /* make as big as possible   */
   rclMax.xLeft  = 35000; /* make as small as possible */
   rclMax.yTop   = 0;     /* make as big as possible   */
   rclMax.yBottom= 35000; /* make as small as possible */

   if (!pObj->NextChild)
      return;

   pTmp = pChain = pObj->NextChild;

   while (pChain && pChain->usClass != CLS_GROUPEND )
   {
      ObjOutLine(pChain,&rcl,pwi,FALSE);
      pChain = pChain->Next;

      if (rcl.xRight > rclMax.xRight)
         rclMax.xRight = rcl.xRight;

      if (rcl.xLeft < rclMax.xLeft)
         rclMax.xLeft = rcl.xLeft;

      if (rcl.yTop > rclMax.yTop)
         rclMax.yTop = rcl.yTop;

      if (rcl.yBottom < rclMax.yBottom)
         rclMax.yBottom = rcl.yBottom;
   }
   /*
   ** Get back to the starting point. Now we gonna put the objects
   ** in the group box...
   */
   pChain = pTmp;
   while (pChain && pChain->usClass != CLS_GROUPEND )
   {
      switch (pChain->usClass)
      {
         case CLS_TXT:
            TxtPutInGroup(pChain,&rclMax,pwi);
            break;
         case CLS_LIN:
            LinPutInGroup(pChain,&rclMax,pwi);
            break;
         case CLS_CIR:
            CirPutInGroup(pChain,&rclMax,pwi);
            break;
         case CLS_GROUP:
            GrpPutInGroup(pChain,&rclMax,pwi);
            break;
         case CLS_SPLINE:
            SplPutInGroup(pChain,&rclMax,pwi);
            break;
         default:
            break;
      }
      pChain->bMultiSel = FALSE;
      pChain = pChain->Next;
   }

   pGrp->rclf.xRight  = (float)rclMax.xRight;
   pGrp->rclf.xLeft   = (float)rclMax.xLeft;
   pGrp->rclf.yBottom = (float)rclMax.yBottom;
   pGrp->rclf.yTop    = (float)rclMax.yTop;

   pGrp->ptlfCenter.x = (float)(rclMax.xLeft + (rclMax.xRight - rclMax.xLeft)/2);
   pGrp->ptlfCenter.y = (float)(rclMax.yBottom + (rclMax.yTop - rclMax.yBottom)/2);

   pGrp->rclf.xRight  /= pwi->usFormWidth; 
   pGrp->rclf.xLeft   /= pwi->usFormWidth;
   pGrp->rclf.yBottom /= pwi->usFormHeight;
   pGrp->rclf.yTop    /= pwi->usFormHeight;

   pGrp->ptlfCenter.x /= (float)pwi->usFormWidth;
   pGrp->ptlfCenter.y /= (float)pwi->usFormHeight;

   pGrp->ustype      = CLS_GROUP;
   pGrp->uslayer     = pwi->uslayer;

}
/*-----------------------------------------------[ private ]--------------*/
static BOOL allElementsOnOneLayer(POBJECT pObj)
{
   pGroup   pGrp = (pGroup)pObj;
   POBJECT  pChain;
   USHORT   usLayer;
   PBASEOBJ pB;

   if (!pObj->NextChild)
      return FALSE;  /* Not a group so quit! */

   usLayer = pGrp->uslayer;

   pChain = pObj->NextChild; /* Start of chain */

   do
   {
       pB = (PBASEOBJ)pChain;
       if (pB->bt.usLayer != usLayer)
          return FALSE;
       pChain = pChain->Next;
   } while (pChain && pChain->usClass != CLS_GROUPEND );

   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  changeGroupLayer.                                                     */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void changeGroupLayer(POBJECT pObj, USHORT usLayer)
{
   POBJECT  pChain;
   PBASEOBJ pB;
   pGroup   pGrp = (pGroup)pObj;

   if (!pObj->NextChild)
      return;  /* Not a group so quit! */

   pGrp->uslayer         = usLayer;

   pChain = pObj->NextChild; /* Start of chain */

   do
   {
       pB = (PBASEOBJ)pChain;
       pB->bt.usLayer = usLayer;
       pChain = pChain->Next;

   } while (pChain && pChain->usClass != CLS_GROUPEND );
}
/*-----------------------------------------------[ private ]--------------*/
/*  sortGroupElements.                                                    */
/*                                                                        */
/*  Description : Sorts the element according to the layer where they are */
/*                put. This makes it finally possible to put a group on   */
/*                particular layer.                                       */
/*                                                                        */
/*  Parameters  : POBJECT pointer to the first element of the group chain.*/
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
static void sortGroupElements(POBJECT pObj, USHORT usLayer)
{
   POBJECT  pChain,pEnd;
   PBASEOBJ pB,*pArray;
   pGroup   pGrp = (pGroup)pObj;
   BOOL    bChange;
   int     i,iCount = 0;


   if (!pObj->NextChild)
      return;  /* Not a group so quit! */

   pChain = pObj->NextChild; /* Start of chain */

   do
   {
     iCount++;
     pChain = pChain->Next;
   } while (pChain && pChain->usClass != CLS_GROUPEND );

   pEnd = pChain;

   pArray = (PBASEOBJ *)calloc(iCount,sizeof( void *));

   pChain = pObj->NextChild; /* Start of chain */

   for ( i = 0; i < iCount; i++)
   {
       pArray[i] = (PBASEOBJ)pChain;
       pChain = pChain->Next;
   }
   /*
   ** Sort the array on layer....
   */
   do
   {
      bChange = FALSE;
      for (i=0; i < iCount-1; i++)
      {
         if (pArray[i]->bt.usLayer > pArray[i+1]->bt.usLayer)
         {
            bChange = TRUE;
            pB = pArray[i];
            pArray[i]   = pArray[i+1];
            pArray[i+1] = pB;
         }
      }
   } while ( bChange); 
   /*
   ** Move all to the given layer
   */
   for ( i = 0; i < iCount; i++)
       pArray[i]->bt.usLayer = usLayer;

   /*
   ** Remake the chain...
   */
   for (i = 0; i < iCount-1; i++)
   {
      if (i == 0)
      {
         ((POBJECT)pArray[i])->Previous = NULL;
         ((POBJECT)pArray[i])->Next     = (POBJECT)pArray[i+1];
         pObj->NextChild                = (POBJECT)pArray[i];
      }
      else
      {
         ((POBJECT)pArray[i])->Previous = (POBJECT)pArray[i-1];
         ((POBJECT)pArray[i])->Next     = (POBJECT)pArray[i+1];
      }
   }
   ((POBJECT)pArray[iCount-1])->Next  = (POBJECT)pEnd;
   pEnd->Previous        = (POBJECT)pArray[iCount-1];
   pGrp->uslayer         = usLayer;
   free(pArray);
}
/*------------------------------------------------------------------------*/
/*  GroupCreate                                                           */
/*                                                                        */
/*  Description : Here we come if the user makes the Group menu choice    */
/*                after a multiselect is done. What we do now is to find  */
/*                the last selected object in the 'main chain'            */
/*                on this position the CLS_GROUP object is created.       */
/*                Now we start traversing again through the main chain    */
/*                but now we put the selected object in the new 'child'   */
/*                chain and remove them from the main chain.              */
/*                of objects.                                             */
/*                                                                        */
/*  Parameters: POBJECT - pointer to the first object in the main chain.  */
/*                                                                        */
/*  Precondition:                                                         */
/*   The selected element are numbered.                                   */
/*   ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿   */
/*   ³aÃ´bÃ´cÃ´1Ã´dÃ´2Ã´3Ã´4Ã´eÃ´fÃ´5Ã´gÃ´hÃ´iÃ´jÃ´kÃ´lÃ´mÃ´nÃ´oÃ´pÃ´q³   */
/*   ÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙ   */
/* Postcondition:                                                         */
/*   The element with the 'G'is CLS_GROUP.                                */
/*   ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿               */
/*   ³aÃ´bÃ´cÃ´dÃ´eÃ´fÃ´GÃ´gÃ´hÃ´iÃ´jÃ´kÃ´lÃ´mÃ´nÃ´oÃ´pÃ´q³               */
/*   ÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÂÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙ               */
/*                     ÚÁ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄ¿ÚÄÄÄÄÄÄÄÄÄÄÄ¿                       */
/*                     ³1Ã´2Ã´3Ã´4Ã´5Ã´CLS_GRPEND ³                       */
/*                     ÀÄÙÀÄÙÀÄÙÀÄÙÀÄÙÀÄÄÄÄÄÄÄÄÄÄÄÙ                       */
/*   Since a file is sequential we need and childchain end marker in this */
/*   case it is the CLS_GROUPEND marker (Group end).                      */
/*                                                                        */
/* Returns : None.                                                        */
/*------------------------------------------------------------------------*/
POBJECT GroupCreate(POBJECT pBegin, WINDOWINFO *pwi)
{
   POBJECT pTmp,plSel;  /* plSel = pointer to last selected object.      */
   POBJECT pB,pN;       /* Previous pointer and Next pointer to remember */ 
   POBJECT pLast;       /* Last object in child chain where we append.   */ 
   POBJECT pObjGrp;     /* Group object inserted at pos of last sel obj  */ 
   pGroupEnd pGrpEnd;
   USHORT  usSel;       /* Number of selected items..                    */

   pTmp = pBegin; /* remember start of chain! */

  if (!pBegin)
     return NULL;

  plSel = (POBJECT)0;
  pLast = (POBJECT)0;
  usSel = 0;
  /*
  ** 1 - find last selected element in the chain.
  */
  while (pBegin)
  {
     if (pBegin->bMultiSel)
     {
        plSel = pBegin;
        usSel++;
     }
     pBegin = pBegin->Next;
  }
  pBegin = pTmp; /* Back to the start */

  if (!plSel || usSel <= 1)
  {
     WinAlarm(HWND_DESKTOP, WA_ERROR);
     return NULL;   /* There was nothing selected or just one element.*/
  }
  /*
  ** 2 - Create CLS_GROUP object...
  */
  if ((pObjGrp =(POBJECT)calloc(sizeof(Group),1))==NULL)
  {
     ErrorBox("Group error","Not enough memory");
     return NULL;
  }

  pObjGrp->usClass = CLS_GROUP;      /* Class identifier */
  pObjGrp->moveOutline = GroupMoveOutline; 
  pObjGrp->paint       = GrpDraw;
  pObjGrp->getInvalidationArea = GroupInvArea;

//  printf("Last selected [%s]\n",StrClass[plSel->usClass]);
  /*
  ** 3 - put our group in the place of plSel.
  */
  pObjGrp->Next     = plSel->Next;
  pObjGrp->Previous = plSel->Previous;
  /*
  ** Check if there is a previous object, if so tell it
  ** that it gets a new next object, namely the cls_group.
  */
  if (plSel->Previous)
  {
     pB = plSel->Previous;
     pB->Next = pObjGrp;
  }

  if (plSel->Next)
  {
     pN = plSel->Next;
     pN->Previous = pObjGrp;
  }
  /*
  ** At this point we are linked in...
  ** Start traversing the chain removing the selected items and
  ** put these in the new created 'child' chain.
  */
   while (pBegin)
   {
      if (pBegin->bMultiSel)
      {
         /*
         ** Remember the next pointer, we need the original to
         ** traverse the chain.
         */
         pN = pBegin->Next; 

         ObjExtract(pBegin);
         /*
         ** If this is the first item in the new chain
         ** the child pointer of pObjGrp->child must be NULL!
         */
         if (!pObjGrp->NextChild)
         {
//            printf("Adding first child to chain [%s]\n",StrClass[plSel->usClass]);
            /*
            ** Since we can be (pBegin) a group don't touch the
            ** NextChild pointer!!!
            */
            pObjGrp->NextChild = pBegin;
            pBegin->Next      = NULL;
            pBegin->Previous  = NULL;
            pBegin->PrevChild = pObjGrp;
            pLast = pBegin;
         }
         else
         {
//            printf("Adding child to chain [%s]\n",StrClass[plSel->usClass]);
            pLast->Next = pBegin;
            pBegin->Previous = pLast;
            /*
            ** go to last in chain...
            */
            pLast = pLast->Next;
            pLast->Next = NULL;
         }

         /*
         ** Step to the next..
         */
         pBegin = pN;
      }
      else
         pBegin = pBegin->Next;
   }

//   printf("**Adding last child to chain [%s]**\n",StrClass[plSel->usClass]);
   pLast->Next = plSel;
   plSel->Previous = pLast;
   pLast = pLast->Next;
   pLast->Next = NULL;

   pTmp = pObjNew(pwi,CLS_GROUPEND);
   /*
   ** Ad the end of chain marker, in this case only for file I/O
   ** Cast it to the GroupEnd structure since this part will be saved!
   ** (2bytes).
   */
   pLast->Next     = pTmp;
   pTmp->Previous  = pLast;
   pLast           = pLast->Next;
   pLast->Next     = NULL;
   pGrpEnd         = (pGroupEnd)pTmp;
   pGrpEnd->ustype = CLS_GROUPEND; /* This goes into file.... as marker */

   PutObjectsInGroup(pObjGrp,pwi);
/*  sortGroupElements(pObjGrp); */

   return pObjGrp;
}
/*------------------------------------------------------------------------*/
/*  GoupoutLine                                                           */
/*                                                                        */
/*  Description : Returns the outline of the group object.                */
/*                                                                        */
/*  Parameters  : pGroup - pointer to a groupobject.                      */
/*                PRECTL - pointer to a rectangle structure.              */
/*                WINDOWINFO * - pointer to the windowinfo....            */
/*                                                                        */
/* Returns      : In the rectl structure the outline in window coords     */
/*------------------------------------------------------------------------*/
void GroupOutLine(pGroup pGrp, RECTL *rcl, WINDOWINFO *pwi)
{
   if (!pGrp)
      return;

   rcl->xRight = (LONG)(pGrp->rclf.xRight  * pwi->usFormWidth);
   rcl->xLeft  = (LONG)(pGrp->rclf.xLeft   * pwi->usFormWidth);
   rcl->yBottom= (LONG)(pGrp->rclf.yBottom * pwi->usFormHeight);
   rcl->yTop   = (LONG)(pGrp->rclf.yTop    * pwi->usFormHeight);

   rcl->xRight  += (LONG)pwi->fOffx;
   rcl->xLeft   += (LONG)pwi->fOffx;
   rcl->yBottom += (LONG)pwi->fOffy;
   rcl->yTop    += (LONG)pwi->fOffy;
}
/*------------------------------------------------------------------------*/
/*  Name: GroupSelect.                                                    */
/*                                                                        */
/*  Description : Did someone clicked with the mouse in our rectl?        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID * GroupSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj)
{
   pGroup pGrp = (pGroup)pObj;
   RECTL rcl;

   if (pGrp->uslayer == pwi->uslayer || pwi->bSelAll)
   {
      GroupOutLine(pGrp, &rcl,pwi);
      if (ptl.x > rcl.xLeft && ptl.x < rcl.xRight)
         if (ptl.y > rcl.yBottom && ptl.y < rcl.yTop)
            return (void *)pGrp;
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/*  Name: GroupInvArea.                                                   */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void  GroupInvArea(POBJECT pObj, RECTL *rcl, WINDOWINFO *pwi, BOOL bInc)
{
   pGroup pGrp = (pGroup)pObj;

   if (!pGrp) return;
   GroupOutLine(pGrp,rcl,pwi);

   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);
   /*
   ** Add extra space for selection handles.
   */
   if (bInc)
   {
      rcl->xLeft    -= HANDLESIZE;
      rcl->xRight   += HANDLESIZE;
      rcl->yBottom  -= HANDLESIZE;
      rcl->yTop     += HANDLESIZE;
   }
}
/*------------------------------------------------------------------------*/
/*  GroupMove                                                             */
/*                                                                        */
/*------------------------------------------------------------------------*/
void GroupMove(POBJECT pObj,SHORT dx,SHORT dy,WINDOWINFO *pwi)
{
   float  fdx,fdy,fcx,fcy;
   pGroup pGrp = (pGroup)pObj;

   if (!pGrp)return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;
      
   fdx = (fdx /fcx);
   fdy = (fdy /fcy);

   pGrp->rclf.xRight  += fdx;
   pGrp->rclf.xLeft   += fdx;
   pGrp->rclf.yBottom += fdy;
   pGrp->rclf.yTop    += fdy;
   pGrp->ptlfCenter.x += fdx;
   pGrp->ptlfCenter.y += fdy;
}
/*------------------------------------------------------------------------*/
/*  GroupDraw                                                             */
/*                                                                        */
/*------------------------------------------------------------------------*/
void GroupDraw(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl,BOOL bSelect)
{
   RECTL rcl,rclDest;
   pGroup pGrp = (pGroup)pObj;
   USHORT usOldWidth,usOldHeight;
   USHORT usWidth,usHeight;
   float  fOldOffx,fOldOffy;
   /*
   ** Since group drawing can be very time consuming,
   ** we first look (only if prect is a vallid pointer)
   ** if we should redraw it.
   */
   GroupOutLine((pGroup)pObj, &rcl,pwi);

   if (prcl)
   {
      if (!WinIntersectRect(hab,&rclDest,prcl,&rcl))
        return;
   }

   usOldWidth  = pwi->usFormWidth;
   usOldHeight = pwi->usFormHeight;
   fOldOffx    = pwi->fOffx;
   fOldOffy    = pwi->fOffy;

   usWidth  = (USHORT)(rcl.xRight - rcl.xLeft);
   usHeight = (USHORT)(rcl.yTop   - rcl.yBottom);

   pwi->fOffx += pGrp->rclf.xLeft   * pwi->usFormWidth;
   pwi->fOffy += pGrp->rclf.yBottom * pwi->usFormHeight;


   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;


   if (pObj->NextChild)
   {
      /*
      ** Go one line down in the chain.
      */
      pObj = pObj->NextChild;
      if (!bSelect)
         ObjDrawSegment(hps,pwi,pObj,prcl);
      else
         ObjDrawSelected(pObj,hps,pwi,(RECTL *)0,FALSE);
   }
   pwi->usFormWidth = usOldWidth;
   pwi->usFormHeight= usOldHeight;
   pwi->fOffx = fOldOffx;
   pwi->fOffy = fOldOffy;

   return;
}

void GrpDraw(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl)
{
   GroupDraw(hps,pwi,pObj,prcl,FALSE);
}

/*------------------------------------------------------------------------*/
/*  grpDrawOutline                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void grpDrawOutline(POBJECT pObj,WINDOWINFO *pwi, BOOL bIgnoreLayer)
{
   RECTL  rcl;
   pGroup pGrp = (pGroup)pObj;
   USHORT usOldWidth,usOldHeight;
   USHORT usWidth,usHeight;
   float  fOldOffx,fOldOffy;
   /*
   ** Since group drawing can be very time consuming,
   ** we first look (only if prect is a vallid pointer)
   ** if we should redraw it.
   */
   GroupOutLine((pGroup)pObj, &rcl,pwi);

   if (pGrp->uslayer != pwi->uslayer && !bIgnoreLayer)
      return;

   usOldWidth  = pwi->usFormWidth;
   usOldHeight = pwi->usFormHeight;
   fOldOffx    = pwi->fOffx;
   fOldOffy    = pwi->fOffy;

   usWidth  = (USHORT)(rcl.xRight - rcl.xLeft);
   usHeight = (USHORT)(rcl.yTop   - rcl.yBottom);

   pwi->fOffx += pGrp->rclf.xLeft   * pwi->usFormWidth;
   pwi->fOffy += pGrp->rclf.yBottom * pwi->usFormHeight;


   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;


   if (pObj->NextChild)
   {
      /*
      ** Go one line down in the chain.
      */
      pObj = pObj->NextChild;
      ObjMultiDrawOutline(pwi,pObj,bIgnoreLayer);
   }
   pwi->usFormWidth = usOldWidth;
   pwi->usFormHeight= usOldHeight;
   pwi->fOffx = fOldOffx;
   pwi->fOffy = fOldOffy;
   return;
}
/*------------------------------------------------------------------------*/
/*  GroupReconstruct.                                                     */
/*                                                                        */
/*  Description : After the DRAWIT file is loaded from disk we need to    */
/*                do some reconstruction work. Since the loaded structure */
/*                is a sequential chain we must now reorder it.           */
/*                Called from module:drwutl.c.                            */
/*                                                                        */
/*  Parameters :  POBJECT pChain starting point of the main chain.        */
/*                                                                        */
/*  Returns    : NONE.                                                    */
/*------------------------------------------------------------------------*/
void GroupReconstruct(POBJECT pChain)
{
   POBJECT pBegin = pChain;
   POBJECT pTmp,pGroupObj;

//   printf("Enter group reconstruct\n");
   while (pBegin)
   {
      if (pBegin->usClass == CLS_GROUP)
      {
	 pBegin->moveOutline = GroupMoveOutline; 
	 pBegin->paint       = GrpDraw;
         pBegin->getInvalidationArea = GroupInvArea;
         pTmp = pBegin->Next;
         pBegin->NextChild = pTmp;
         pTmp->PrevChild = pBegin;
//         printf("Recurse [%s]\n",StrClass[pTmp->usClass]);
         GroupReconstruct(pTmp);
      }
      else if (pBegin->usClass == CLS_GROUPEND)
      {
//         printf("Recontstruct:CLS_GROUPEND\n");
         pTmp = pChain;
         /*
         ** Go back to the chain were we came from,
         ** one level up. If everything is alright
         ** we are than in CLS_GROUP object!
         ** The GROUP object's next pointer will be connected
         ** with the one the CLS_GROUPEND Next pointer is 
         ** connected with and the next pointer of the endobj
         ** is set to null.
         */
         while (!pTmp->PrevChild && pTmp->Previous)
            pTmp = pTmp->Previous;

         if (!pTmp->PrevChild)
            return;

         pTmp = pTmp->PrevChild; /* Back to the cls_group */
         
//         if (pTmp->usClass == CLS_GROUP)
//            printf("Yes we are CLS_GROUP again...\n");

         pTmp->Next = pBegin->Next;
         pGroupObj = pTmp;
         pTmp = pTmp->Next;
         if (pTmp)
         {
            pTmp->Previous = pGroupObj;
            pBegin->Next = NULL; /* Close child chain */
         }         
      }
      pBegin = pBegin->Next;
   }
}
/*------------------------------------------------------------------------*/
/*  GroupDelete                                                           */
/*                                                                        */
/*  Description : Deletes a group from the chain.                         */
/*                of objects.                                             */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void GroupDelete(POBJECT pObj)
{
   POBJECT Tmp,p;
   pSpline pSpl;        /*delete on POLYLINES    */
   pImage  pImg;

   if (!pObj)
      return;
   
   do {

      switch (pObj->usClass)
      {
         case CLS_SPLINE:
            pSpl = (pSpline)pObj;
            free(pSpl->pptl);       /* Delete the allocated points.*/
            break;
         case CLS_IMG:
            pImg = (pImage)pObj;
            if (pImg->ImgData)
               free(pImg->ImgData);
            if (pImg->pbmp2)
               free(pImg->pbmp2);
            break;
         case CLS_GROUP:
            Tmp = pObj->NextChild;
            GroupDelete(Tmp);
            break;
      }
 
      p = pObj->Next;
      free(pObj);
      pObj = p;

   }while (pObj);

   return;
}
/*------------------------------------------------------------------------*/
/*  GroupStretch                                                          */
/*                                                                        */
/*  Description : Stretches all elements in a given group.                */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void GrpStretch(POBJECT pObj,RECTL *rcl,WINDOWINFO *pwi)
{
   pGroup pGrp = (pGroup)pObj;

   USHORT usOldWidth,usOldHeight;
   USHORT usWidth,usHeight;
   float  fOldOffx,fOldOffy;

   usOldWidth  = pwi->usFormWidth;
   usOldHeight = pwi->usFormHeight;
   fOldOffx    = pwi->fOffx;
   fOldOffy    = pwi->fOffy;


   if (!pGrp) return;

   pGrp->rclf.xRight   = (float)rcl->xRight;
   pGrp->rclf.xRight  /= pwi->usFormWidth;
   pGrp->rclf.xLeft    = (float) rcl->xLeft;
   pGrp->rclf.xLeft   /= pwi->usFormWidth;
   pGrp->rclf.yBottom  = (float) rcl->yBottom;
   pGrp->rclf.yBottom /= pwi->usFormHeight;
   pGrp->rclf.yTop     = (float)rcl->yTop;
   pGrp->rclf.yTop    /= pwi->usFormHeight;

   pGrp->ptlfCenter.x = (float)(rcl->xLeft + (rcl->xRight - rcl->xLeft)/2);
   pGrp->ptlfCenter.x /= pwi->usFormWidth;
   pGrp->ptlfCenter.y = (float)(rcl->yBottom + (rcl->yTop - rcl->yBottom)/2);
   pGrp->ptlfCenter.y /= pwi->usFormHeight;



   usWidth  = (USHORT)(rcl->xRight - rcl->xLeft);
   usHeight = (USHORT)(rcl->yTop   - rcl->yBottom);

   pwi->fOffx = pGrp->rclf.xLeft   * pwi->usFormWidth;
   pwi->fOffy = pGrp->rclf.yBottom * pwi->usFormHeight;

   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;

   pObj = pObj->NextChild;

   while (pObj && pObj->usClass != CLS_GROUPEND )
   {
      ObjMoveOutLine(pObj,pwi,0,0);
      pObj = pObj->Next;
   }

   pwi->usFormWidth = usOldWidth;
   pwi->usFormHeight= usOldHeight;
   pwi->fOffx = fOldOffx;
   pwi->fOffy = fOldOffy;

}
/*------------------------------------------------------------------------*/
/*  Name: GroupMoveOutline.                                               */
/*                                                                        */
/*  Description : Called during the mousemove message. Shows the outline  */
/*                of all objects in the group.                            */
/*                                                                        */
/*  Called from : Drwutl.c                                                */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void GroupMoveOutline(POBJECT pObj,WINDOWINFO *pwi,SHORT dx,SHORT dy)
{
   pGroup pGrp     = (pGroup)pObj;
   
   USHORT usOldWidth,usOldHeight;
   USHORT usWidth,usHeight;
   float  fOldOffx,fOldOffy;
   RECTL  rcl;

   usOldWidth  = pwi->usFormWidth;
   usOldHeight = pwi->usFormHeight;
   fOldOffx    = pwi->fOffx;
   fOldOffy    = pwi->fOffy;

   GroupOutLine(pGrp,&rcl,pwi);

   usWidth  = (USHORT)(rcl.xRight - rcl.xLeft);
   usHeight = (USHORT)(rcl.yTop   - rcl.yBottom);

   if (!(LONG)pwi->fOffx || !(LONG)pwi->fOffy)
   {
      pwi->fOffx = pGrp->rclf.xLeft   * pwi->usFormWidth;
      pwi->fOffy = pGrp->rclf.yBottom * pwi->usFormHeight;

      pwi->fOffx += (float)dx;
      pwi->fOffy += (float)dy;
   }
   else
   {
      pwi->fOffx += pGrp->rclf.xLeft   * pwi->usFormWidth;
      pwi->fOffy += pGrp->rclf.yBottom * pwi->usFormHeight;
   }

   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;

   /*
   ** Go one down in chain...
   */
   pObj = pObj->NextChild;

   do
   {
      ObjMoveOutLine(pObj,pwi,0,0);
      pObj = pObj->Next;
   }while(pObj);   

   pwi->usFormWidth = usOldWidth;
   pwi->usFormHeight= usOldHeight;
   pwi->fOffx = fOldOffx;
   pwi->fOffy = fOldOffy;
}
/*------------------------------------------------------------------------*/
/* GrpGetCenter.                                                          */
/*                                                                        */
/* Returns     :  NONE.                                                   */
/*------------------------------------------------------------------------*/
BOOL GrpGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl)
{
   pGroup pGrp     = (pGroup)pObj;
   POBJECT p;

   if (!pObj) return FALSE;

   /*
   ** Groups in groups makes it impossible to rotate. Code
   ** is not ready for it.
   */
   p = pObj;
   p = p->NextChild;
   do
   {
      if (p && p->usClass == CLS_GROUP)
         return FALSE;
      p = p->Next;
   }while(p);
   

   ptl->x = (LONG)(pGrp->ptlfCenter.x * pwi->usFormWidth);
   ptl->y = (LONG)(pGrp->ptlfCenter.y * pwi->usFormHeight);
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* GrpSetCenter.                                                          */
/*                                                                        */
/* Returns     :  NONE.                                                   */
/*------------------------------------------------------------------------*/
BOOL GrpSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl)
{
   pGroup pGrp     = (pGroup)pObj;

   if (!pObj) return FALSE;

   pGrp->ptlfCenter.x = (float)ptl->x; 
   pGrp->ptlfCenter.x /= pwi->usFormWidth;
   pGrp->ptlfCenter.y = (float)ptl->y;
   pGrp->ptlfCenter.y /= pwi->usFormHeight;

   return TRUE;
}
/*------------------------------------------------------------------------*/
/* GrpPtrAboveCenter.                                                     */
/*                                                                        */
/* Returns     :  BOOL TRUE if mousepointer is above the center.          */
/*------------------------------------------------------------------------*/
BOOL GrpPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse)
{
   pGroup pGrp     = (pGroup)pObj;
   RECTL rcl;

   if (!pObj) return FALSE;

   rcl.xLeft = (LONG)(pGrp->ptlfCenter.x * pwi->usFormWidth);
   rcl.xLeft  -= HANDLESIZE;
   rcl.xRight = rcl.xLeft + (HANDLESIZE * 2);

   rcl.yBottom = (LONG)(pGrp->ptlfCenter.y * pwi->usFormHeight);
   rcl.yBottom -= HANDLESIZE;
   rcl.yTop  = rcl.yBottom + (HANDLESIZE * 2);
   return WinPtInRect((HAB)0,&rcl,&ptlMouse);
}

void GrpDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg,POINTL *pt)
{
   RECTL rcl;
   pGroup pGrp = (pGroup)pObj;
   USHORT usOldWidth,usOldHeight;
   USHORT usWidth,usHeight;
   POINTL ptCenter;
   float  fOldOffx,fOldOffy;

   usOldWidth  = pwi->usFormWidth;
   usOldHeight = pwi->usFormHeight;
   fOldOffx    = pwi->fOffx;
   fOldOffy    = pwi->fOffy;

   if (!pt)
   {
      ptCenter.x = (LONG)(pGrp->ptlfCenter.x * pwi->usFormWidth);
      ptCenter.y = (LONG)(pGrp->ptlfCenter.y * pwi->usFormHeight);
   }
   else
      ptCenter = *pt;

   GroupOutLine(pGrp, &rcl,pwi);

   usWidth  = (USHORT)(rcl.xRight - rcl.xLeft);
   usHeight = (USHORT)(rcl.yTop   - rcl.yBottom);

   pwi->fOffx += pGrp->rclf.xLeft   * pwi->usFormWidth;
   pwi->fOffy += pGrp->rclf.yBottom * pwi->usFormHeight;

   pwi->usFormWidth   = usWidth;
   pwi->usFormHeight  = usHeight;

   if (pObj->NextChild)
   {
      /*
      ** Go one line down in the chain.
      */
      pObj = pObj->NextChild;
      do
      {
         ObjDrawRotate(pwi,pObj,lRotate,ulMsg,&ptCenter);
         pObj = pObj->Next;
      }while (pObj);
   }

   pwi->usFormWidth = usOldWidth;
   pwi->usFormHeight= usOldHeight;
   pwi->fOffx = fOldOffx;
   pwi->fOffy = fOldOffy;

   return;
}
/*--------------------------------------------------------------------------*/
static void copyChain(POBJECT pOrgChain,POBJECT pNewGrp)
{
   POBJECT pCopy;
   POBJECT pTmp,previous;

   pNewGrp->NextChild = makeCopy(pOrgChain);
   pNewGrp->Next      = NULL;
   pNewGrp->Previous  = NULL;

   pTmp              = pNewGrp->NextChild;
   pTmp->Next        = NULL;
   pTmp->Previous    = NULL;
   pTmp->PrevChild   = pNewGrp;

   pOrgChain = pOrgChain->Next;

   while (pOrgChain)
   {
      pCopy          = makeCopy(pOrgChain);
      pTmp->Next     = pCopy;
      previous       = pTmp;
      pTmp           = pTmp->Next;
      pTmp->Previous = previous;
      pOrgChain      = pOrgChain->Next;
   }
}
/*---------------------------------------------------------------------------*/
/* copyGroupObj                                                              */
/*---------------------------------------------------------------------------*/
POBJECT copyGroup(POBJECT pOrg)
{
   POBJECT pCopy;

   if (!pOrg->NextChild)
      return NULL;

   pCopy =  pObjNew(NULL,pOrg->usClass);
   memcpy(pCopy,pOrg,ObjGetSize(CLS_GROUP));

   copyChain(pOrg->NextChild,pCopy);
   return pCopy;
}
/*-----------------------------------------------[ public ]------------------*/
BOOL canPackGroup(POBJECT pObj)
{
   POBJECT p;

   if (pObj->usClass != CLS_GROUP)
      return FALSE;
   /*
   ** Groups in groups cannot be packed to one layer.
   ** Code is not ready for it.
   */
   p = pObj;
   p = p->NextChild;
   do
   {
      if (p && p->usClass == CLS_GROUP)
         return FALSE;
      p = p->Next;
   }while(p);
   
   return TRUE;
}
/*---------------------------------------------------------------------------*/
USHORT getGroupLayer( POBJECT pObj )
{
    pGroup pGrp = (pGroup)pObj;
    return pGrp->uslayer;    
}
/*-----------------------------------------------[ private ]-----------------*/

void  setGroupLayer( POBJECT pObj, USHORT usLayer )
{
    if (allElementsOnOneLayer(pObj))
       changeGroupLayer(pObj,usLayer);
    else
       sortGroupElements(pObj,usLayer);       
}
