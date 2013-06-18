#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_txt.h"
#include "dlg_fnt.h"
#include "drwtxtin.h"
#include "drwcanv.h"


#define CURSORCX  10  /* 20 * 0.1 mm */
#define CURSORCY  40  /* 40 * 0.1 mm */

static char Inputstr[120]; /*------Space for the inp string------------*/
static POINTL aptl[120];   /* 120 chars means 120 different positions  */
static USHORT Index;       /* General index in our string              */
static POINTL ptlCurs;     /* x,y Position of our cursor               */
static POINTL ptlStCsr;    /* x,y StartPosition of our cursor          */
static BOOL   SegmOpen=FALSE;

static Textstruct pTxt;

/*------------------------------------------------------------------------*/
static void CalcStringInvArea(WINDOWINFO *pwi, PPOINTL pptl, char *String, RECTL *rcl)
{
   PPOINTL naptl;           /* Pointer een array van punten */
   POINTL  nptl[5];        /* QueryTextbox needs 5 points  */


   naptl = nptl;            /* To give querytextbox space to work with  */

   GpiSetCurrentPosition(pwi->hps,pptl);

   GpiQueryTextBox(pwi->hps,
                   (LONG)strlen(String),
                   (PSZ)String,
                   TXTBOX_COUNT,
                   naptl);

   rcl->yTop    = naptl[TXTBOX_TOPLEFT].y;
   rcl->xLeft   = naptl[TXTBOX_BOTTOMLEFT].x;
   rcl->xRight  = naptl[TXTBOX_BOTTOMRIGHT].x;
   rcl->yBottom = naptl[TXTBOX_BOTTOMRIGHT].y;

   rcl->yTop     += pptl->y;
   rcl->xLeft    += pptl->x;
   rcl->xRight   += pptl->x;
   rcl->yBottom  += pptl->y;
   /*
   ** We need pixels so convert....
   */
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);

}


BOOL DrwTextInp ( HWND hwnd, MPARAM mp1, MPARAM mp2, WINDOWINFO *pwi)
{
   BOOL   fProcessed;  /* Flag is key is pocessed.             */
   POINTL ptl;         /* The point we get from OS/2....       */

   fProcessed = FALSE;
   /*
   ** Process some virtual keys
   */

   if (SHORT1FROMMP(mp1) & KC_VIRTUALKEY)
   {
      switch (SHORT2FROMMP(mp2))
      {
         case VK_BACKSPACE:
            fProcessed = TRUE;
            if (Index > 0)
            {
               WinShowCursor (pwi->hwndClient, FALSE) ;
               UpdateInputString(pwi,FALSE);
               Index--;
               ptlCurs.x = aptl[Index].x;
               ptlCurs.y = aptl[Index].y;
               GpiSetCurrentPosition(pwi->hps,&ptlCurs);
               Inputstr[Index]=0;
               DrawInputString(pwi);
               WinShowCursor (pwi->hwndClient, TRUE);
            }
            break ;

         /*
         **  Newline and Enter keys
         */

         case VK_NEWLINE:
         case VK_ENTER:
            /*
            ** If there is text inserted e.g our index > 0.
            */

            if (SegmOpen && Index > 0)
            {
               CloseTextSegment(pTxt,Inputstr,pwi);
               SegmOpen=FALSE;
               Index = 0;
               Inputstr[0]=0;
            }
            /*
            ** 1 - Go on line down
            ** 2 - Set cursor at the xStart position.
            */
            ptlCurs.y -= pwi->fattrs.lMaxBaselineExt;
            checkScrollbars(pwi,ptlCurs);

            ptlStCsr.y = ptlCurs.y;
            ptlCurs.x  = ptlStCsr.x;
            GpiSetCurrentPosition(pwi->hps,&ptlCurs);
            fProcessed=TRUE;
            break;

         case VK_ESC:
            if (SegmOpen && Index > 0)
            {
               CloseTextSegment(pTxt,Inputstr,pwi);
            }
            SegmOpen = FALSE;
            Index = 0;
            Inputstr[0]=0;
            WinShowCursor(pwi->hwndClient,FALSE);
            WinDestroyCursor(pwi->hwndClient);
            break;
      }
   }  /* EOF Process virtual keys */

   /*------------------------
     Process character keys
   ------------------------*/

   if (!fProcessed && SHORT1FROMMP(mp1) & KC_CHAR)
   {
      if (!SegmOpen)
      {
         pTxt = OpenTextSegment(ptlCurs,pwi);
         SegmOpen=TRUE;
      }

      if (Index < 120 && SegmOpen)
      {
         WinShowCursor (hwnd, FALSE) ;
         Inputstr[Index++]=(CHAR)SHORT1FROMMP(mp2);
         Inputstr[Index]=0;
         UpdateInputString(pwi,TRUE);
         WinShowCursor (pwi->hwndClient, TRUE) ;
         fProcessed = TRUE ;
      }
      else
      {
         DosBeep(880,20);
         return TRUE;
      }


   }
   /*--------------------------------
    Process remaining virtual keys
    --------------------------------*/

   if (fProcessed)
   {
      GpiQueryCurrentPosition(pwi->hps,&ptl);
      aptl[Index].x = ptl.x;
      aptl[Index].y = ptl.y;
      ptlCurs.x = ptl.x;
      ptlCurs.y = ptl.y;
      checkScrollbars(pwi,ptlCurs);
      GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptl);
      WinCreateCursor(pwi->hwndClient,ptl.x,ptl.y,0,0,CURSOR_SETPOS,NULL);
   }
   return fProcessed;
}
/*------------------------------------------------------------------------*/
void SetCursorStartPosition(HWND hwnd,WINDOWINFO *pwi,POINTL *ptl)
{
   ptlStCsr.x = ptl->x;
   ptlStCsr.y = ptl->y;
   ptlCurs.x  = ptl->x;
   ptlCurs.y  = ptl->y;

   aptl[0].x  = ptl->x;
   aptl[0].y  = ptl->y;

   if (SegmOpen && Index > 0)
   {
     CloseTextSegment(pTxt,Inputstr,pwi);
   }
   SegmOpen = FALSE;
   Index = 0;
   Inputstr[0]=0;

   GpiSetCurrentPosition(pwi->hps,ptl);
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,ptl);
   WinCreateCursor(pwi->hwndClient,ptl->x,ptl->y,0,0,
                   CURSOR_SETPOS,NULL);
   WinShowCursor (pwi->hwndClient, TRUE);
}
/*------------------------------------------------------------------------*/
void UpdateInputString(WINDOWINFO *pwi,BOOL direct)
{
   RECTL rcl;

   CalcStringInvArea(pwi,&ptlStCsr,Inputstr,&rcl);
   WinFillRect(pwi->hps,&rcl,pwi->lBackClr);

   if (direct)
   {
      GpiSetMix(pwi->hps,FM_OVERPAINT);
      GpiSetColor(pwi->hps,pwi->ulColor);
      GpiCharStringAt(pwi->hps,&ptlStCsr,(LONG)strlen(Inputstr),(PSZ)Inputstr);
   }

   if (pTxt && pwi->ulColor != pTxt->bt.fColor )
      pTxt->bt.fColor = pwi->ulColor;
}
/*------------------------------------------------------------------------*/
/* WM_PAINT on textinput.....                                             */
/*------------------------------------------------------------------------*/
void DrawInputString(WINDOWINFO *pwi)
{
  GpiSetMix(pwi->hps,FM_OVERPAINT);
  GpiSetColor(pwi->hps,pwi->ulColor);
  GpiCharStringAt(pwi->hps,&ptlStCsr,(LONG)strlen(Inputstr),(PSZ)Inputstr);
}
/*------------------------------------------------------------------------*/
/* Create a cursor for text input.                                        */
/*------------------------------------------------------------------------*/
void Createcursor(WINDOWINFO *pwi)
{
   POINTL ptl;

   ptl.x = CURSORCX;
   ptl.y = CURSORCY;

   if (GpiQueryCharSet(pwi->hps)!= pwi->lcid)
      setFont(pwi,&pwi->fattrs,pwi->sizfx);

//   GpiQueryFontMetrics(pwi->hps,
//                       (LONG)sizeof(FONTMETRICS),&pwi->fontmetrics);

   if (ptl.y < pwi->fattrs.lMaxBaselineExt)
      ptl.y = pwi->fattrs.lMaxBaselineExt;

   /*
   ** Convert from 0.1 mm to pixels.
   */
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptl);

   WinCreateCursor(pwi->hwndClient,0,0,
                   ptl.x,                /* CX */
                   ptl.y,                /* CY */
                   CURSOR_FLASH,
                   NULL);
}
/*------------------------------------------------------------------------*/
/*  Name: breakInputString.                                               */
/*                                                                        */
/*  Description : Breaks the inputstring during editing.                  */
/*                Called whenever a user presses the Ctrl-I key           */
/*                combination while in input mode.                        */
/*                Works completely on the local globals defined in this   */
/*                module.                                                 */
/*------------------------------------------------------------------------*/
void breakInputString( WINDOWINFO *pwi )
{
   POINTL ptl;
   /*
   ** If there is text inserted e.g our index > 0.
   */
   if (SegmOpen && Index > 0 && pTxt)
   {
      GpiQueryCurrentPosition(pwi->hps,&ptl);

      ptlStCsr.x = ptl.x;
      ptlStCsr.y = ptl.y;
      ptlCurs.x  = ptl.x;
      ptlCurs.y  = ptl.y;
      aptl[0].x  = ptl.x;
      aptl[0].y  = ptl.y;

      CloseTextSegment(pTxt,Inputstr,pwi);
      SegmOpen    = FALSE;
      pTxt        = NULL;
      Index       = 0;
      Inputstr[0] = 0;
    }
}
