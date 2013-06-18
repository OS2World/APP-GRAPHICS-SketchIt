/*------------------------------------------------------------------------*/
/*  Name: dlg_txt.c                                                       */
/*                                                                        */
/*  Description : Functions for handling the textobjects.                 */
/*                                                                        */
/*  Functions  :                                                          */
/*  TextSelect     : returns a txt obj if the mouse point falls in a      */
/*                   txt element.                                         */
/*  DrawTextSegment: Draws a text element on a device (screen/printer etc)*/
/*  MoveTextSegment: Moves a given txt obj over an given amount of units  */
/*  OpenTextSegment: Creates a textelement in the main chain!             */
/*  TextDlgProc    : Property dialog for a given textsegment.             */
/*  TxtFlipHoriz   : Mirror the text.                                     */
/*  TextInvArea    : Calculates the inv area of the given textobj         */
/*  TextOutLine    : Outline is invarea minus selection corners.          */
/*  TextStretch    : Stretches the text into a given box.                 */
/*  Txtfountain    : Fountain fill change.                                */
/*  CloseTextSegment:Closes the textsegment.                              */
/*  RegisterTxt    : Registers the windowprocedures for the windows in the*/
/*                   used dialogs.                                        */
/*  TxtRemFromGroup: Remove the given object from the given group UNGROUP */
/*  TxtPutInGroup  : Insert the given object into the given group GROUP.  */
/*  isTxtOutlineSet: Returns true if an outline is drawn for this text.   */
/*  txtDrawOutline : [public] draws the text in plain vanilla.            */
/*  txtSetFont     : Set the font of selected obj.                        */
/*  txtEdit        : Edit text of selected text object.                   */
/*  setupWidthTable: Calculates the width-table of the text object.       */
/*                                                                        */
/*--Private functions-----------------------------------------------------*/
/*                                                                        */
/*  DrawTxtOutline : Draws a text element while moving or for boundary... */
/*  TxtCircStretch : Stretches the text when it has a circular shape.     */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define ID_PATH  1L
#define CHARCOLOR  4
#define SHADECLR   5
#define OUTLNCLR   6
#define MINCHARCY  8        /* Min point size cy in spinbuttoncontrol */
#define MINCHARCX  8        /* Min point size cx in spinbuttoncontrol */
#define MAXCHARCX  16000    /* Max point size cx in spinbuttoncontrol */
#define MAXCHARCY  16000    /* Max point size cy in spinbuttoncontrol */
#define VECTOR     100      /* Used to create a vector for rotation.  */
#define DEFAULTSTR "Draw"   /* default example text in object dlg     */
#define PI 3.1415926

#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_fnt.h"
#include "dlg_hlp.h"
#include "dlg_clr.h"
#include "dlg_txt.h"
#include "dlg_sqr.h"
#include "drwfount.h" /* fountain fill function prototypes */
#include "drwutl.h"
#include "resource.h"

#ifdef __IBMC__
#define itoa _itoa
#endif

extern ULONG   ColorTab[];       /* SEE dlg_val.c                    */
static LONG    sColor[8] ;       /* eerste vijf elem te gebruiken in comb met scrollbar*/
static LONG    lShade[2];        /* Shading in x and y direction     */
//static WINDOWINFO *pwi;          /* Used  windowproc.                */

typedef struct _defTxt
{
   PTEXT       pTxt;
   BOOL        bDialogActive;
   WINDOWINFO *pwi;
} defTxt;

static defTxt defText; /* Used in the dialog proc and window proc */

/*Scroll bar values */

extern HPS hpsClient;
/*
 * sColor[0] = vertical shading in 0.1 mm    (scroll bar controlled)
 * sColor[1] = horz     shading in 0.1 mm    (scroll bar controlled)
 * sColor[2] = Shear                         (scroll bar controlled)
 * sColor[3] = 0
 * sColor[4] = Character color (not realy used anymore)
 * sColor[5] = Shading color
 * sColor[6] = Outline color
 * sColor[7] = BackGround Color  --- NOT USED ---
 */

#define MAX_SHADING  30
#define MIN_SHADING -30
#define MAX_SHEAR    7
#define MIN_SHEAR    0



FONTMETRICS fmetrics; /* To give the value from the colorwinproc to the dlgproc*/

POINTL aptlShear[7] = { -100, 41, -100, 100,
                        -41, 100,    0, 100,
                         41, 100,  100, 100,
                         100,  41 } ;
HWND ClrHwnd;
/*----------------------------------------------[  public ]------------------*/
/* Name        : setupWidthTable.                                            */
/*                                                                           */
/* Description : Calculates the width table for the given text. The width    */
/*               table is later used printing to get real WYSIWYG.           */
/*               Function is called after each resize of the text            */
/*               and during the creation of a text object.                   */
/* Parameters  : POBJECT pObj     - reference to a text object.              */
/*               WINDOWINFO *pwi  - reference to the screen instance.        */
/*                                                                           */
/* Returns     : NONE.                                                       */
/*---------------------------------------------------------------------------*/
static void setupWidthTable(POBJECT pObj, WINDOWINFO *pwi)
{
   SIZEF      sizfx;
   Textstruct pText = (Textstruct)pObj;

   if (!pObj)
      return;

   sizfx.cx = pText->sizfx.fcx * pwi->usWidth;
   sizfx.cy = pText->sizfx.fcy * pwi->usHeight;
   setFont(pwi,&pText->fattrs,sizfx);
   GpiQueryWidthTable(pwi->hps,0,MAX_NRWIDTH,pObj->lWidth);
   return;
}
/*------------------------------------------------------------------------*/
/*  CalcTextClipArea.                                                     */
/*                                                                        */
/*  Description : Calculates the clipping rectangle for the text struct   */
/*                Used for printing etc.                                  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void CalcTextClipArea(HPS hps,Textstruct pText, WINDOWINFO *pwi, RECTL *prcl)
{
   PPOINTL aptl;           /* Pointer een array van punten */
   POINTL  nptl[5];        /* QueryTextbox needs 5 points  */
   POINTL  ptl;
   RECTL   rclInv;
   SIZEF   sizfx;
   aptl = nptl;            /* To give querytextbox space to work with  */

   if (!(pText->ulState & TXT_CIRCULAR))
   {
      ptl.x = (LONG)(pText->ptl.x * pwi->usFormWidth );
      ptl.y = (LONG)(pText->ptl.y * pwi->usFormHeight);
      /*
      ** Add the offset. Maybe we are part of a group.
      */
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;

      GpiSetCurrentPosition(hps,&ptl);
      GpiQueryTextBox(hps,(LONG)strlen(pText->Str),
                      pText->Str,TXTBOX_COUNT,aptl);

      /* first get the corner coordinates */


      rclInv.yBottom  = nptl[TXTBOX_BOTTOMLEFT].y;
      rclInv.xRight   = nptl[TXTBOX_BOTTOMRIGHT].x;

      /* search for the highest x value */

      if (rclInv.xRight < nptl[TXTBOX_BOTTOMLEFT].x )
         rclInv.xRight = nptl[TXTBOX_BOTTOMLEFT].x;
      if (rclInv.xRight < nptl[TXTBOX_TOPLEFT].x )
         rclInv.xRight = nptl[TXTBOX_TOPLEFT].x;
      if (rclInv.xRight < nptl[TXTBOX_TOPRIGHT].x )
         rclInv.xRight = nptl[TXTBOX_TOPRIGHT].x;

      /* search for the LOWEST x value */

      rclInv.xLeft    = ptl.x;

      if (rclInv.xLeft > nptl[TXTBOX_BOTTOMRIGHT].x )
         rclInv.xLeft = nptl[TXTBOX_BOTTOMRIGHT].x;
      if (rclInv.xLeft > nptl[TXTBOX_TOPLEFT].x )
         rclInv.xLeft = nptl[TXTBOX_TOPLEFT].x;
      if (rclInv.xLeft > nptl[TXTBOX_TOPRIGHT].x )
         rclInv.xLeft = nptl[TXTBOX_TOPRIGHT].x;

      /* search for the highest Y value */

      rclInv.yTop     = nptl[TXTBOX_TOPLEFT].y;

      if (rclInv.yTop < nptl[TXTBOX_BOTTOMRIGHT].y )
         rclInv.yTop = nptl[TXTBOX_BOTTOMRIGHT].y;
      if (rclInv.yTop < nptl[TXTBOX_BOTTOMLEFT].y )
         rclInv.yTop = nptl[TXTBOX_BOTTOMLEFT].y;
      if (rclInv.yTop < nptl[TXTBOX_TOPRIGHT].y )
         rclInv.yTop = nptl[TXTBOX_TOPRIGHT].y;

      /* search for the lowest Y value */

      rclInv.yBottom  = nptl[TXTBOX_BOTTOMLEFT].y;

      if (rclInv.yBottom > nptl[TXTBOX_BOTTOMRIGHT].y )
         rclInv.yBottom = nptl[TXTBOX_BOTTOMRIGHT].y;
      if (rclInv.yBottom > nptl[TXTBOX_TOPLEFT].y )
         rclInv.yBottom = nptl[TXTBOX_TOPLEFT].y;
      if (rclInv.yBottom > nptl[TXTBOX_TOPRIGHT].y )
         rclInv.yBottom = nptl[TXTBOX_TOPRIGHT].y;

      rclInv.xLeft     += ptl.x;
      rclInv.xRight    += ptl.x;
      rclInv.yTop      += ptl.y;
      rclInv.yBottom   += ptl.y;

      prcl->xLeft   = rclInv.xLeft;
      prcl->xRight  = rclInv.xRight;
      prcl->yTop    = rclInv.yTop;
      prcl->yBottom = rclInv.yBottom;
   }
   else
   {
      TextOutLine((POBJECT)pText,prcl,pwi,FALSE);
   }
   return;
}
/*-----------------------------------------------------------------------*/
Textstruct OpenTextSegment(POINTL ptlStart, WINDOWINFO *pwi)
{
   Textstruct  pText;
   POBJECT     pObj;

   pText = (Textstruct)pObjCreate(pwi,CLS_TXT);
//   pText->usSize = (USHORT)sizeof(Textstruct);
   pText->ptl.x =  (float) ptlStart.x;
   pText->ptl.y =  (float) ptlStart.y;
   pText->ptl.x /= (float)pwi->usFormWidth;
   pText->ptl.y /= (float)pwi->usFormHeight;

   pText->ustype   = CLS_TXT;
   /*
   ** Setup vector for 0 degrees
   */
   pText->gradl.x  = 100;
   pText->gradl.y  = 0;
   pText->fRotate  = (float)0;

   pText->TxtShear = 3;          /* Default no shear (see table on top) */
   pText->ulState  = 0;
//   strcpy(pText->facename,FontFaceName);
   /*
   ** Our paint and move routine....
   */
   pObj = (POBJECT)pText;
   pObj->paint			= DrawTextSegment;
   pObj->moveOutline	        = TxtMoveOutLine;
   pObj->getInvalidationArea    = TextInvArea;
   return pText;
}
/*------------------------------------------------------------------------*/
/* Function : CloseTextSegment                                            */
/* Creates a new node in the chain of segments so it can directly used by */
/* by the opentextsegent to fill in the coordinates. And this routine will*/
/* finally save the current font into the current segment before creating */
/* a new one. Set the general String index used in Dialog.c to zero       */
/*------------------------------------------------------------------------*/
Textstruct CloseTextSegment(Textstruct pT,char *InpString, WINDOWINFO *pwi)
{
   Textstruct pText;
   POINTL     ptl;
   RECTL      rcl;

   if (pT)
      pText = pT;
   else
      pText = (Textstruct)ObjCurrent();

   strcpy(pText->Str,InpString);
   pText->sizfx.fcx    = (float) pwi->sizfx.cx;
   pText->sizfx.fcy    = (float) pwi->sizfx.cy;

   pText->sizfx.fcx    /= (float)pwi->usFormWidth;
   pText->sizfx.fcy    /= (float)pwi->usFormHeight;

   pText->LineWidth       = pwi->lLnWidth;
   memcpy(&pText->fattrs,&pwi->fattrs,sizeof(FATTRS));
   /*
   ** Circular text.
   ** Full circle by default.
   ** Starting point at 0 degrees.
   */
   pText->TxtCircular.ulSweep  = 360;
   pText->TxtCircular.ulStart  = 180;
   /*
   ** Finally find out how big the rectl is where we live in on screen.
   ** And setup the rotation center.
   */
   TextOutLine((POBJECT)pText,&rcl,pwi,TRUE);

   ptl.x = rcl.xLeft + ((rcl.xRight - rcl.xLeft)/2);
   ptl.y = rcl.yBottom + ((rcl.yTop - rcl.yBottom)/2);

   pText->bt.ptlfCenter.x = (float)ptl.x;
   pText->bt.ptlfCenter.y = (float)ptl.y;
   pText->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
   pText->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
   setupWidthTable((POBJECT)pText,pwi);
   return pText;
}
/*------------------------------------------------------------------------*/
/* Move a Text segment.                                                   */
/*------------------------------------------------------------------------*/
void MoveTextSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy,fcx,fcy;
   Textstruct pTxt = (Textstruct)pObj;

   if (!pTxt)
      return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;

   pTxt->ptl.x += (fdx /fcx);
   pTxt->ptl.y += (fdy /fcy);

   if ((pTxt->ulState & TXT_CIRCULAR))
   {
      pTxt->TxtCircular.rclf.xLeft   += (float)(fdx /fcx);
      pTxt->TxtCircular.rclf.yBottom += (float)(fdy /fcy);
      pTxt->TxtCircular.rclf.yTop    += (float)(fdy /fcy);
      pTxt->TxtCircular.rclf.xRight  += (float)(fdx /fcx);
   }
   pTxt->bt.ptlfCenter.x += (fdx /fcx);
   pTxt->bt.ptlfCenter.y += (fdy /fcy);

   return;
}
/*----------------------------------------------------[ private ]---------*/
/*  DrawTxtOutline.                                                       */
/*                                                                        */
/*  Description : This functions draws the text in plain vanilla.         */
/*                used when text is moved/stretched  or for boundary      */
/*                calculations.                                           */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void DrawTxtOutline(WINDOWINFO *pwi,Textstruct pTxt)
{
   POINTL ptl;
   SIZEF  sizfx;

   if (!pTxt) return;

   if (!(pTxt->ulState & TXT_CIRCULAR))
   {
      sizfx.cx = pTxt->sizfx.fcx * pwi->usFormWidth;
      sizfx.cy = pTxt->sizfx.fcy * pwi->usFormHeight;
      setFont(pwi,&pTxt->fattrs,sizfx);

      GpiSetCharAngle (pwi->hps, &pTxt->gradl);      // Char angle
      GpiSetCharShear (pwi->hps, aptlShear + pTxt->TxtShear);  // Char shear

      ptl.x = (LONG)(pTxt->ptl.x * pwi->usFormWidth );
      ptl.y = (LONG)(pTxt->ptl.y * pwi->usFormHeight);
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;
      GpiCharStringAt(pwi->hps,&ptl,(LONG)strlen(pTxt->Str),pTxt->Str);
   }
   return;
}
/*-----------------------------------------------[ private ]-----------------*/
static void txtCalcOutLine(Textstruct pTxt, RECTL *prcl,WINDOWINFO *pwi)
{
      GpiResetBoundaryData(pwi->hps);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_ON);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
      DrawTxtOutline(pwi,pTxt);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_OFF);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
      GpiQueryBoundaryData(pwi->hps, prcl);

}
/*-----------------------------------------------[ public ]----------*/
/* txtDrawOutline.                                                   */
/*-------------------------------------------------------------------*/
void txtDrawOutline(POBJECT pObj, WINDOWINFO *pwi,BOOL bIgnoreLayer)
{
   Textstruct pT = (Textstruct)pObj;

   if (pwi->usdrawlayer == pT->bt.usLayer || bIgnoreLayer)
      DrawTxtOutline(pwi,(Textstruct)pObj);
   return;
}
/*-----------------------------------------------[ public ]----------*/
/* txtSetFont     called in drwmain before starting fontdlg.         */
/*-------------------------------------------------------------------*/
BOOL txtSetFont(POBJECT pObj, WINDOWINFO *pwi)
{
  BOOL bReturn;
  PTEXT  pText = (PTEXT)pObj;
  bReturn = FontDlg(pwi,&pText->fattrs,TRUE);
  if (bReturn)
     setupWidthTable(pObj,pwi);
  return bReturn;
}
/*-----------------------------------------------[ public  ]----------------*/
/* drawCharString                                                           */
/*                                                                          */
/* Description  : Draws a character string (SINGLE BYTE!!) in the given     */
/*                presentation space (pwi->hps). Important is that it used  */
/*                the character width table of the screen to position the   */
/*                text on the printer. Only when text is displayed horz.    */
/*                                                                          */
/* Note         : Also called from drwbtext.c.                              */
/*                                                                          */
/* Parameters   : WINDOWINFO * pwi - Reference to the context informataion. */
/*                LONG lCount      - String length.                         */
/*                char *           - Pointer to a null terminating string.  */
/*                LONG *           - Pointer to a width table.              */
/*                FIXED            - fxBreakExtra. only for adjusted btext  */
/*                int              - lBreakcount   only for adjusted btext  */
/*                                                                          */
/* Returns      : NONE.                                                     */
/*--------------------------------------------------------------------------*/
void drawCharString(WINDOWINFO *pwi,
                    LONG lCount, 
                    char *pszData,
                    LONG *plWidth,
                    FIXED fxBreakExtra,
                    int   iBreakCount )
{
   int i;
   LONG alWidth[255];
   LONG lWidth[255];
   float fx1,fx2,fFactor;
   SIZEF sizef;

   if (pwi->bPrinter && !pwi->bDBCS)
   {
      for (i = 0; i < lCount; i++)
      {
         alWidth[i] = plWidth[pszData[i]];
      }
      GpiQueryWidthTable(pwi->hps,0,MAX_NRWIDTH,lWidth);

      fx1 = (float)lWidth[ (LONG)'M'];
      fx2 = (float)plWidth[(LONG)'M'];
      fFactor = fx2/fx1; 

      GpiQueryCharBox(pwi->hps,&sizef);

      sizef.cx *= fFactor;
      sizef.cy *= fFactor;

      GpiSetCharBox(pwi->hps,&sizef);
      /*
      ** If the text is justified (block text only!) We should 
      ** insert extra space between the words ( lBreakCount >0)
      ** of extra space between the letters. The extra space 
      ** between the letters may cause letters to overlap each other..
      */
      if (iBreakCount || fxBreakExtra)
      {
         fxBreakExtra *= fFactor; /* Use the printer factor to get WYSIWYG*/

         if (iBreakCount > 0 && fxBreakExtra )
         {
            GpiSetCharBreakExtra (pwi->hps,fxBreakExtra);
         }
         else if (fxBreakExtra)
         {
            GpiSetCharExtra (pwi->hps,fxBreakExtra);
         }
      }

      GpiCharStringPos(pwi->hps,NULL,CHS_VECTOR,lCount,pszData,alWidth);

      if (iBreakCount || fxBreakExtra)
      {
          /*
          ** Reset the extra space always!!
          */
          GpiSetCharExtra(pwi->hps, 0) ;
          GpiSetCharBreakExtra(pwi->hps, 0);
      }
   }
   else
      GpiCharString(pwi->hps,lCount,pszData);
}
/*------------------------------------------------------------------------*/
/* Draws segment when a paint message has come in.                        */
/*                                                                        */
/* Parameters : HPS hps - presentation space to draw in.                  */
/*              WINDOWINFO *pwi - pointer to the environment              */
/*              RECTL *prcl - pointer to the area which needs repaint.    */
/*                            Only set for SCREEN. See drwmain.c          */
/*                                                                        */
/*------------------------------------------------------------------------*/
void DrawTextSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl)
{
   SIZEF  sizfx;
   POINTL ptl;            /* used for shading effect       */
   POINTL ptlbox;         /* Used for patternfill          */
   RECTL  rcl,rclDest;    /* Used for our clippath!        */

   Textstruct pStart = (Textstruct)pObj;
  
   GpiSetMix(hps,FM_DEFAULT);

   if (pwi->usdrawlayer == pStart->bt.usLayer)
   {
      if (pObj->bDirty)
      {
         /*
         ** Dirty flag is set after the object is loaded from file
         */
         setupWidthTable(pObj,pwi);
         pObj->bDirty = FALSE;        
      }

      if (prcl)
      {
         TextOutLine(pObj,&rcl,pwi,TRUE);
         if (!WinIntersectRect(hab,&rclDest,prcl,&rcl))
             return;
      }

      sizfx.cx = pStart->sizfx.fcx * pwi->usFormWidth;
      sizfx.cy = pStart->sizfx.fcy * pwi->usFormHeight;

      setFont(pwi,&pStart->fattrs,sizfx);

      if (!(pStart->ulState & TXT_CIRCULAR))
         GpiSetCharAngle (hps,&pStart->gradl);

      GpiSetCharShear (hps, aptlShear + pStart->TxtShear);


      if ((pStart->lShadeX || pStart->lShadeY ) &&
          !(pStart->ulState & TXT_CIRCULAR))
      {
         LONG lShadeX,lShadeY;
         lShadeX = pStart->lShadeX;
         lShadeY = pStart->lShadeY;

         GpiSetColor(hps,pStart->bt.ShadeColor);

         ptl.x = (LONG)(pStart->ptl.x * pwi->usFormWidth );
         ptl.y = (LONG)(pStart->ptl.y * pwi->usFormHeight);
         ptl.x += (LONG)pwi->fOffx;
         ptl.y += (LONG)pwi->fOffy;

         if (pwi->ulUnits == PU_PELS)
         {
            lShadeX = (lShadeX  * pwi->xPixels)/10000;
            lShadeY = (lShadeY  * pwi->yPixels)/10000;
         }
         ptl.x += lShadeX;
         ptl.y += lShadeY;

         GpiSetCurrentPosition(hps,&ptl);
         drawCharString(pwi,(LONG)strlen(pStart->Str),pStart->Str,pObj->lWidth,0,0);
      } /*end if there is shadow */
      else if ((pStart->ulState & TXT_CIRCULAR) && pStart->lShadeX)
         DisplayRotate (hps,pwi,pStart,TRUE);

      ptl.x = (LONG)(pStart->ptl.x * pwi->usFormWidth );
      ptl.y = (LONG)(pStart->ptl.y * pwi->usFormHeight);
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;
      GpiSetColor(hps,pStart->bt.fColor);
      GpiSetCurrentPosition(hps,&ptl);

      if (prcl && pStart->bt.lPattern != PATSYM_DEFAULT &&
          pStart->bt.lPattern != PATSYM_SOLID && !pwi->bSuppress)
      {
         TextOutLine(pObj,&rcl,pwi,FALSE);
         if (pwi->ulUnits == PU_PELS)
            GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)&rcl);

         GpiBeginPath(hps,1L);   /* define a clip path    */
      }
      else if (!prcl && pStart->bt.lPattern != PATSYM_DEFAULT &&
               pStart->bt.lPattern != PATSYM_SOLID && !pwi->bSuppress)
      {
         CalcTextClipArea(hps,pStart,pwi,&rcl);
         GpiBeginPath( hps, 1L);   /* printer  define a clip path   */
      }

      if ((pStart->ulState & TXT_CIRCULAR))
         DisplayRotate (hps,pwi,pStart,FALSE);
      else
         drawCharString(pwi,(LONG)strlen(pStart->Str),pStart->Str,pObj->lWidth,0,0);

      if (pStart->bt.lPattern != PATSYM_DEFAULT  &&
          pStart->bt.lPattern != PATSYM_SOLID    &&
          !pwi->bSuppress)
      {
         GpiEndPath(hps);
         GpiSetClipPath(hps,1L,SCP_AND);
         if (pStart->bt.lPattern == PATSYM_GRADIENTFILL)
         {
            /*
            ** Only solid gradient fill.
            */
            GpiSetPattern(hps,PATSYM_SOLID);
            GradientFill(pwi,hps,&rcl,&pStart->bt.gradient);
         }
         else if (pStart->bt.lPattern == PATSYM_FOUNTAINFILL)
         {
            GpiSetPattern(hps,PATSYM_SOLID);
            FountainFill(pwi,hps,&rcl,&pStart->bt.fountain);
         }
         else
         {
            GpiSetPattern(hps,pStart->bt.lPattern);
            ptlbox.x = rcl.xLeft;
            ptlbox.y = rcl.yTop;
            GpiMove(hps, &ptlbox);
            ptlbox.x = rcl.xRight;
            ptlbox.y = rcl.yBottom;
            GpiBox(hps,DRO_FILL,&ptlbox,0L,0L);
         }
         GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
      }

      if ((pStart->ulState & TXT_OUTLINE))
      {
         GpiSetPattern(hps,PATSYM_SOLID);
         GpiSetCurrentPosition(hps,&ptl);
         GpiSetMix(hps, FM_OVERPAINT);

         GpiBeginPath (hps, ID_PATH) ;
         if ((pStart->ulState & TXT_CIRCULAR))
            DisplayRotate (hps,pwi,pStart,FALSE);
         else
         {
            drawCharString(pwi,(LONG)strlen(pStart->Str),pStart->Str,pObj->lWidth,0,0);
         }
         GpiEndPath (hps) ;

         GpiSetColor(hps,pStart->bt.line.LineColor);
         if (pStart->bt.line.LineWidth > 1 && pwi->uXfactor >= (float)1)
         {
            LONG lLineWidth = pStart->bt.line.LineWidth;
            if (pwi->ulUnits == PU_PELS)
            {
               lLineWidth = (lLineWidth * pwi->xPixels)/10000;
            }
            GpiSetLineWidthGeom(hps,lLineWidth);
         }
         else
            GpiSetLineWidthGeom(hps,1);
         GpiStrokePath (hps, ID_PATH, 0L) ;   // Stroke path
      }

      GpiDeleteSetId (hps, 2L);
   }        /* if drawlayer */
}
/*------------------------------------------------------------------------*/
/*  Name: TxtMoveOutLine.                                                 */
/*                                                                        */
/*  Description : Shows the textouline while the object is moved.         */
/*                Called from the obj interface, and used in the          */
/*                WM_MOUSEMOVE message.                                   */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID TxtMoveOutLine(POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   RECTL rcl;
   POINTL ptl;
   SIZEF  sizfx;
   Textstruct pTxt = (Textstruct)pObj;

   if (!pTxt) return;

   /*
   ** When the group is stretched or moved, do not show any
   ** outline's.
   */

   if (!(pTxt->ulState & TXT_CIRCULAR))
   {
      sizfx.cx = (FIXED) (pTxt->sizfx.fcx * pwi->usFormWidth );
      sizfx.cy = (FIXED) (pTxt->sizfx.fcy * pwi->usFormHeight);

      setFont(pwi,&pTxt->fattrs,sizfx);

      GpiSetCharAngle (pwi->hps, &pTxt->gradl);     // Char angle
      GpiSetCharShear (pwi->hps, aptlShear + pTxt->TxtShear);  // Char shear

      ptl.x = (LONG)(pTxt->ptl.x * pwi->usFormWidth );
      ptl.y = (LONG)(pTxt->ptl.y * pwi->usFormHeight);
      ptl.x += (LONG)pwi->fOffx;
      ptl.y += (LONG)pwi->fOffy;
      ptl.x += (LONG)dx;
      ptl.y += (LONG)dy;
      GpiCharStringAt(pwi->hps,&ptl,(LONG)strlen(pTxt->Str),pTxt->Str);
   }
   else
   {
      /*
      ** For circular text we only show the outline during the move.
      ** A single redraw takes to long to do this during the mouse move.
      */
      TextOutLine((POBJECT)pTxt,&rcl,pwi,FALSE);
      ptl.x = rcl.xLeft + dx;
      ptl.y = rcl.yTop  + dy;
      GpiMove(pwi->hps,&ptl);
      ptl.x = rcl.xRight + dx;
      ptl.y = rcl.yBottom+ dy;
      GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0L,0L);
   }
}
/*-------------------------------------------[ private ]------------------*/
/*  TxtCircStretch.                                                       */
/*                                                                        */
/*  Description : Stretches the circular text.                            */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
static void TxtCircStretch(Textstruct pTxt,RECTL *rclNew, WINDOWINFO *pwi, ULONG ulMsg)
{
   LONG ulcxSize,ulcySize,ulSize;
   POINTL ptl;

   if (!(pTxt->ulState & TXT_CIRCULAR))
   {
      return;
   }

   switch(ulMsg)
   {

      case WM_MOUSEMOVE:
         GpiSetLineType(pwi->hps, LINETYPE_DOT);
         ptl.x = rclNew->xLeft;
         ptl.y = rclNew->yBottom;
         GpiMove(pwi->hps, &ptl);
         ptl.x = rclNew->xRight;
         ptl.y = rclNew->yTop;
         GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0L,0L);
         break;

      case WM_BUTTON1UP:
        /*
        ** box structure.
        */
        ulcySize = (rclNew->yTop-rclNew->yBottom);
        ulcxSize = (rclNew->xRight - rclNew->xLeft);

        if (ulcxSize > ulcySize)
           ulSize = ulcxSize;
        else
           ulSize = ulcySize;

        rclNew->xRight = rclNew->xLeft + ulSize;
        rclNew->yTop   = rclNew->yBottom + ulSize;

        pTxt->TxtCircular.rclf.xLeft   = (float)rclNew->xLeft;
        pTxt->TxtCircular.rclf.yBottom = (float)rclNew->yBottom;
        pTxt->TxtCircular.rclf.yTop    = (float)rclNew->yTop;
        pTxt->TxtCircular.rclf.xRight  = (float)rclNew->xRight;
        /*
        ** Normalize positions...
        */
        pTxt->TxtCircular.rclf.xLeft   /= (float)pwi->usFormWidth;
        pTxt->TxtCircular.rclf.yBottom /= (float)pwi->usFormHeight;
        pTxt->TxtCircular.rclf.yTop    /= (float)pwi->usFormHeight;
        pTxt->TxtCircular.rclf.xRight  /= (float)pwi->usFormWidth;
        break;
   }
   return;
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY TextDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   USHORT idxCol;
   USHORT idxRow;
   ULONG idItem;        /* color selection */
   SIZEF  sizfx;
   static HwndColor;
   static BOOL bInit;
   ColorStruct ClientStruct;
   SWP swp;             /* Screen Window Position Holder	*/
   ULONG ulStorage[2];    /* To get the vals out of the spins  */
   PVOID pStorage;
   POBJECT pObj;


   switch (msg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         defText.bDialogActive = TRUE;
         pObj = (POBJECT)defText.pTxt;
         pObj->bLocked = TRUE;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         lShade[0] = (SHORT)defText.pTxt->lShadeY;  /* shade y - direction */
         lShade[1] = (SHORT)defText.pTxt->lShadeX;  /* shade x - direction */

         sColor[2] = (SHORT)defText.pTxt->TxtShear; /*Shear of the chars   */
         sColor[3] = 0;
         sColor[5] = defText.pTxt->bt.ShadeColor;

         sizfx.cx = (FIXED) (defText.pTxt->sizfx.fcx * defText.pwi->usFormWidth );
         sizfx.cy = (FIXED) (defText.pTxt->sizfx.fcy * defText.pwi->usFormHeight);
         /*
         ** Init HORZ SHADE, VERT SHADE, SHEAR.
         */
         WinSendDlgItemMsg( hwnd, ID_SPNHORZ, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_SHADING), 
                            MPFROMLONG(MIN_SHADING));

         WinSendDlgItemMsg( hwnd, ID_SPNVERT, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_SHADING), 
                            MPFROMLONG(MIN_SHADING));

         WinSendDlgItemMsg( hwnd, ID_SPNSHEAR, SPBM_SETLIMITS,
                            MPFROMLONG(MAX_SHEAR), 
                            MPFROMLONG(MIN_SHEAR));

         WinSendDlgItemMsg( hwnd, ID_SPNVERT, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)lShade[0]), NULL);

         WinSendDlgItemMsg( hwnd, ID_SPNHORZ, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)lShade[1]), NULL);

         WinSendDlgItemMsg( hwnd, ID_SPNSHEAR, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)sColor[2]), NULL);
         /*
         ** Initialize our spinbuttons, start with charsizes cx and cy.
         */
         WinSendDlgItemMsg(hwnd,ID_SPNCX, SPBM_SETLIMITS,
                           MPFROMLONG(MAXCHARCX), MPFROMLONG(MINCHARCX));
         WinSendDlgItemMsg(hwnd,ID_SPNCY,SPBM_SETLIMITS,
                           MPFROMLONG(MAXCHARCY), MPFROMLONG(MINCHARCY));
         WinSendDlgItemMsg(hwnd,ID_SPNCX,SPBM_SETCURRENTVALUE,
                           MPFROMLONG((sizfx.cx >> 16) + 1), NULL);
         WinSendDlgItemMsg(hwnd,ID_SPNCY,SPBM_SETCURRENTVALUE,
                           MPFROMLONG((sizfx.cy >> 16) + 1), NULL);

         HwndColor = WinWindowFromID(hwnd, ID_TXWND);
         /*
         ** Fill the color value set with the colors
         */
         for (idxCol=1; idxCol<= 10; idxCol++)
            for (idxRow=1; idxRow<=4; idxRow++)
               WinSendDlgItemMsg(hwnd,ID_VALCOLOR,VM_SETITEM,
                         MPFROM2SHORT(idxRow,idxCol),
                         MPFROMLONG(ColorTab[ (10 * (idxRow-1))+(idxCol-1) ]));
         /*
         ** setup the layer spinbutton
         */
         WinSendDlgItemMsg(hwnd, ID_SPNLAYER, SPBM_SETLIMITS,
                           MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));
         WinSendDlgItemMsg(hwnd, ID_SPNLAYER, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)defText.pTxt->bt.usLayer), NULL);
         bInit = FALSE;
         return 0;

      case WM_COLORING:
         memcpy(&ClientStruct,(pColorStruct )mp1,sizeof(ColorStruct));
         WinSendDlgItemMsg(hwnd,ID_VALCOLOR,VM_SETITEM,MPFROM2SHORT(1,1),
                           MPFROMLONG((ULONG) ClientStruct.rgb[0] << 16 |
                                              (ULONG) ClientStruct.rgb[1] <<  8 |
                                              (ULONG) ClientStruct.rgb[2]));

         sColor[5] = (ULONG) ClientStruct.rgb[0] << 16 |
                     (ULONG) ClientStruct.rgb[1] <<  8 |
                     (ULONG) ClientStruct.rgb[2];
         WinInvalidateRect (HwndColor,NULL, FALSE);
         return 0;
      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               defText.pTxt->lShadeY     = lShade[0];
               defText.pTxt->lShadeX     = lShade[1];
               defText.pTxt->TxtShear    = (USHORT)sColor[2];
               defText.pTxt->bt.ShadeColor  = (ULONG)sColor[5];
               /*
               ** Get the values from the spinbuttons
               */
               pStorage = (PVOID)ulStorage;

               WinSendDlgItemMsg(hwnd,ID_SPNCX,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));

               sizfx.cx  = ulStorage[0] << 16;
               defText.pTxt->sizfx.fcx  = (float)(sizfx.cx /defText.pwi->usFormWidth);

               WinSendDlgItemMsg(hwnd,ID_SPNCY,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));

               sizfx.cy  = ulStorage[0] << 16;
               defText.pTxt->sizfx.fcy    = (float)(sizfx.cy/defText.pwi->usFormHeight);
               /*
               ** Get our layer info out of the spin
               */
               WinSendDlgItemMsg(hwnd,ID_SPNLAYER,SPBM_QUERYVALUE,
                                 (MPARAM)(pStorage),MPFROM2SHORT(0,0));

               defText.pTxt->bt.usLayer = (USHORT)ulStorage[0];

               WinPostMsg(defText.pwi->hwndClient,UM_ENDDIALOG,(MPARAM)defText.pTxt,(MPARAM)0);
               defText.bDialogActive = FALSE;
               pObj = (POBJECT)defText.pTxt;
               pObj->bLocked = FALSE;
               WinDismissDlg(hwnd,TRUE);
               return 0;
            case DID_CANCEL:
               defText.bDialogActive = FALSE;
               pObj = (POBJECT)defText.pTxt;
               pObj->bLocked = FALSE;
               WinDismissDlg(hwnd,TRUE);
               return 0;
            case DID_HELP:
               ShowDlgHelp(hwnd);
               return 0;
         }
         return (MRESULT)0;

      case WM_CONTROL:
         switch (LOUSHORT(mp1))
         {
            case ID_VALCOLOR:
               if (HIUSHORT(mp1) == VN_SELECT)
               {
	          idItem = (ULONG)WinSendDlgItemMsg(hwnd,ID_VALCOLOR,VM_QUERYSELECTEDITEM, NULL,NULL);
                  sColor[5] = (ULONG)WinSendDlgItemMsg(hwnd,ID_VALCOLOR,
                                                       VM_QUERYITEM,MPFROMLONG(idItem),NULL);
                  WinInvalidateRect (HwndColor,NULL, FALSE);
	          return (MRESULT)0;
               }
               break;
            case ID_SPNHORZ:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNHORZ,SPBM_QUERYVALUE,
                                    (MPARAM)&lShade[1],MPFROM2SHORT(0,0));
                  if (lShade[1] <= MIN_SHADING)
                     lShade[1] = MIN_SHADING;
                  lShade[1] = min(lShade[1],MAX_SHADING);
                  WinInvalidateRect (HwndColor,NULL, FALSE);
               }            
               break;
            case ID_SPNVERT:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNVERT,SPBM_QUERYVALUE,
                                    (MPARAM)&lShade[0],MPFROM2SHORT(0,0));
                  if (lShade[0] <= MIN_SHADING)
                     lShade[0] = MIN_SHADING;
                  lShade[0] = min(lShade[0],MAX_SHADING);
                  WinInvalidateRect (HwndColor,NULL, FALSE);
               }            
               break;
            case ID_SPNSHEAR:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPNSHEAR,SPBM_QUERYVALUE,
                                    (MPARAM)&sColor[2],MPFROM2SHORT(0,0));
                  if (sColor[2] <= MIN_SHADING)
                     sColor[2] = MIN_SHADING;
                  sColor[2] = min(sColor[2],MAX_SHADING);
                  WinInvalidateRect (HwndColor,NULL, FALSE);
               }            
               break;
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}

MRESULT EXPENTRY ClrWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static POINTL ptl;
   USHORT cxBox,cyBox;
   RECTL rcl;
   HPS   hps;
   POINTL ptlShade;
   char facename[40];
   SIZEF sizefx;
   PPOINTL aptl;          /* Pointer een array van punten */
   POINTL  nptl[5];        /* QueryTextbox needs 5 points  */
   USHORT cxClient,cyClient;

   switch(msg)
   {
      case WM_CREATE:
         WinQueryWindowRect(hwnd,&rcl);
         cxClient = rcl.xRight - rcl.xLeft;
         cyClient = rcl.yTop - rcl.yBottom;
         aptl = nptl;        /* To give querytextbox space to work with  */
         hps = WinGetPS(hwnd);
         /*
         ** Calculate boxsize to set our string in the center.
         */
         sizefx.cx = MAKEFIXED(24,0);
         sizefx.cy = MAKEFIXED(24,0);
         strcpy(facename,defText.pTxt->fattrs.szFacename);
         if (!strlen(facename))
            FontInit(hps,"Courier",FONT_SET,&sizefx);
         else
            FontInit(hps, facename,FONT_SET,&sizefx);
         GpiSetCharBox(hps,&sizefx);

         GpiSetCurrentPosition(hps,&ptl);
         GpiQueryTextBox(hps,
                         (LONG)strlen(DEFAULTSTR),
                         DEFAULTSTR,
                         TXTBOX_COUNT,
                         aptl);

         cxBox = aptl[TXTBOX_TOPRIGHT].x - aptl[TXTBOX_TOPLEFT].x;
         cyBox = aptl[TXTBOX_TOPLEFT].y - aptl[TXTBOX_BOTTOMLEFT].y;

         ptl.x = (cxClient - cxBox) / 2;
         ptl.y = (cyClient - cyBox) / 2+ (cyBox/2);
         GpiSetCharSet (hps, LCID_DEFAULT) ;     // Clean up
         GpiDeleteSetId (hps, 2L);
         WinReleasePS(hps);
         return 0;
      case WM_PAINT:
	 WinQueryWindowRect(hwnd,&rcl);
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiCreateLogColorTable ( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         WinFillRect(hps,&rcl,0x00FFFFFF);
         sizefx.cx = MAKEFIXED(24,0);
         sizefx.cy = MAKEFIXED(24,0);
         strcpy(facename,defText.pTxt->fattrs.szFacename);
         if (!strlen(facename))
            FontInit(hps,"Courier",FONT_SET,&sizefx);
         else
            FontInit(hps, facename,FONT_SET,&sizefx);
         GpiSetCharBox(hps,&sizefx);

         GpiSetCharShear (hps, aptlShear + sColor[2]) ;         // Char shear

         if (lShade[0] || lShade[1] )  /* if there is shading make it */
         {
            ptlShade.x = ptl.x;
            ptlShade.y = ptl.y;
            GpiSetColor(hps,(ULONG)sColor[5]); /* shading color */

            ptlShade.y += (defText.pwi->yPixels * lShade[0])/10000;
            ptlShade.x += (defText.pwi->xPixels * lShade[1])/10000;
            GpiCharStringAt(hps,&ptlShade,(ULONG)strlen(DEFAULTSTR),DEFAULTSTR);
         }  /* endof shading effect for dialog*/

         GpiSetColor(hps,(ULONG)sColor[4]); /* char color */
         GpiSetBackColor(hps,(ULONG)sColor[7]);

         GpiCharStringAt(hps,&ptl,(ULONG)strlen(DEFAULTSTR),DEFAULTSTR);

         if (defText.pTxt->ulState & TXT_OUTLINE)
         {
            GpiBeginPath (hps, ID_PATH) ;
            GpiCharStringAt(hps,&ptl,(ULONG)strlen(DEFAULTSTR),DEFAULTSTR);
            GpiEndPath (hps) ;
            GpiSetColor(hps,(ULONG)sColor[6]);    // Set the outline color
            GpiStrokePath (hps, ID_PATH, 0L) ;   // Stroke path
         }

        GpiQueryFontMetrics(hps,
			     (LONG)sizeof(fmetrics),
			     &fmetrics);
         GpiSetCharSet (hps, LCID_DEFAULT) ;     // Clean up
         GpiDeleteSetId (hps, 2L);
         WinEndPaint(hps);
         return 0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}

/*------------------------------------------------------------------------*/
VOID * TextSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   Textstruct  pStart = (Textstruct)pObj;
   RECTL rcl;
   /*
   ** Check first if the current layer
   ** the same is as the layer where the textsegment belongs to
   ** Hence save calculation time.
   */

   if (pStart->bt.usLayer == pwi->uslayer || pwi->bSelAll)
   {
      TextOutLine(pObj,&rcl,pwi,FALSE);
      if (WinPtInRect((HAB)0,&rcl,&ptl))
         return (void *)pObj;
   }
   return ( void *)0;
}
/*------------------------------------------------------------------------*/
/* TxtFlipHoriz : Mirror the text horizontal.                             */
/*------------------------------------------------------------------------*/
void TxtFlipHoriz(POBJECT pObj, WINDOWINFO *pwi)
{
   Textstruct  pTxt = (Textstruct)pObj;
   RECTL rcl;

   if (!pObj) return;

   pTxt->sizfx.fcx *= -1;
   TextOutLine((POBJECT)pObj, &rcl,pwi,FALSE);

   if (pTxt->sizfx.fcx < 0)
      pTxt->ptl.x =  (float)rcl.xRight;
   else
      pTxt->ptl.x =  (float)rcl.xLeft;

   pTxt->ptl.x /= (float)pwi->usFormWidth;
}
/*------------------------------------------------------------------------*/
/* TxtFlipVert : Mirror the text vertical.  Put it upside down.          */
/*------------------------------------------------------------------------*/
void TxtFlipVert(POBJECT pObj, WINDOWINFO *pwi)
{
   Textstruct  pTxt = (Textstruct)pObj;
   if (!pObj) return;
   pTxt->sizfx.fcy *= -1;
}
/*------------------------------------------------------------------------*/
/* TextOutLine(Textstruct pTxt, RECTL *rcl, BOOL bCalc)                   */
/*                                                                        */
/* Calculates the outline box of the selected text.                       */
/* text element pointed by pTxt.                                          */
/*                                                                        */
/* IMPORTANT: The charbox is reset to the default by this function!!!     */
/*------------------------------------------------------------------------*/
void TextOutLine(POBJECT pObj, RECTL * prcl, WINDOWINFO *pwi, BOOL bCalc)
{
  Textstruct pTxt = (Textstruct)pObj;

   if (!pObj)        /* No text struct... return */
      return;

   if (!(pTxt->ulState & TXT_CIRCULAR))
   {
      /*
      ** Not a circular text... ]
      ** so the calculation is straight forward.
      */
      if (bCalc)
         txtCalcOutLine(pTxt,&pObj->rclOutline,pwi);

      prcl->xLeft   = pObj->rclOutline.xLeft;
      prcl->xRight  = pObj->rclOutline.xRight;
      prcl->yTop    = pObj->rclOutline.yTop;
      prcl->yBottom = pObj->rclOutline.yBottom;
   }
   else
   {
      /*
      ** This is circular text so...
      */
      prcl->xLeft   = (LONG)(pTxt->TxtCircular.rclf.xLeft  *  pwi->usFormWidth );
      prcl->yBottom = (LONG)(pTxt->TxtCircular.rclf.yBottom * pwi->usFormHeight);
      prcl->xRight  = (LONG)(pTxt->TxtCircular.rclf.xRight *  pwi->usFormWidth );
      prcl->yTop    = (LONG)(pTxt->TxtCircular.rclf.yTop   *  pwi->usFormHeight);

      prcl->xLeft   += (LONG)pwi->fOffx;
      prcl->xRight  += (LONG)pwi->fOffx;
      prcl->yTop    += (LONG)pwi->fOffy;
      prcl->yBottom += (LONG)pwi->fOffy;

   }
}
/*------------------------------------------------------------------------*/
/* TextInvArea(Textstruct pTxt, RECTL *rcl)                               */
/*                                                                        */
/* Calculates the invallid window area of which us covered by the         */
/* text element pointed by pTxt.                                          */
/*                                                                        */
/* Returns : In rcl the invalid area.                                     */
/*------------------------------------------------------------------------*/
void TextInvArea(POBJECT pObj, RECTL * prcl, WINDOWINFO *pwi, BOOL bInc)
{
   Textstruct pTxt = (Textstruct)pObj;

   if (!pTxt)        /* No text struct... return */
      return;

   /* first get the corner coordinates */

   TextOutLine((POBJECT)pTxt,prcl,pwi,TRUE);

   /*
   ** Add the shadow to the inv area.
   */

   if (pTxt->lShadeX)
   {
      prcl->xLeft   -= pTxt->lShadeX;
      prcl->xRight  += pTxt->lShadeX;
      prcl->yBottom -= pTxt->lShadeX;
      prcl->yTop    += pTxt->lShadeX;
   }

   /*
   ** Add the handles 50 * 0.1 MM
   */
   if (bInc)
   {
     prcl->xLeft   -= 60;
     prcl->xRight  += 60;
     prcl->yBottom -= 60;
     prcl->yTop    += 60;
   }
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)prcl);

   if (!bInc)
   {
     prcl->xLeft   -= 2;
     prcl->xRight  += 2;
     prcl->yBottom -= 2;
     prcl->yTop    += 2;
   }
 }
/*------------------------------------------------------------------------*/
/*  RegisterTxt.                                                          */
/*                                                                        */
/*  Description : Register the windowprocedures for the windows living    */
/*                in the "circular text" dialog and the text object       */
/*                detail dialog.                                          */
/*                of objects.                                             */
/*------------------------------------------------------------------------*/
void RegisterTxt(HAB hab)
{

   WinRegisterClass(hab,                  // Another block handle
                    (PSZ)"COLORCLASS",    // Name of class being registered
                    (PFNWP)ClrWndProc,	  // Window procedure for class
                    CS_SIZEREDRAW,        // Class style
                    0L);                  // Extra bytes to reserve

   WinRegisterClass(hab,                  // Another block handle
                    (PSZ)"TXTCIRCULAR",   // Name of class being registered
                    (PFNWP)TxtCirWndProc, // Window procedure for class
                    CS_SIZEREDRAW,        // Class style
                    0L);                  // Extra bytes to reserve

}
/*------------------------------------------------------------------------*/
/*  TxtPutInGroup.                                                        */
/*                                                                        */
/*  Description : Here we put the TXT in the given rectl of the group.    */
/*                In fact the original parent is replaced by a new one.   */
/*                When the TXT is not part of a group than the parent is  */
/*                the paper.                                              */
/*                                                                        */
/*  Parameters  : POBJECT pObj-pointer to a sqr object which is put in the*/
/*                             group.                                     */
/*                RECTL * - pointer to the rect of the group.             */
/*                WINDOWINFO * - pointer to the original windowinfo.      */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void TxtPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   Textstruct pTxt = (Textstruct)pObj;
   USHORT  usGrpWidth,usGrpHeight;


   if (!pTxt) return;

   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth  = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight = (USHORT)( prcl->yTop   - prcl->yBottom);

   pTxt->sizfx.fcx *= pwi->usFormWidth;
   pTxt->sizfx.fcy *= pwi->usFormHeight;

   pTxt->sizfx.fcx /= usGrpWidth;
   pTxt->sizfx.fcy /= usGrpHeight;

   pTxt->ptl.x *= (float)pwi->usFormWidth;
   pTxt->ptl.y *= (float)pwi->usFormHeight;

   pTxt->ptl.x -= (float)prcl->xLeft;
   pTxt->ptl.y -= (float)prcl->yBottom;

   pTxt->ptl.x /= (float)usGrpWidth;
   pTxt->ptl.y /= (float)usGrpHeight;

   if ((pTxt->ulState & TXT_CIRCULAR))
   {
      /*
      ** This is circular text so...
      */
      pTxt->TxtCircular.rclf.xLeft   *= (float)pwi->usFormWidth;
      pTxt->TxtCircular.rclf.yBottom *= (float)pwi->usFormHeight;
      pTxt->TxtCircular.rclf.xRight  *= (float)pwi->usFormWidth;
      pTxt->TxtCircular.rclf.yTop    *= (float)pwi->usFormHeight;

      pTxt->TxtCircular.rclf.xLeft   -= (float)prcl->xLeft;
      pTxt->TxtCircular.rclf.xRight  -= (float)prcl->xLeft;
      pTxt->TxtCircular.rclf.yBottom -= (float)prcl->yBottom;
      pTxt->TxtCircular.rclf.yTop    -= (float)prcl->yBottom;

      pTxt->TxtCircular.rclf.xLeft   /= (float)usGrpWidth;
      pTxt->TxtCircular.rclf.yBottom /= (float)usGrpHeight;
      pTxt->TxtCircular.rclf.xRight  /= (float)usGrpWidth;
      pTxt->TxtCircular.rclf.yTop    /= (float)usGrpHeight;
   }
}
/*------------------------------------------------------------------------*/
/*  TxtRemFromGroup.                                                      */
/*                                                                        */
/*  Remove the object from the group.                                     */
/*------------------------------------------------------------------------*/
void TxtRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi)
{
   Textstruct pTxt = (Textstruct)pObj;
   USHORT  usGrpWidth,usGrpHeight;

   if (!pTxt) return;

   /*
   ** Use the width of the group as the formwidth.
   */
   usGrpWidth  = (USHORT)( prcl->xRight - prcl->xLeft);
   usGrpHeight = (USHORT)( prcl->yTop   - prcl->yBottom);

   pTxt->sizfx.fcx *= usGrpWidth;
   pTxt->sizfx.fcy *= usGrpHeight;

   pTxt->sizfx.fcx /= pwi->usFormWidth;
   pTxt->sizfx.fcy /= pwi->usFormHeight;

   pTxt->ptl.x *= (float)usGrpWidth;
   pTxt->ptl.y *= (float)usGrpHeight;

   pTxt->ptl.x += (float)prcl->xLeft;
   pTxt->ptl.y += (float)prcl->yBottom;

   pTxt->ptl.x /= (float)pwi->usFormWidth;
   pTxt->ptl.y /= (float)pwi->usFormHeight;

   if ((pTxt->ulState & TXT_CIRCULAR))
   {
      /*
      ** This is circular text so...
      */
      pTxt->TxtCircular.rclf.xLeft   *= (float)usGrpWidth;
      pTxt->TxtCircular.rclf.yBottom *= (float)usGrpHeight;
      pTxt->TxtCircular.rclf.xRight  *= (float)usGrpWidth;
      pTxt->TxtCircular.rclf.yTop    *= (float)usGrpHeight;

      pTxt->TxtCircular.rclf.xLeft   += (float)prcl->xLeft;
      pTxt->TxtCircular.rclf.xRight  += (float)prcl->xLeft;
      pTxt->TxtCircular.rclf.yBottom += (float)prcl->yBottom;
      pTxt->TxtCircular.rclf.yTop    += (float)prcl->yBottom;

      pTxt->TxtCircular.rclf.xLeft   /= (float)pwi->usFormWidth;
      pTxt->TxtCircular.rclf.yBottom /= (float)pwi->usFormHeight;
      pTxt->TxtCircular.rclf.xRight  /= (float)pwi->usFormWidth;
      pTxt->TxtCircular.rclf.yTop    /= (float)pwi->usFormHeight;
   }
}
/*------------------------------------------------------------------------*/
/*  TextStretch.                                                          */
/*                                                                        */
/*  Description : Stretches the circle. Called during the following msg's.*/
/*                WM_BUTTON1DOWN,WM_BUTTON1UP and WM_MOUSEMOVE.           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void TextStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg)
{
   Textstruct pTxt = (Textstruct)pObj;
   static USHORT usWidth,usHeight;
   USHORT usOldWidth,usOldHeight;
   static float fOffx,fOffy;
   float fOldx,fOldy;

   if (!pTxt) return;

   usWidth  =(USHORT)(prcl->xRight - prcl->xLeft);
   usHeight =(USHORT)(prcl->yTop - prcl->yBottom);

   switch(ulMsg)
   {
      case WM_BUTTON1DOWN:
         if ((pTxt->ulState & TXT_CIRCULAR))
            return;

         pTxt->sizfx.fcx    *= (float)pwi->usFormWidth;
         pTxt->sizfx.fcy    *= (float)pwi->usFormHeight;
         pTxt->sizfx.fcx    /= (float)usWidth;
         pTxt->sizfx.fcy    /= (float)usHeight;
         pTxt->ptl.x        *= (float)pwi->usFormWidth;
         pTxt->ptl.y        *= (float)pwi->usFormHeight;
         pTxt->ptl.x        -= (float)prcl->xLeft;
         pTxt->ptl.y        -= (float)prcl->yBottom;
         pTxt->ptl.x        /= (float)usWidth;
         pTxt->ptl.y        /= (float)usHeight;

         pTxt->bt.ptlfCenter.x *= (float)pwi->usFormWidth;
         pTxt->bt.ptlfCenter.y *= (float)pwi->usFormHeight;
         pTxt->bt.ptlfCenter.x -= (float)prcl->xLeft;
         pTxt->bt.ptlfCenter.y -= (float)prcl->yBottom;
         pTxt->bt.ptlfCenter.x /= (float)usWidth;
         pTxt->bt.ptlfCenter.y /= (float)usHeight;
         /* FALL-THROUGH...*/

      case WM_MOUSEMOVE:
         if ((pTxt->ulState & TXT_CIRCULAR))
         {
            TxtCircStretch(pTxt,prcl,pwi,ulMsg);
            return;
         }
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
         TxtMoveOutLine(pObj,pwi,0,0);
         pwi->usFormWidth  = usOldWidth;
         pwi->usFormHeight = usOldHeight;
         pwi->fOffx = fOldx;
         pwi->fOffy = fOldy;
         return;

      case WM_BUTTON1UP:
         if (!(pTxt->ulState & TXT_CIRCULAR))
         {
         pTxt->sizfx.fcx    *= (float)usWidth;
         pTxt->sizfx.fcy    *= (float)usHeight;
         pTxt->sizfx.fcx    /= (float)pwi->usFormWidth;
         pTxt->sizfx.fcy    /= (float)pwi->usFormHeight;
         pTxt->ptl.x        *= (float)usWidth;
         pTxt->ptl.y        *= (float)usHeight;
         pTxt->ptl.x        += (float)prcl->xLeft;
         pTxt->ptl.y        += (float)prcl->yBottom;
         pTxt->ptl.x        /= (float)pwi->usFormWidth;
         pTxt->ptl.y        /= (float)pwi->usFormHeight;

         pTxt->bt.ptlfCenter.x *= (float)usWidth;
         pTxt->bt.ptlfCenter.y *= (float)usHeight;
         pTxt->bt.ptlfCenter.x += (float)prcl->xLeft;
         pTxt->bt.ptlfCenter.y += (float)prcl->yBottom;
         pTxt->bt.ptlfCenter.x /= (float)pwi->usFormWidth;
         pTxt->bt.ptlfCenter.y /= (float)pwi->usFormHeight;
         setupWidthTable(pObj,pwi); /* @chxxx */
         showTxtFont(pObj,pwi); /* Show result in status line */
         }
         else
         {
             setupWidthTable(pObj,pwi); /* @chxxx */
             TxtCircStretch(pTxt,prcl,pwi,ulMsg);
         }
         return;
   }
   return;
}
/*---------------------------------------------------------------------------*/
void TxtDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg, POINTL *pt)
{
   Textstruct pTxt = (Textstruct)pObj;
   POINTL ptl;
   static SIZEF sizfx;
   static POINTL ptlCenter;
   GRADIENTL gradl;


   if (!pTxt) return;


   switch (ulMsg)
   {
      case WM_BUTTON1UP:
         if (!pt)
         {
            ptlCenter.x = (LONG)(pTxt->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pTxt->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter = *pt;

         ptl.x = (LONG)(pTxt->ptl.x * pwi->usFormWidth );
         ptl.y = (LONG)(pTxt->ptl.y * pwi->usFormHeight);
         ptl.x += (LONG)(pwi->fOffx);
         ptl.y += (LONG)(pwi->fOffy);

         pTxt->fRotate += lRotate;
         RotateSqrSegment(lRotate,ptlCenter,&ptl,1);

         pTxt->ptl.x = (float)ptl.x;
         pTxt->ptl.y = (float)ptl.y;

         pTxt->ptl.x -= (LONG)(pwi->fOffx);
         pTxt->ptl.y -= (LONG)(pwi->fOffy);

         pTxt->ptl.x /= pwi->usFormWidth;
         pTxt->ptl.y /= pwi->usFormHeight;
         pTxt->gradl.x = (LONG)VECTOR * cos(pTxt->fRotate);
         pTxt->gradl.y = (LONG)VECTOR * sin(pTxt->fRotate);
         break;

      case WM_BUTTON1DOWN:
         sizfx.cx = (FIXED) (pTxt->sizfx.fcx * pwi->usFormWidth );
         sizfx.cy = (FIXED) (pTxt->sizfx.fcy * pwi->usFormHeight);
         setFont(pwi,&pTxt->fattrs,sizfx);
         if (!pt)
         {
            ptlCenter.x = (LONG)(pTxt->bt.ptlfCenter.x * pwi->usFormWidth );
            ptlCenter.y = (LONG)(pTxt->bt.ptlfCenter.y * pwi->usFormHeight);
         }
         else
            ptlCenter = *pt;

      default:
         ptl.x = (LONG)(pTxt->ptl.x * pwi->usFormWidth );
         ptl.y = (LONG)(pTxt->ptl.y * pwi->usFormHeight);
         ptl.x += (LONG)(pwi->fOffx);
         ptl.y += (LONG)(pwi->fOffy);
         RotateSqrSegment(lRotate,ptlCenter,&ptl,1);

         lRotate += pTxt->fRotate;

         gradl.x = (LONG)VECTOR * cos(lRotate);
         gradl.y = (LONG)VECTOR * sin(lRotate);
         setFont(pwi,&pTxt->fattrs,sizfx);
         GpiSetCharAngle (pwi->hps, &gradl);                      // Char angle
         GpiSetCharShear (pwi->hps, aptlShear + pTxt->TxtShear);  // Char shear
         GpiCharStringAt(pwi->hps,&ptl,(LONG)strlen(pTxt->Str),pTxt->Str);
         break;
   }
}
/*------------------------------------------------------------------------*/
/* Handles the small dialog for changing the text.                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ChangeTextDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static Textstruct  pStart;
   static WINDOWINFO  *pwi;
   char   ItemText[100];
   LONG   Retlen;
   SWP    swp;     /* Screen Window Position Holder  */

   switch (msg)
   {
      case WM_INITDLG:
         pwi       = (WINDOWINFO *)mp2;
         pStart    = (Textstruct)pwi->pvCurrent;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         WinSendDlgItemMsg(hwnd,ID_CHTXT,EM_SETTEXTLIMIT,(MPARAM)100,(MPARAM)0);
	 WinSetDlgItemText(hwnd,ID_CHTXT,pStart->Str);
         return 0;
      case WM_COMMAND:
	 switch(LOUSHORT(mp1))
	 {
            case DID_OK:
               Retlen = WinQueryDlgItemText(hwnd,ID_CHTXT,sizeof(ItemText),ItemText);
               if (Retlen)
                  strcpy(pStart->Str,ItemText);
               WinPostMsg(pwi->hwndClient,UM_ENDDIALOG,(MPARAM)pStart,(MPARAM)0);
               WinDismissDlg(hwnd,TRUE);
               return 0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,TRUE);
               return 0;
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*---------------------------------------------------------------------*/
/* setTxtOutline.                                                      */
/*                                                                     */
/* Description  : Switched to off when explicitly done. Or implicit    */
/*                switched on when a linewidth change comes in for     */
/*                for text.                                            */
/*---------------------------------------------------------------------*/
void setTxtOutline(POBJECT pObj,BOOL bAction)
{
   Textstruct pTxt = (Textstruct )pObj;

   if (bAction)
      pTxt->ulState |= TXT_OUTLINE;
   else
      pTxt->ulState &= ~TXT_OUTLINE;
   return;
}
/*------------------------------------------------------------------------*/
BOOL isTxtOutlineSet(POBJECT pObj)
{
   Textstruct pTxt = (Textstruct )pObj;
   return (BOOL)(pTxt->ulState & TXT_OUTLINE);
}
/*---------------------------------------------------------------------*/
/* setFontSquare.                                                      */
/*                                                                     */
/* Description  : Makes the character box of the used font square.     */
/*---------------------------------------------------------------------*/
void setFontSquare(POBJECT pObj, WINDOWINFO *pwi)
{
   SIZEFF     sizf; /* floating point version of SIZEF */
   Textstruct pTxt = (Textstruct )pObj;
   RECTL      rcl;

   TextInvArea(pTxt,&rcl,pwi,TRUE);

   sizf.fcx = pTxt->sizfx.fcx * pwi->usFormWidth;
   sizf.fcy = pTxt->sizfx.fcy * pwi->usFormHeight;

   if (sizf.fcx < sizf.fcy)
      pTxt->sizfx.fcy = (float)(sizf.fcx / pwi->usFormHeight);
   else
      pTxt->sizfx.fcx = (float)(sizf.fcy / pwi->usFormWidth);

   showTxtFont(pObj,pwi);

   WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   return;
}
/*---------------------------------------------------------------------*/
/* showTxtFont.                                                        */
/*                                                                     */
/* Description  : Sets the fontname in the status line.                */
/*---------------------------------------------------------------------*/
void showTxtFont(POBJECT pObj, WINDOWINFO *pwi)
{
  PTEXT pText = (PTEXT)pObj;
  SIZEF sizfx;
  sizfx.cx = (FIXED)(pText->sizfx.fcx * pwi->usWidth );
  sizfx.cy = (FIXED)(pText->sizfx.fcy * pwi->usHeight);
  showFontInStatusLine(sizfx,pText->fattrs.szFacename);
  return;
}
/*--------------------------------------------------------------------------*/
void textDetail( HWND hOwner, WINDOWINFO *pwi)
{
   if (!defText.bDialogActive)
   {
      defText.pwi  = pwi;
      defText.pTxt = (PTEXT)pwi->pvCurrent;
      WinLoadDlg(HWND_DESKTOP,hOwner,(PFNWP)TextDlgProc,(HMODULE)0,
                 ID_TXTOBJ,(PVOID)pwi);
   }
   return;
}
/*---------------------------------------------------------------------------*/
BOOL txtEdit(POBJECT pObj, WINDOWINFO *pwi)
{
  WinLoadDlg(HWND_DESKTOP,pwi->hwndClient,(PFNWP)ChangeTextDlgProc,
             0L,DLG_CHTXT,(PVOID)pwi);
  return TRUE;
}
