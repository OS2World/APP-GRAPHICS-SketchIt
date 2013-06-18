/*------------------------------------------------------------------------*/
/*  Name: dlg_sel.c                                                       */
/*                                                                        */
/* Functions:                                                             */
/* ConvPts2Rect   - converts two points to a rectangle structure.         */
/* mapSelTypeOpmode - Maps a selection type to an operation mode.         */
/* changeMode     - Changes the selection mode in the windowinfo struct   */
/* isSingleSelect - Returns true is a multiselection is active (drwmenu)  */
/*                                                                        */
/* Private functions:                                                     */
/*    stopSelection    Handles the unselection.                           */
/*------------------------------------------------------------------------*/
#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_sel.h"
#include "dlg_sqr.h"
#include "dlg_cir.h"
#include "dlg_file.h"
#include "dlg_txt.h"
#include "dlg_lin.h"
#include "drwtxtin.h"
#include "drwsplin.h"
#include "drwutl.h"
#include "drwbox.h"
#include "drwgrp.h"
#include "drwbtext.h"
#include "resource.h"

#define SQUAREROOM    20
#define TEXT          107
#define MAXSELECTED   100

#define ID_TIMER       1    /*Timer ID used for freestyle lines*/

struct
   {
   short iClass;
   short iOpmode;
   } selOpmode[] = { { CLS_TXT,   TEXTSELECT   },
                     { CLS_CIR,   CIRSELECT    },
                     { CLS_SQR,   SQRSELECT    },
                     { CLS_SPLINE,SPLINESELECT },
                     { CLS_LIN,   LINESELECT   },
                     { CLS_IMG,   IMGSELECT    },
                     { CLS_META,  METAPICSELECT},
                     { CLS_GROUP, GROUPSELECT  },
                     { CLS_BLOCKTEXT,BLOCKTEXTSEL},
                     { CLS_UNKNOWN, NOSELECT   }};
/*------------------------------------------------------------------------*/
/* isSingleSelection.                                                     */
/*                                                                        */
/* Description : Returns true when a Singleselection is active. Used at   */
/*               the initialisation of attribute-submenus.                */
/*               Used in:drwmenu.c                                        */
/*                                                                        */
/* Returns     : BOOL.                                                    */
/*------------------------------------------------------------------------*/
BOOL isSingleSelection(WINDOWINFO *pwi)
{
   if (pwi->op_mode == GROUPSELECT || pwi->op_mode == MULTISELECT )
      return FALSE;
   else
      return TRUE;
 }
/*------------------------------------------------------------------------*/
/* mapSelTypeOpmode.                                                      */
/*                                                                        */
/* Description : mappes a selected object to a selection mode in the main */
/*               module.                                                  */
/*------------------------------------------------------------------------*/
int mapSelTypeOpmode(int iClass)
{
   int i = 0;

   do
   {
      if (selOpmode[i].iClass == iClass)
         return selOpmode[i].iOpmode;
      i++;
   } while (selOpmode[i].iClass != CLS_UNKNOWN);

   return (int)CLS_UNKNOWN;
}
/*------------------------------------------------------------------------*/
/* stopSelection.                                                         */
/*                                                                        */
/* Description : Stops the selection while going to another mode.         */
/*------------------------------------------------------------------------*/
void stopSelection(WINDOWINFO *pwi)
{
   switch(pwi->op_mode)
   {
      case TEXTSELECT:
      case IMGSELECT:
      case CIRSELECT:
      case SQRSELECT:
      case LINESELECT:
      case METAPICSELECT:
      case SPLINESELECT:
      case GROUPSELECT:
      case BLOCKTEXTSEL:
         if (pwi->pvCurrent)
            showSelectHandles(pwi->pvCurrent,pwi);
         break;
   }
   pwi->pvCurrent = NULL;
}
/*-----------------------------------------------[ public ]---------------*/
/* changeMode.                                                            */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT changeMode(WINDOWINFO *pwi,USHORT usMode, ULONG ulExtra)
{
   GRADIENTL gradl = {100,0};
   POINTL    ptl;

   switch(pwi->op_mode)
   {
      case MULTISELECT:
         ObjMultiUnSelect();
         break;
     case MULTIMOVE:
         ObjMultiUnSelect();
         break;
     case MULTICOPY:
         ObjMultiUnSelect();
         break;
      case TEXTINPUT:
         WinShowCursor(pwi->hwndClient,FALSE);
         break;
      case FREESTYLE:
         WinStopTimer(hab,pwi->hwndClient,ID_TIMER);
         break;
   }

   pwi->bFileHasChanged  = TRUE; 

   switch (usMode)
   {
      case NOSELECT:
         stopSelection(pwi);
         pwi->op_mode   = NOSELECT;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,SPTR_ARROW,FALSE));
         break;
      case MULTISELECT:
         stopSelection(pwi);
         pwi->op_mode   = MULTISELECT;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,SPTR_ARROW,FALSE));
         break;

      case SQUAREDRAW:
         stopSelection(pwi);
         pwi->op_mode   = SQUAREDRAW;
         break;
      case TEXTINPUT:
         stopSelection(pwi);
         pwi->op_mode         = TEXTINPUT;
         Createcursor(pwi);   /*drwtxtin.c*/
         ptl.x                = 0;
         ptl.y                = 0;
         GpiSetCurrentPosition(pwi->hps,&ptl);
         GpiSetCharAngle(pwi->hps,&gradl);
         break;
      case REGPOLYDRAW:
         stopSelection(pwi);
         pwi->op_mode         = REGPOLYDRAW;
         StartupPolyDlg(pwi);
         break;
      case SPLINEDRAW:
         stopSelection(pwi);
         pwi->op_mode         = SPLINEDRAW;
         SplineSetup(ulExtra);
         break;
      case FREESTYLE:
         stopSelection(pwi);
         pwi->op_mode         = FREESTYLE;
         SplineSetup(ulExtra);
         break;
      case CIRCLEDRAW:
         stopSelection(pwi);
         pwi->op_mode         = CIRCLEDRAW;
         pwi->fxArc   = MAKEFIXED(360,0); /* full arc????*/
         break;
      case REGPOLYSTAR:
         stopSelection(pwi);
         pwi->op_mode         = REGPOLYSTAR;
         StartupPolyDlg(pwi);
         break;
      default:                /* Something selected?*/
         pwi->op_mode         = usMode;
         break;
   }
   WinSetFocus(HWND_DESKTOP,pwi->hwndClient); /* Set focus on drawing window*/
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/* Dialog procedure for the object dialog. Which makes it possible to     */
/* change the atributes of the selected item.                             */
/*                                                                        */
/* Attributes are:   Shade, Rotate, Color (filling-,border-,shade color)  */
/* Displays : The selected object which can be a circle,ellipse or square */
/* Specials : The rotate slider will be disabled when the selected item   */
/*            is a circle, which means the radius in the x direction is   */
/*            the same as the radius in the y-direction.                  */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ObjectDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   ULONG  ulLayer;
   ULONG  ulStorage[2];    /* To get the vals out of the spins  */
   PVOID  pStorage;
   SWP    swp;
   WINDOWINFO *pwi;
   static POBJECT pObj;
   static PBLOCKTEXT pT;
   static pGroup pGrp;

   switch (msg)
   {
      case WM_INITDLG:
         pwi = (WINDOWINFO *)mp2;

         /* Centre dialog on the screen	*/

         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         if (!pwi)
            return (MRESULT)0;

         pT   = NULL;
         pObj = pwi->pvCurrent;

         if (pObj->usClass == CLS_BLOCKTEXT)
         {
            WinSetWindowText(hwnd,"Detail BlockText Object");
            pT = (PBLOCKTEXT)pObj;
            pGrp = NULL;
            ulLayer = (ULONG)pT->bt.usLayer;
         }
         else if (pObj->usClass == CLS_GROUP)
         {
            WinSetWindowText(hwnd,"Detail Group Object");
            pGrp = (pGroup)pObj;
            pT   = NULL;
            ulLayer = (ULONG)pGrp->uslayer;
         }

        /* Set the layer spinbutton */

        WinSendDlgItemMsg( hwnd, 864, SPBM_SETLIMITS,
                           MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

        WinSendDlgItemMsg( hwnd, 864, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)ulLayer), NULL);
        return 0;

      case WM_COMMAND:
	   switch(LOUSHORT(mp1))
	   {
              case DID_OK:
                 pStorage = (PVOID)ulStorage;
                 WinSendDlgItemMsg( hwnd,864,SPBM_QUERYVALUE,
                                    (MPARAM)(pStorage),MPFROM2SHORT(0,0));
                 if (pObj->usClass== CLS_BLOCKTEXT)
                    pT->bt.usLayer  = (USHORT)ulStorage[0];
                 else if (pObj->usClass == CLS_GROUP)
                    setGroupLayer(pObj,(USHORT)ulStorage[0]);
                 WinDismissDlg(hwnd,TRUE);
                 return 0;
              case DID_CANCEL:
                 WinDismissDlg(hwnd,TRUE);
                 return 0;
              case DID_HELP:
                 return 0;
           }
           return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
VOID DragBox(HPS hps, POINTL *pptlStart, POINTL *pptlEnd, SHORT dx, SHORT dy, USHORT mode)
{
   static POINTL ptlE;
   static POINTL ptlS;

   POINTL ptlEnd;
   POINTL ptlStart;

   ULONG lSaveType;

  switch (mode)
  {
     case DRAG_START:
        ptlEnd.x = ptlE.x = pptlEnd->x;
        ptlEnd.y = ptlE.y = pptlEnd->y;
        ptlStart.x = ptlS.x = pptlStart->x;
        ptlStart.y = ptlS.y = pptlStart->y;
        break;
     case DRAG_MOVE:
        ptlEnd.x = ptlE.x + dx;
        ptlEnd.y = ptlE.y + dy;
        ptlStart.x = ptlS.x + dx;
        ptlStart.y = ptlS.y + dy;
        break;
  }
  lSaveType = GpiQueryLineType(hps);
  GpiSetLineType(hps, LINETYPE_DOT);
  GpiSetMix(hps,FM_INVERT);
  GpiMove(hps, &ptlStart);
  GpiBox(hps,DRO_OUTLINE,&ptlEnd,0L,0L);
  GpiSetLineType(hps, lSaveType);
}
/*------------------------------------------------------------------------*/
VOID SelectBox(HPS hps, POINTL *pptlStart, POINTL *pptlEnd, SHORT dx, SHORT dy, USHORT mode)
{
   static POINTL ptlE;
   static POINTL ptlS;

   POINTL ptlEnd;
   POINTL ptlStart;

  switch (mode)
  {
     case DRAG_START:
        ptlEnd.x = ptlE.x = pptlEnd->x;
        ptlEnd.y = ptlE.y = pptlEnd->y;
        ptlStart.x = ptlS.x = pptlStart->x;
        ptlStart.y = ptlS.y = pptlStart->y;
        break;
     case DRAG_MOVE:
        ptlEnd.x = ptlE.x + dx;
        ptlEnd.y = ptlE.y + dy;
        ptlStart.x = ptlS.x + dx;
        ptlStart.y = ptlS.y + dy;
        break;
  }
  GpiSetMix(hps,FM_INVERT);
  DrawHandle(hps,ptlStart.x,ptlStart.y); /* top left     */
  DrawHandle(hps,ptlStart.x,ptlEnd.y);   /* bottom left  */
  DrawHandle(hps,ptlEnd.x,ptlStart.y);   /* top right    */
  DrawHandle(hps,ptlEnd.x,ptlEnd.y);     /* bottom right */
}
/*------------------------------------------------------------------------*/
/*  ConvPts2Rect                                                          */
/*                                                                        */
/*  Description : Converts two points of an rectangular area to an        */
/*                rectl structure.                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ConvPts2Rect(POINTL ptlStart, POINTL ptlEnd, RECTL *rcl)
{

   if (ptlStart.x <= ptlEnd.x)
   {
      rcl->xLeft = ptlStart.x;
      rcl->xRight= ptlEnd.x;
   }
   else
   {
      rcl->xLeft = ptlEnd.x;
      rcl->xRight= ptlStart.x;
   }

   if (ptlStart.y <= ptlEnd.y)
   {
      rcl->yBottom = ptlStart.y;
      rcl->yTop  = ptlEnd.y;
   }
   else
   {
      rcl->yBottom = ptlEnd.y;
      rcl->yTop  = ptlStart.y;
   }

}
