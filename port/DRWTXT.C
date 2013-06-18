/*------------------------------------------------------------------------*/
/*  Name: drwtxt.c                                                        */
/*                                                                        */
/*  date : 17 sept 1995 initial version.                                  */
/*                                                                        */
/*  Description : Contains all functions for handling text displayed      */
/*                along curved lines like circles and ellipses.           */
/*                                                                        */
/*  Functions  :                                                          */
/*     DisplayRotate : Displays the txt obj along a circular line.        */
/*     TxtCirDlgProc : Dialog proc for the circular text.                 */
/*     TxtCirWndProc : Windowproc for the result window in the dialog.    */
/*  Private    :                                                          */
/*     InitializeSlider : Initializes the sliders for circular text.      */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS

#define PI 3.1415926
#define ID_PATH 1L

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_sqr.h"
#include "dlg_txt.h"
#include "dlg_fnt.h"
#include "dlg_hlp.h"
#include "drwutl.h"
#include "resource.h"

/*
** define a static which we use to put in our original
** from the textstructure. If the user click's on OK
** the values are copied back to the txtstruct.
*/
static WINDOWINFO *pwi;
static POBJECT    pObj;
static Textstruct pTxt;
static BOOL       bDialog;

#define MAX_SWEEPANGLE 360
#define MIN_SWEEPANGLE   5

#define MAX_STARTANGLE 360
#define MIN_STARTANGLE   0

static  GRADIENTL resetgradl = {100,0}; /* 0 degrees for reset   */

/*------------------------------------------------------------------------*/
/*  DisplayRotate                                                         */
/*                                                                        */
/*  Description : Displays the text along a circular line.                */
/*                It start on the circle at a starting point in degrees   */
/*                and walks along the circular outline the number of      */
/*                degrees defined in the sweepangle.                      */
/*                                                                        */
/* Parameters   : HPS hps   presentation space, which can be linked with  */
/*                          any device context.                           */
/*                WINDOWINFO *                                            */
/*                          Our window info which can of any device       */
/*                          or any window (like print preview window,     */
/*                          printer etc).                                 */
/*                Textstruct                                              */
/*                          Pointer to the textobject.                    */
/*------------------------------------------------------------------------*/
VOID DisplayRotate (HPS hps, WINDOWINFO *pwi, Textstruct pText, BOOL bShade)
{
   static CHAR *szText;
   static LONG cbText;
   static LONG alWidthTable[256];
   double      ang, angCharWidth, angChar,angStart;
   double      sweep; /* sweep angle in pi radialen */
   double      start; /* start angle in pi radialen */
   FONTMETRICS fm;
   GRADIENTL   gradl;
   INT         iChar;
   LONG        lRadius, lTotWidth, lCharRadius;
   POINTL      ptl;
   RECTL       rcl;

   LONG cxClient,cyClient;

   szText = pText->Str;
   cbText = strlen(szText);

   sweep = ((2 * PI)/360 ) * pText->TxtCircular.ulSweep;
   start = ((2 * PI)/360 ) * pText->TxtCircular.ulStart;

   GpiQueryFontMetrics (hps, (LONG) sizeof fm, &fm) ;

   if (bDialog)
   {
      /*
      ** OK we are drawing in the window of the dialog.
      */
      rcl.xLeft   = (LONG)pTxt->TxtCircular.rclf.xLeft;
      rcl.yBottom = (LONG)pTxt->TxtCircular.rclf.yBottom;
      rcl.yTop    = (LONG)pTxt->TxtCircular.rclf.yTop;
      rcl.xRight  = (LONG)pTxt->TxtCircular.rclf.xRight;
   }
   else
   {
      // Find circle dimensions and scale font
      TextOutLine((POBJECT)pText,&rcl,pwi,FALSE);
   }

   lRadius = max((rcl.xRight - rcl.xLeft ),(rcl.yTop - rcl.yBottom))/2;
   cxClient= rcl.xRight - rcl.xLeft;
   cyClient= rcl.yTop - rcl.yBottom;
   /*
   ** Text should stay in it's box.
   */
   lRadius -= fm.lMaxAscender;

   // Obtain width table and total width

   GpiQueryWidthTable (hps, 0L, 256L, alWidthTable) ;

   for (lTotWidth = 0, iChar = 0 ; iChar < (INT) cbText ; iChar ++)
        lTotWidth += alWidthTable [szText [iChar]] ;

   ang = start; //sweep / 2 ;      // Initial angle for first character

   if (pText->lShadeX && bShade)
   {
      GpiSetColor(hps,pText->bt.ShadeColor);
      angStart = ang;
      angStart -= (((2 * PI)/360 ) * pText->lShadeX)/2;

      for (iChar = 0 ; iChar < (INT) cbText ; iChar++)
      {
         angCharWidth = sweep * alWidthTable [szText [iChar]] / lTotWidth ;
         gradl.x = (LONG) (lRadius * cos (angStart - angCharWidth / 2 - PI / 2));
         gradl.y = (LONG) (lRadius * sin (angStart - angCharWidth / 2 - PI / 2));

         GpiSetCharAngle (hps, &gradl);

         // Find position for character and display it

         angChar = atan2 ((double) alWidthTable [szText [iChar]] / 2,
                          (double) lRadius) ;

         lCharRadius = (LONG) (lRadius / cos (angChar)) ;
         angChar += angStart - angCharWidth / 2 ;

         ptl.x = (LONG)rcl.xLeft + (cxClient / 2 + lCharRadius * cos (angChar)) ;
         ptl.y = (LONG)rcl.yBottom + (cyClient / 2 + lCharRadius * sin (angChar)) ;

         GpiCharStringAt (hps, &ptl, 1L, szText + iChar) ;
         angStart -= angCharWidth ;
      }
      lRadius -=pText->lShadeX * pwi->uXfactor;
      GpiSetCharAngle (hps, &resetgradl);
      return; /* Yes return you are called twice if there is shade see dlg_txt*/
   }

   GpiSetColor(hps,pText->bt.fColor);
   for (iChar = 0 ; iChar < (INT) cbText ; iChar++)
   {
                              // Set character angle
      angCharWidth = sweep * alWidthTable [szText [iChar]] / lTotWidth ;
      gradl.x = (LONG) (lRadius * cos (ang - angCharWidth / 2 - PI / 2)) ;
      gradl.y = (LONG) (lRadius * sin (ang - angCharWidth / 2 - PI / 2)) ;

      GpiSetCharAngle (hps, &gradl) ;

      // Find position for character and display it

      angChar = atan2 ((double) alWidthTable [szText [iChar]] / 2,
                       (double) lRadius) ;

      lCharRadius = (LONG) (lRadius / cos (angChar)) ;
      angChar += ang - angCharWidth / 2 ;

      ptl.x = (LONG)rcl.xLeft + (cxClient / 2 + lCharRadius * cos (angChar)) ;
      ptl.y = (LONG)rcl.yBottom + (cyClient / 2 + lCharRadius * sin (angChar)) ;

      GpiCharStringAt (hps, &ptl, 1L, szText + iChar) ;

      ang -= angCharWidth ;
    }
    GpiSetCharAngle (hps, &resetgradl);
}
/*------------------------------------------------------------------------*/
/*  SetupOutline                                                          */
/*                                                                        */
/*  Description : This  function sets up the box in paper coords where    */
/*                the circular text lives in. So this is done using the   */
/*                old textoutline and after the calculation is done the   */
/*                ulState flag is set to TXT_CIRCULAR (!!!!).             */
/*                                                                        */
/*                Called as soon the user sets the text to cicular and the*/
/*                ulState flag was NOT TXT_CIRCULAR.                      */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID TxtSetupOutline(Textstruct pTxt, WINDOWINFO *pwi)
{
   RECTL rcl;
   LONG  ulSize,yPos;
   if (!pTxt)
      return;
   /*
   ** May not be called if the state is already
   ** TXT_CIRCULAR.
   */
   if ((pTxt->ulState & TXT_CIRCULAR))
      return;
   /*
   ** 1. Get old outline in window coords.
   ** 2. Translate the to paper coords.
   ** 3. Save in the txtobject struct.
   ** 4. Set the stateflag to.... TXT_CIRCULAR.
   */
   TextOutLine((POBJECT)pTxt,&rcl,pwi,FALSE);
   /*
   ** Store the square in the
   ** box structure and make the box square!!
   ** Use the bigest size to make it square.
   */
   ulSize = max((rcl.xRight - rcl.xLeft),(rcl.yTop - rcl.yBottom));
   yPos = (LONG)(pTxt->ptl.y * pwi->usFormHeight);

   rcl.yBottom = yPos - (ulSize / 2);
   rcl.xRight = rcl.xLeft + ulSize;
   rcl.yTop   = rcl.yBottom + ulSize;

   pTxt->TxtCircular.rclf.xLeft   = (float)rcl.xLeft;
   pTxt->TxtCircular.rclf.yBottom = (float)rcl.yBottom;
   pTxt->TxtCircular.rclf.yTop    = (float)rcl.yTop;
   pTxt->TxtCircular.rclf.xRight  = (float)rcl.xRight;
   /*
   ** Normalize positions...
   */
   pTxt->TxtCircular.rclf.xLeft   /= (float)pwi->usFormWidth;
   pTxt->TxtCircular.rclf.yBottom /= (float)pwi->usFormHeight;
   pTxt->TxtCircular.rclf.yTop    /= (float)pwi->usFormHeight;
   pTxt->TxtCircular.rclf.xRight  /= (float)pwi->usFormWidth;

   pTxt->ulState |= TXT_CIRCULAR;
   /*
   ** bye bye..
   */
   return;
}
/*------------------------------------------------------------------------*/
/*  TxtCirWndProc.                                                        */
/*                                                                        */
/*  Description : Shows directly to the user the results of the changes   */
/*                in the circular text dialog.                            */
/*                of objects.                                             */
/*                                                                        */
/*  Parameters  : HWND  hwnd - windowhandle.                              */
/*                ULONG ulmsg- window message.                            */
/*                MPARAM mp1 - message param 1.                           */
/*                MPARAM mp2 - message param 2.                           */
/*                                                                        */
/* Returns     : MRESULT.                                                 */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY TxtCirWndProc(HWND hwnd, ULONG ulmsg, MPARAM mp1, MPARAM mp2)
{
   HPS hps;
   RECTL rcl;
   POINTL ptl;
   SIZEF sizefx;

   switch (ulmsg)
   {
      case WM_PAINT:
         WinQueryWindowRect(hwnd, &rcl);
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiCreateLogColorTable ( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         WinFillRect(hps,&rcl,pwi->lBackClr);
         ptl.x = (LONG)(pTxt->ptl.x);
         ptl.y = (LONG)(pTxt->ptl.y);
         GpiMove(hps,&ptl);
         GpiSetColor(hps,pTxt->bt.fColor);
         sizefx.cx = MAKEFIXED(12,0);
         sizefx.cy = MAKEFIXED(12,0);
         if (!strlen(pTxt->fattrs.szFacename))
            FontInit(hps,"Courier",FONT_SET,&sizefx);
         else
            FontInit(hps,pTxt->fattrs.szFacename,FONT_SET,&sizefx);
         GpiSetCharBox(hps,&sizefx);

         DisplayRotate(hps,pwi,pTxt,FALSE);

         GpiSetCharSet (hps, LCID_DEFAULT) ;     // Clean up
         GpiDeleteSetId (hps, 2L);

         WinEndPaint(hps);
         return 0;

   }
   return WinDefWindowProc (hwnd,ulmsg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/*  TxtCirDlgProc.                                                        */
/*                                                                        */
/*  Description : Dialog procedure for handling the circular text def.    */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY TxtCirDlgProc(HWND hwnd,ULONG ulMsg,MPARAM mp1,MPARAM mp2)
{
   static HWND HwndCir;
   static ULONG ulState;
   SWP swp;		   /* Screen Window Position Holder   */
   POBJECT pTmp;
   Textstruct pTxttmp;
   RECTL  rcl;
   static BOOL bInit;

   switch (ulMsg)
   {
      case WM_INITDLG:
         bDialog = TRUE;
         bInit   = TRUE;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         pwi  = (WINDOWINFO *)mp2;
         pTmp = (POBJECT)pwi->pvCurrent;

         /*
         ** Make a copy of the original structure since we
         ** need to update coords and size to display in
         ** the dialog window.
         */
         pObj =(POBJECT)calloc(sizeof(Text),1);
         memcpy(pObj,pTmp,sizeof(Text));
         pTxt    = (Textstruct)pObj;

         /*
         ** Setup the check box.
         */

         if (pTxt->ulState & TXT_CIRCULAR)
            WinSendDlgItemMsg(hwnd,ID_CHKCIRCULAR,BM_SETCHECK,
                              (MPARAM)1,(MPARAM)0);
         else
            WinSendDlgItemMsg(hwnd,ID_CHKCIRCULAR,BM_SETCHECK,
                              (MPARAM)0,(MPARAM)0);


         HwndCir = WinWindowFromID(hwnd,ID_TXTCIRWIN);
         /*
         ** Here we go, get the rect of the display window in
         ** define the new position of the string.
         */
         WinQueryWindowRect(HwndCir, &rcl);
         pTxt->ptl.x = (float) ( rcl.xRight / 2);
         pTxt->ptl.y = (float) ( rcl.yTop   / 2);

         pTxt->TxtCircular.rclf.xLeft   = (float)rcl.xLeft   + 10;
         pTxt->TxtCircular.rclf.yBottom = (float)rcl.yBottom + 10;
         pTxt->TxtCircular.rclf.yTop    = (float)rcl.yTop    - 10;
         pTxt->TxtCircular.rclf.xRight  = (float)rcl.xRight  - 10;
         /*
         ** Here we have to unset the state since displayrotate
         ** calls the textoutline function. Textoutline does
         ** a recalculation from papervalues to window values
         ** if the TXT_CIRCULAR bit is on. So switch it off
         ** and use a static local for further work.
         */
         ulState = pTxt->ulState;

         pTxt->ulState &= ~TXT_CIRCULAR;

         /* SweepAngle*/
         WinSendDlgItemMsg( hwnd, ID_SPNSWEEP, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_SWEEPANGLE), MPFROMLONG(MIN_SWEEPANGLE));
         WinSendDlgItemMsg( hwnd, ID_SPNSWEEP, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)pTxt->TxtCircular.ulSweep), NULL);
         /* StartAngle*/
         WinSendDlgItemMsg( hwnd, ID_SPNSTART, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_STARTANGLE), MPFROMLONG(MIN_STARTANGLE));
         WinSendDlgItemMsg( hwnd, ID_SPNSTART, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)pTxt->TxtCircular.ulStart), NULL);
         bInit = FALSE;
         return (MRESULT)0;

      case WM_COMMAND:
	   switch(LOUSHORT(mp1))
	   {
              case DID_OK:
                 /*
                 ** Get the original and copy the circular
                 ** value's from the temp in to original.
                 */
                 pTxttmp = (Textstruct)pwi->pvCurrent;

                 if (ulState & TXT_CIRCULAR)
                    TxtSetupOutline((Textstruct)pwi->pvCurrent,pwi);
                 else
                 {
                    /*
                    ** Set up outline so we get do an wininvalidate
                    ** rect in the main mod if we get the UM_ENDDIALOG
                    ** And finally switch the circular bit off.
                    */
                    pTxttmp->ulState &= ~TXT_CIRCULAR;
                 }
                 pTxttmp->TxtCircular.ulSweep = pTxt->TxtCircular.ulSweep;
                 pTxttmp->TxtCircular.ulStart = pTxt->TxtCircular.ulStart;
                 /*
                 ** Draw it in the dark so we get a new rectangle calculation.
                 */
                 WinPostMsg(pwi->hwndClient,UM_ENDDIALOG,(MPARAM)pTxttmp,(MPARAM)0);
                 free(pObj);
                 bDialog = FALSE;
                 WinDismissDlg(hwnd,TRUE);
                 break;
              case DID_CANCEL:
                 free(pObj);
                 bDialog = FALSE;
                 WinDismissDlg(hwnd,TRUE);
                 break;
              case DID_HELP:
                 ShowDlgHelp(hwnd);
                 return 0;
           }
           return (MRESULT)0;

     case WM_CONTROL:
        switch (LOUSHORT(mp1))
        {

           case ID_CHKCIRCULAR:
              if (!(ulState & TXT_CIRCULAR))
              {
                 ulState |= TXT_CIRCULAR;
                 WinSendDlgItemMsg(hwnd,ID_CHKCIRCULAR,BM_SETCHECK,
                                   (MPARAM)1,(MPARAM)0);
              }
              else
              {
                 ulState &= ~TXT_CIRCULAR;
                 WinSendDlgItemMsg(hwnd,ID_CHKCIRCULAR,BM_SETCHECK,
                                   (MPARAM)0,(MPARAM)0);
              }
              return (MRESULT)0;
            
           /* SweepAngle */
           case ID_SPNSWEEP:
              if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
              {
                 WinSendDlgItemMsg(hwnd,ID_SPNSWEEP,SPBM_QUERYVALUE,
                                   (MPARAM)((VOID *)&pTxt->TxtCircular.ulSweep),MPFROM2SHORT(0,0));
                 if (pTxt->TxtCircular.ulSweep <= MIN_SWEEPANGLE)
                    pTxt->TxtCircular.ulSweep = MIN_SWEEPANGLE;
                 WinInvalidateRect (HwndCir,NULL,FALSE);
              }
              break;

           /* StartAngle */
           case ID_SPNSTART:
              if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
              {
                 WinSendDlgItemMsg(hwnd,ID_SPNSTART,SPBM_QUERYVALUE,
                                   (MPARAM)((VOID *)&pTxt->TxtCircular.ulStart),MPFROM2SHORT(0,0));
                 pTxt->TxtCircular.ulStart = min(pTxt->TxtCircular.ulStart,MAX_STARTANGLE);
                 WinInvalidateRect (HwndCir,NULL,FALSE);
              }
              break;
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, ulMsg, mp1, mp2));
}
