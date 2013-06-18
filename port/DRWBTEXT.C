/*------------------------------------------------------------------------*/
/*  Name: drwbtext.c                                                      */
/*                                                                        */
/*  Description : Functions for handling blocktxt objects.                */
/*                                                                        */
/* Public functions.                                                      */
/*                                                                        */
/* newBlockText        : Creates a new textblock object.                  */
/* free_BlockText      : Frees the textblock (destructor?)                */
/* BlockTextSelect     : Uses the mouse click to see if it is on *this    */
/* BlockTextMoveOutLine: Column dragging.                                 */
/* BlockTextStretch    : Object resizing.                                 */
/* BlockTextFaceName   : Set the font facename of the blocktext.          */
/* BlockTextEdit       : Edit text of given object.                       */
/* drawBlockText       : Draw the block text on screen / printer.         */
/* copyBlockTextObject : Copy constructor.                                */
/* BlockTextSelectable : Is the blocktext selectable?                     */
/* BlockTextPrepPrint  : Prepare printing. Calculation before print starts*/
/* BlockTextClosePrint : Cleanup printing structs.                        */
/* setColumnColor      : Sets the column color. [ call from drwmenu.c ]   */
/*                                                                        */
/*--[ private functions ]-------------------------------------------------*/
/*                                                                        */
/* del_ptextprint      : Deletes a print block. Made during prep print.   */
/* bTextMakePrintBlock : Makes a printerblock during print preparation.   */
/* newBlockText        : Creates a blocktext after dialog is confirmed.   */
/* free_BlockText      : Free's the memory used by the blocktext object.  */
/* PrintBlockText      : Prints a blocktext on paper.                     */
/* ---------------------------------------------------------------------- */
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
#include "drwbtext.h"
#include "dlg_fnt.h"
#include "dlg_hlp.h"
#include "drwutl.h"
#include "dlg_txt.h"
#include "drwfount.h"
#include "dlg_clr.h"
#include "resource.h"

#define MAX_MLEBUF 32000

#define MODE_NORMAL       1 /* Normal painting (screen & printer ) */
#define MODE_PREPPRINTING 2 /* Prepare for printing.               */
#define MODE_OUTLINE      3 /* Show outline only (draft printing)  */

#define CX_BORDER         10

typedef struct _defTextBlock
{
   PBLOCKTEXT  pT;
   BOOL        bEdit;
   BOOL        bDialogActive;
   char        *pszMleBuffer;
   WINDOWINFO *pwi;
} defTextBlock;

static defTextBlock defTBlock; /* Used in the dialog proc */
/*------------------------------------------------[ private ]------------*/
void  del_ptextprint(PBTEXTPRINT p)
{
   if (p->pszString)
      free((void *)p->pszString);
   free((void *)p);
   return;
}
/*------------------------------------------------[ private ]------------*/
/* bTextMakePrintBlock                                                   */
/*                                                                       */
/* Description  : Sets up a printing block. During the print preparation */
/*                The text is rendered on the screen to get for each line*/
/*                of text its starting point and the textline itself.    */
/*                For each line a printblock is added in a chain of      */
/*                blocks. During the print of a single blocktext object  */
/*                the chain of textlines is rendered on the printer.     */
/*                This method insures that we get real WYSIWYG.          */
/*-----------------------------------------------------------------------*/
static void bTextMakePrintBlock(POBJECT pObj,WINDOWINFO *pwi,FIXED fxBreak,
                                int iBreak, POINTL ptlStart,LONG lLength, char *pText)
{
   PBTEXTPRINT p;

   p = NULL;

   if (pObj->pvData)
   {
      p = (PBTEXTPRINT)pObj->pvData;
      while (p->next) p = p->next;
      p->next = (PBTEXTPRINT)calloc(sizeof(BTEXTPRINT),1);
      p = p->next;
      
   }
   else
   {
      p = (PBTEXTPRINT)calloc(sizeof(BTEXTPRINT),1);
      p->next = NULL;
      pObj->pvData = (void *)p;
   }

   p->pObj      = pObj;   /* To get access to the character width table. */
   p->ptlf.x    = (float)ptlStart.x; /* Get starting point of this text  */
   p->ptlf.x   /= (float)pwi->usWidth;
   p->ptlf.y    = (float)ptlStart.y;
   p->ptlf.y   /= (float)pwi->usHeight;
   if (lLength > 0)
   {
      p->pszString = calloc(lLength+1,1); /* Include 1 space for zero termination*/
      memcpy((void *)p->pszString,(void *)pText,lLength);
      p->fxBreakExtra = fxBreak;
      p->iBreakCount  = iBreak;
   }
   return;
}
/*------------------------------------------------[ private ]------------*/
/* PrintBlockText.                                                       */
/*                                                                       */
/* Description  : Print a block text on paper.                           */
/*                                                                       */
/* Returns      : NONE.                                                  */
/*-----------------------------------------------------------------------*/
static void  PrintBlockText(POBJECT pObj,WINDOWINFO *pwi)
{
   PBTEXTPRINT p,pTmp;
   POINTL      ptl;

   if (!pObj->pvData)
      return;

   p = (PBTEXTPRINT)pObj->pvData;

   do
   {
      ptl.x = (LONG)(p->ptlf.x * pwi->usFormWidth);
      ptl.y = (LONG)(p->ptlf.y * pwi->usFormHeight);
      GpiSetCurrentPosition(pwi->hps,&ptl);
      if (p->pszString)
         drawCharString(pwi,(LONG)strlen(p->pszString),
                        p->pszString,
                        pObj->lWidth,
                        p->fxBreakExtra,
                        p->iBreakCount);
      pTmp = p;
      p = p->next;
      del_ptextprint(pTmp); /* Kill this printer block */
   } while (p);

   pObj->pvData = NULL;
}
/*------------------------------------------------[ private ]------------*/
static void getColumnOutline(PBLOCKTEXT pT, RECTL *prcl, WINDOWINFO *pwi, 
                             int iColumn,BOOL bPreparePrinting)
{
   if (bPreparePrinting)
   {
      prcl->xLeft   = (LONG)(pT->rclf[iColumn].xLeft  * pwi->usWidth);
      prcl->xRight  = (LONG)(pT->rclf[iColumn].xRight * pwi->usWidth);
      prcl->yTop    = (LONG)(pT->rclf[iColumn].yTop   * pwi->usHeight);
      prcl->yBottom = (LONG)(pT->rclf[iColumn].yBottom* pwi->usHeight);
   }
   else
   {
      prcl->xLeft   = (LONG)(pT->rclf[iColumn].xLeft  * pwi->usFormWidth);
      prcl->xRight  = (LONG)(pT->rclf[iColumn].xRight * pwi->usFormWidth);
      prcl->yTop    = (LONG)(pT->rclf[iColumn].yTop   * pwi->usFormHeight);
      prcl->yBottom = (LONG)(pT->rclf[iColumn].yBottom* pwi->usFormHeight);
   }
   return;
}
/*-----------------------------------------------[ private ]-------------*/
static blocktext * newBlockText(POINTL ptlStart, WINDOWINFO *pwi)
{
   blocktext *pText;
   POBJECT    pObj;

   pText = (blocktext *)pObjNew(pwi,CLS_BLOCKTEXT);

   pText->ustype         = CLS_BLOCKTEXT;
   pText->nAlign         = ALIGN_LEFT;   /* Text alignment in column. */
   pText->nSpace         = SPACE_SINGLE; /* Spacing.                  */
   memcpy(&pText->fattrs,&pwi->fattrs,sizeof(FATTRS));
   pText->sizfx.fcx    = (float) pwi->sizfx.cx;
   pText->sizfx.fcy    = (float) pwi->sizfx.cy;
   pText->sizfx.fcx   /= (float)pwi->usFormWidth;
   pText->sizfx.fcy   /= (float)pwi->usFormHeight;
   pText->lTextlen     = 0;
   pText->pszText      = NULL;
   pText->rclf[0].xLeft =  (float)50;
   pText->rclf[0].xRight=  (float)250;
   pText->rclf[0].yTop  =  (float)(pwi->usFormHeight - 250);
   pText->rclf[0].yBottom =(float)(pwi->usFormHeight - 500);
   pText->rclf[0].xLeft  /= (float)pwi->usFormWidth;
   pText->rclf[0].xRight /= (float)pwi->usFormWidth;
   pText->rclf[0].yTop   /= (float)pwi->usFormHeight;
   pText->rclf[0].yBottom/= (float)pwi->usFormHeight;
   /*
   ** Do no show any column by default
   */
   pText->bt.line.LineType   = LINETYPE_INVISIBLE;
   pText->bt.lPattern        = PATSYM_DEFAULT;
   /*
   ** methods...
   */
   pObj = (POBJECT)pText;
   pObj->moveOutline           = BlockTextMoveOutLine; 
   pObj->paint                 = drawBlockText;
   pObj->getInvalidationArea   = BlockTextInvArea;

   return pText;
}
/*---------------------------------------------------------------------------*/
void free_BlockText(POBJECT pObj)
{
   blocktext *pText;

   if (!pObj)
      return;

   pText = (blocktext *)pObj;

   if (pText->pszText)
      free((void *)pText->pszText);
   free(pObj);
}
/*-----------------------------------------------[ public ]---------------*/
/*  Name: copyBlockTextObject.                                            */
/*                                                                        */
/*  Make a copy of the given blocktextobject and returns the reference    */
/*  to the new created object on success. Otherwise NULL.                 */
/*------------------------------------------------------------------------*/
POBJECT copyBlockTextObject( POBJECT pObj)
{
   POBJECT    pCopy;
   PBLOCKTEXT pOrg = (PBLOCKTEXT)pObj;
   PBLOCKTEXT pTextCopy;

   if (!pObj)
      return NULL;
   if (pObj->usClass != CLS_BLOCKTEXT)
      return NULL;

   pCopy = pObjNew(NULL, CLS_BLOCKTEXT);

   memcpy(pCopy,pObj,ObjGetSize(CLS_BLOCKTEXT));

   pCopy->bMultiSel = FALSE;

   pTextCopy = (PBLOCKTEXT)pCopy;

   pTextCopy->pszText = strdup(pOrg->pszText);
   return pCopy;
}
/*----------------------------------------------------------------------------*/
VOID * BlockTextSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj)
{
   blocktext *pT = (blocktext *)pObj;
   RECTL     rcl;
   int       i;

   if (pT->bt.usLayer == pwi->uslayer || pwi->bSelAll)
   {
      for (i = 0; i <= pT->usColumns; i++)
      {
         getColumnOutline(pT,&rcl,pwi,i,FALSE);
         if (ptl.x > rcl.xLeft && ptl.x < rcl.xRight)
            if (ptl.y > rcl.yBottom && ptl.y < rcl.yTop)
            {
              pObj->iSelected = i; /* remember selected column */
              return (void *)pT;
            }
      }
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/* Set the new position of our blocktext.                         --------*/
/*------------------------------------------------------------------------*/
void MoveBlockText(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   blocktext *pT = (blocktext *)pObj;
   float fdx,fdy,fcx,fcy;

   if (!pT)
      return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;

   pT->rclf[0].xLeft   += (fdx /fcx);
   pT->rclf[0].xRight  += (fdx /fcx);
   pT->rclf[0].yTop    += (fdy /fcy);
   pT->rclf[0].yBottom += (fdy /fcy);
}
/*------------------------------------------------------------------------*/
void BlockTextMoveOutLine(POBJECT pObj,WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   blocktext *pT = (blocktext *)pObj;
   POINTL ptl1,ptl2;
   RECTL  rcl;

   if (!pObj)
      return;

   rcl.xLeft   = pT->rclf[0].xLeft  * pwi->usFormWidth;
   rcl.xRight  = pT->rclf[0].xRight * pwi->usFormWidth;
   rcl.yTop    = pT->rclf[0].yTop   * pwi->usFormHeight;
   rcl.yBottom = pT->rclf[0].yBottom* pwi->usFormHeight;

   ptl1.x = rcl.xLeft;
   ptl1.y = rcl.yTop;

   ptl1.y += (LONG)dy;
   ptl1.x += (LONG)dx;
 
   ptl2.x = rcl.xRight;
   ptl2.y = rcl.yBottom;

   ptl2.y += (LONG)dy;
   ptl2.x += (LONG)dx;

   GpiMove(pwi->hps,&ptl1);
   GpiBox(pwi->hps,DRO_OUTLINE,&ptl2,0,0);
   return;
}
/*---------------------------------------------------------------------------*/
void BlockTextOutLine(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi)
{
   blocktext *pT = (blocktext *)pObj;

   prcl->xLeft   = pT->rclf[0].xLeft  * pwi->usFormWidth;
   prcl->xRight  = pT->rclf[0].xRight * pwi->usFormWidth;
   prcl->yTop    = pT->rclf[0].yTop   * pwi->usFormHeight;
   prcl->yBottom = pT->rclf[0].yBottom* pwi->usFormHeight;
   return;
}
/*---------------------------------------------------------------------------*/
void BlockTextInvArea(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi, BOOL bInc)
{
   BlockTextOutLine(pObj, prcl,pwi);

   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)prcl);

   if (bInc)
   {
      prcl->xLeft    -= HANDLESIZE;
      prcl->xRight   += HANDLESIZE;
      prcl->yBottom  -= HANDLESIZE;
      prcl->yTop     += HANDLESIZE;
   }
   return;
}
/*------------------------------------------------------------------------*/
/* BlockTextStretch.                                                      */
/*------------------------------------------------------------------------*/
void BlockTextStretch(POBJECT pObj,PRECTL prclNew,WINDOWINFO *pwi,ULONG ulMsg)
{
   POINTL ptl;
   blocktext *pT = (blocktext *)pObj;

   switch (ulMsg)
   {
      case WM_BUTTON1DOWN:
         break;
      case WM_BUTTON1UP:
         pT->rclf[0].xLeft  = (float)prclNew->xLeft;
         pT->rclf[0].xRight = (float)prclNew->xRight;
         pT->rclf[0].yTop   = (float)prclNew->yTop;
         pT->rclf[0].yBottom= (float)prclNew->yBottom;
         pT->rclf[0].xLeft  /= (float)pwi->usFormWidth;
         pT->rclf[0].xRight /= (float)pwi->usFormWidth;
         pT->rclf[0].yTop   /= (float)pwi->usFormHeight;
         pT->rclf[0].yBottom/= (float)pwi->usFormHeight;
         break;
      case WM_MOUSEMOVE:
         GpiSetLineType(pwi->hps,LINETYPE_DOT);
         ptl.x = prclNew->xLeft;
         ptl.y = prclNew->yBottom;
         GpiMove(pwi->hps, &ptl);
         ptl.x = prclNew->xRight;
         ptl.y = prclNew->yTop;
         GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0L,0L);
         break;
      default:
         break;
   }
   return;
}
/*-----------------------------------------------[ private ]-----------------*/
static BOOL paintColumn(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL rclCol,int iMode)
{
   POINTL ptl1,ptl2;
   RECTL rcl;
   blocktext *pT = (blocktext *)pObj;

   ptl1.x = rclCol.xLeft;
   ptl1.y = rclCol.yBottom;
   ptl2.x = rclCol.xRight;
   ptl2.y = rclCol.yTop;

   rcl = rclCol;

   if (pT->bt.lPattern == PATSYM_DEFAULT && 
       pT->bt.line.LineType == LINETYPE_INVISIBLE)
      return FALSE;

   if (iMode == MODE_PREPPRINTING)
   {
      /*
      ** During the print preparation we do not draw the column but
      ** we return true so the column painting can take this square
      ** into account.
      */
      return TRUE;
   }
   GpiSetLineType(hps,pT->bt.line.LineType);
   GpiSetPattern(hps, PATSYM_SOLID);
   GpiSetColor(hps,pT->ulColColor);

   if (pT->bt.lPattern == PATSYM_DEFAULT)
   {
      /*
      ** No filling
      */
      GpiSetColor(hps,pT->bt.line.LineColor);
      GpiMove(hps,&ptl1);
      GpiBox(hps,DRO_OUTLINE,&ptl2,0,0);
   }
   else if ( pT->bt.lPattern != PATSYM_GRADIENTFILL && 
             pT->bt.lPattern != PATSYM_FOUNTAINFILL )
   {
      /*
      ** we have a standard OS/2 patternfill.
      */
      GpiMove(hps,&ptl1);

      if (pT->bt.line.LineColor == pT->ulColColor )
      {
         GpiBox(hps,DRO_OUTLINEFILL, 
                &ptl2,0,0);
      }
      else
      {
         GpiMove(hps,&ptl1);
         /*
         ** Draw the filling part
         */
         GpiBox(hps,DRO_FILL, 
                &ptl2,0,0);
         /*
         ** Draw the outline
         */
         GpiMove(hps,&ptl1);
         GpiSetColor(hps,pT->bt.line.LineColor);

         GpiBox(hps,DRO_OUTLINE, 
                &ptl2,0,0);
      }
   }
   else if (pT->bt.lPattern == PATSYM_GRADIENTFILL || 
            pT->bt.lPattern == PATSYM_FOUNTAINFILL)
   {
      GpiBeginPath( hps, 1L);  /* define a clip path    */

      GpiMove(hps,&ptl1);
      GpiBox(hps,DRO_OUTLINE, 
             &ptl2,0,0);

      GpiEndPath(hps);
      GpiSetClipPath(hps,1L,SCP_AND);
      GpiSetPattern(hps,PATSYM_SOLID);
      if (pT->bt.lPattern == PATSYM_GRADIENTFILL)
         GradientFill(pwi,hps,&rcl,&pT->bt.gradient);
      else
         FountainFill(pwi,hps,&rcl,&pT->bt.fountain);

       GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */


       if (pT->bt.line.LineType !=  LINETYPE_INVISIBLE)
       {
         /*
         ** Draw the outline
         */
         GpiMove(hps,&ptl1);
         GpiSetColor(hps,pT->bt.line.LineColor);
         GpiBox(hps,DRO_OUTLINE, 
                &ptl2,0,0);
      }
   }
   return TRUE;
}
/*-----------------------------------------------[ private ]-----------------*/
/* paintText.                                                                */
/*                                                                           */
/* Description : Paints the text in the given HPS or does printer prep.      */
/*                                                                           */
/* Parameters  : HPS hps  - presentation space.                              */
/*               WINDOWINFO *pwi - reference to application context.         */
/*               POBJECT pObj - Object to be painted in HPS.                 */
/*               RECTL *rcl   - Rectangle of invalidated area (screen only!) */
/*               int iMode   - MODE_NORMAL | MODE_PREPPRINTING | MODE_OUTLINE*/
/*                                                                           */
/* Returns      : NONE.                                                      */
/*---------------------------------------------------------------------------*/
void paintText( HPS hps,WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl, int iMode )
{
   int        iBreakCount, iSurplus ;
   PCHAR      pStart, pEnd ;
   PCHAR      pText;
   POINTL     ptlStart, aptlTextBox [TXTBOX_COUNT] ;
   blocktext *pT;
   RECTL      rclColumn;
   SIZEF      sizfx;
   FIXED      fxBreakExtra;
   int        iLineBreak;
   int        iLen;
   iSurplus = 0;


   pT = (blocktext *)pObj;

   if (iMode != MODE_PREPPRINTING)
   {
      if (pwi->usdrawlayer != pT->bt.usLayer)
         return;
   }
   /*
   ** Just in case the text starts with a linebreak...
   */
   memset ((void *)aptlTextBox,0,( sizeof(POINTL) * TXTBOX_COUNT)); 
   /*
   ** Get column rectangle of first column..
   */
   getColumnOutline(pT,&rclColumn,pwi,0,(BOOL)(iMode == MODE_PREPPRINTING));

   pText = (PCHAR)pT->pszText;
     
   ptlStart.y = rclColumn.yTop ;

   if (!pText)
      return;

   if (prcl)
   {
      RECTL rclDest;
      /*
      ** prcl is only true for screen operations!
      ** For this moment we only check on the first column
      ** TODO !!! All columns
      */
      if (!WinIntersectRect(hab,&rclDest,prcl,&rclColumn))
         return;
   }

   if (iMode == MODE_PREPPRINTING)
   {
      sizfx.cx = pT->sizfx.fcx * pwi->usWidth;
      sizfx.cy = pT->sizfx.fcy * pwi->usHeight;
   }
   else
   {
      sizfx.cx = pT->sizfx.fcx * pwi->usFormWidth;
      sizfx.cy = pT->sizfx.fcy * pwi->usFormHeight;
   }

   setFont(pwi,&pT->fattrs,sizfx);

   if (iMode == MODE_PREPPRINTING)
      GpiQueryWidthTable(pwi->hps,0,MAX_NRWIDTH,pObj->lWidth);

  if (paintColumn(hps,pwi,pObj,rclColumn,iMode))
  {
     rclColumn.xLeft   += 20;
     rclColumn.yBottom += 20;
     rclColumn.xRight  -= 20;
     rclColumn.yTop    -= 20;
  }

   if (iMode != MODE_OUTLINE)
      GpiSetColor(hps,pT->bt.fColor);

   if (pwi->bPrinter)
   {
      /*
      ** Set the text on paper and get out!!
      */
      PrintBlockText(pObj,pwi);
      return;
   }


   do                                 // until end of text
   {
          iBreakCount  = 0;
          fxBreakExtra = 0;
          iLineBreak   = 0;

          while (*pText == ' ')         // Skip over leading blanks
               pText++ ;

          pStart = pText ;

          do                            // until line is known
               {
               iLineBreak   = 0;

               while (*pText == ' ')    // Skip over leading blanks
                    pText++ ;

                                        // Find next break point

               while (*pText != '\x00' && *pText != ' ' && 
                      *pText != '\r' && *pText != '\n')
                    pText++ ;

               if (*pText == '\r'|| *pText == '\n')
               {
                  pText++;
                  iLineBreak = 1;
                  if (*pText == '\n' || *pText == '\r')
                  {
                     pText++;
                     iLineBreak = 2;
                  }
               }
               /*
               ** A line with only a cariage return linefeed?
               */
               if ( ((pText - pStart) - iLineBreak) <= 0)
               {
                  pEnd  = pText ;
                  break;
               }
                                        // Determine text width

               GpiQueryTextBox (hps, (pText - pStart) - iLineBreak, pStart,
                                TXTBOX_COUNT, aptlTextBox) ;

                         // Normal case: text less wide than column

               if (aptlTextBox[TXTBOX_CONCAT].x < (rclColumn.xRight - rclColumn.xLeft))
                    {
                    iBreakCount++ ;
                    pEnd  = pText;
                    }

                         // Text wider than window with only one word

               else if (iBreakCount == 0)
                    {
                    pEnd  = pText ;
                    break ;
                    }

                         // Text wider than window, so fix up and get out
               else
                    {
                    iBreakCount-- ;
                    pText = pEnd ;
                    /*
                    ** Although we could have found a line break the
                    ** text did not fit the line at all. So...
                    */
                    iLineBreak = 0;
                    break ;
                    }
               }
          while (*pText != '\x00' && !iLineBreak) ;

                         // Get the final text box

          iLen = (int)(pEnd - pStart);
          if (iLen - iLineBreak > 0)
             GpiQueryTextBox (hps, (pEnd - pStart)-iLineBreak, pStart,
                              TXTBOX_COUNT, aptlTextBox) ;

                         // Drop down by maximum ascender

          ptlStart.y -= aptlTextBox[TXTBOX_TOPLEFT].y ;

                         // Find surplus space in text line

          iSurplus = rclColumn.xRight - rclColumn.xLeft -
                     aptlTextBox[TXTBOX_CONCAT].x ;

                        // Adjust starting position and
                        // space and character spacing

          switch (pT->nAlign)
               {
               case ALIGN_LEFT:
                    ptlStart.x = rclColumn.xLeft ;
                    break ;

               case ALIGN_RIGHT:
                    ptlStart.x = rclColumn.xLeft + iSurplus ;
                    break ;

               case ALIGN_CENTER:
                    ptlStart.x = rclColumn.xLeft + iSurplus / 2 ;
                    break ;

               case ALIGN_JUST:
                    ptlStart.x = rclColumn.xLeft ;

                    if (*pText == '\x00')
                         break ;

                    if (iBreakCount > 0)
                    {
                         fxBreakExtra = 65536 * iSurplus / iBreakCount;
                         GpiSetCharBreakExtra (hps,fxBreakExtra);
                    }
                    else if (pEnd - pStart - 1 > 0)
                    {
                         fxBreakExtra = 65536 * iSurplus / (pEnd - pStart - 1 - iLineBreak);
                         GpiSetCharExtra (hps,fxBreakExtra);
                    }
                    break ;
               }

                         // Drop down by maximum descender

          if (pT->nSpace != SPACE_NONE)
             ptlStart.y += aptlTextBox[TXTBOX_BOTTOMLEFT].y ;

                         // Display the string & return to normal
          if ((pEnd - pStart))
             GpiCharStringAt (hps, &ptlStart, (pEnd - pStart ) - iLineBreak, pStart) ;

          if (iMode == MODE_PREPPRINTING)
          {
             bTextMakePrintBlock(pObj,
                                 pwi,
                                 fxBreakExtra,
                                 iBreakCount,
                                 ptlStart,
                                 (LONG)((pEnd - pStart) -iLineBreak),
                                 pStart);
          }
          GpiSetCharExtra (hps, 0) ;
          GpiSetCharBreakExtra (hps, 0) ;

                         // Do additional line-spacing

          switch (pT->nSpace)
               {
               case SPACE_HALF:
                    ptlStart.y -= (aptlTextBox[TXTBOX_TOPLEFT].y -
                                   aptlTextBox[TXTBOX_BOTTOMLEFT].y) / 2 ;
                    break ;

               case SPACE_DOUBLE:
                    ptlStart.y -= aptlTextBox[TXTBOX_TOPLEFT].y -
                                  aptlTextBox[TXTBOX_BOTTOMLEFT].y ;
                    break ;
               }
          }

     while (*pText != '\x00' && ptlStart.y > rclColumn.yBottom) ;
}
/*-----------------------------------------------[ public ]----------*/
/*-------------------------------------------------------------------*/
void BlockTextDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer)
{
   blocktext *pT = (blocktext *)pObj;

   if (pwi->usdrawlayer == pT->bt.usLayer || bIgnorelayer)
      paintText(pwi->hps,pwi,pObj,NULL, MODE_OUTLINE);
   return;
}
/*-----------------------------------------------[ public ]----------*/
/* drawBlockText.                                                    */
/*-------------------------------------------------------------------*/
void drawBlockText(HPS hps,WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl)
{
   paintText(hps,pwi,pObj,prcl, MODE_NORMAL);
}
/*---------------------------------------------------------------------------*/
void BlockTextPrepPrint(POBJECT pObj, WINDOWINFO *pwi)
{
   paintText(pwi->hps,pwi,pObj,NULL,MODE_PREPPRINTING);
}
/*---------------------------------------------------------------------------*/
MRESULT EXPENTRY BlockTextDlgProc(HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SWP     swp;  /* Screen Window Position Holder */
   POBJECT pObj;
   IPT     lOffset; /* Insertion point MLE */
   ULONG   lBytes;
   LONG    lCursor;
   BOOL    bRet;
   SWP     *pswp;
   char    buf[100];
   static  HWND hText;    /* Multiline textbox     */
   static  SWP  swpText;  /* Size & pos of editbox */
   static  long cxborder,cyborder,cyTitlebar;

   switch (msg)
   {
      case WM_INITDLG:
                /* Centre dialog on the screen */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);
         pObj = (POBJECT)defTBlock.pT;
         pObj->bLocked = TRUE;

         cyborder = (LONG)WinQuerySysValue(HWND_DESKTOP,SV_CYSIZEBORDER);
         cxborder = (LONG)WinQuerySysValue(HWND_DESKTOP,SV_CXSIZEBORDER);
         cyTitlebar = (LONG)WinQuerySysValue(HWND_DESKTOP,SV_CYTITLEBAR);

         defTBlock.pszMleBuffer = (char *)calloc(MAX_MLEBUF,1);

         hText = WinWindowFromID(hwnd, ID_BTEXTMLE);
         WinQueryWindowPos(hText,(PSWP)&swpText);

         if (defTBlock.pT->pszText)
         {
            lOffset =  0;
            lCursor = -1;
            lBytes  = defTBlock.pT->lTextlen -1; /* \0 must be removed!!*/
            /*
            ** Copy text from the texblock object to the Multiline editbox.
            */
            WinSendDlgItemMsg(hwnd,ID_BTEXTMLE,MLM_SETIMPORTEXPORT,
                              MPFROMP(defTBlock.pszMleBuffer),
                              MPFROMSHORT((USHORT)lBytes));
            if (lBytes < MAX_MLEBUF)
            {
               strcpy(defTBlock.pszMleBuffer,defTBlock.pT->pszText);
               WinSendDlgItemMsg(hwnd,ID_BTEXTMLE,MLM_IMPORT,
                                 MPFROMP(&lCursor),MPFROMP(&lBytes));
            }
            else
            {
               sprintf(buf,
                       "Text size exceeds the maximum allowed size of %d bytes",
                       MAX_MLEBUF);
               WinMessageBox(HWND_DESKTOP,defTBlock.pwi->hwndClient,
                             (PSZ)buf,
                             (PSZ)"Application Error",
                             0,
                             MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                             MB_ICONEXCLAMATION);
            }
         }
         break;
      case WM_WINDOWPOSCHANGED:
         pswp = (SWP *)mp1;
         if (pswp)
         {
            WinSetWindowPos(hText,
                            (HWND)0,
                            cxborder,swpText.y,     /* x,y position */
                            pswp->cx - ( 2 * cxborder),
                            pswp->cy - ( swpText.y + cyborder + cyTitlebar),
                            SWP_MOVE | SWP_SIZE);
         }
         break;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               bRet = TRUE;  /* Successfull ending?*/
               defTBlock.bDialogActive = FALSE;
               pObj = (POBJECT)defTBlock.pT;
               pObj->bLocked = FALSE;

               lBytes = (ULONG)WinSendDlgItemMsg(hwnd,ID_BTEXTMLE,
                                                  MLM_QUERYFORMATTEXTLENGTH,
                                                  MPFROMLONG(0L),MPFROMLONG((-1)));
               if (lBytes)
               {
                  if (defTBlock.pT->pszText)
                     free((void *)defTBlock.pT->pszText);
                  defTBlock.pT->lTextlen = lBytes+1;
                  defTBlock.pT->pszText  = calloc(lBytes+1,1);
                  lOffset = 0;
                  

                  WinSendDlgItemMsg(hwnd,ID_BTEXTMLE,MLM_SETIMPORTEXPORT,
                                    MPFROMP(defTBlock.pszMleBuffer),
                                    MPFROMSHORT((USHORT)MAX_MLEBUF));

                  WinPostMsg(defTBlock.pwi->hwndClient,
                             UM_ENDDIALOG,(MPARAM)pObj,(MPARAM)0);
               
                  lBytes = (LONG)WinSendDlgItemMsg(hwnd,ID_BTEXTMLE,MLM_EXPORT,
                                                   MPFROMP(&lOffset),
                                                   MPFROMP(&lBytes));
                  memcpy(defTBlock.pT->pszText,defTBlock.pszMleBuffer,lBytes);
               }
               else
                  bRet = FALSE; /* Close dialog with error! */

               if (defTBlock.pszMleBuffer)
                  free((void *)defTBlock.pszMleBuffer);
               defTBlock.pszMleBuffer = NULL;
               WinDismissDlg(hwnd,bRet);
               break;
            case DID_CANCEL:
               defTBlock.bDialogActive = FALSE;
               pObj = (POBJECT)defTBlock.pT;
               pObj->bLocked = FALSE;
               if (defTBlock.pszMleBuffer)
                  free((void *)defTBlock.pszMleBuffer);
               defTBlock.pszMleBuffer = NULL;
               WinDismissDlg(hwnd,FALSE);
               break;
            case DID_HELP:
               ShowDlgHelp(hwnd);
               return 0;
         }
         return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*--------------------------------------------------------------------------*/
void createBlockText(WINDOWINFO *pwi)
{
   POINTL ptl;
   HWND hOwner = pwi->hwndClient;

   if (!defTBlock.bDialogActive)
   {
      defTBlock.pwi  = pwi;
      defTBlock.bEdit= FALSE;
      defTBlock.pszMleBuffer = NULL;
      ptl.x = 0;
      ptl.y = 0;
      defTBlock.pT = (blocktext *)newBlockText(ptl,pwi);
      if (WinDlgBox(HWND_DESKTOP,hOwner,(PFNWP)BlockTextDlgProc,(HMODULE)0,
                 DLG_BLOCKTEXT,(PVOID)pwi))
      {
         pObjAppend((POBJECT)defTBlock.pT);
      }
      else
      {
         free_BlockText((POBJECT)defTBlock.pT);
      }
      defTBlock.pT = NULL;
   }
   return;
}
/*------------------------------------------------------------------------*/
void BlockTextFaceName(POBJECT pObj, char *pszFontname)
{
   blocktext *pT;
   pT = (blocktext *)pObj;
   strcpy(pT->fattrs.szFacename,pszFontname);
}
/*------------------------------------------------------------------------*/
BOOL BlockTextEdit(POBJECT pObj, WINDOWINFO *pwi)
{
   POINTL ptl;
   HWND hOwner = pwi->hwndClient;

   if (!defTBlock.bDialogActive)
   {
      defTBlock.pwi  = pwi;
      defTBlock.bEdit= FALSE;
      defTBlock.pszMleBuffer = NULL;
      ptl.x = 0;
      ptl.y = 0;
      defTBlock.pT = (blocktext *)pObj;
      WinDlgBox(HWND_DESKTOP,hOwner,(PFNWP)BlockTextDlgProc,(HMODULE)0,
                DLG_BLOCKTEXT,(PVOID)pwi);
   }
   return TRUE;
}
/*-----------------------------------------------[ public ]----------*/
/* BlockTextFontDlg     called in drwutl  before starting fontdlg.   */
/*-------------------------------------------------------------------*/
BOOL BlockTextFontDlg(POBJECT pObj, WINDOWINFO *pwi)
{
   blocktext *pT;
   BOOL      bResult;
   SIZEF     sizfx;
   pT = (blocktext *)pObj;
   bResult = FontDlg(pwi,&pT->fattrs,TRUE); /* Start the fontdialog.... */

   if (bResult && getFontDlgSizfx(&sizfx))
   { 
      pT->sizfx.fcx    = (float)sizfx.cx;
      pT->sizfx.fcy    = (float)sizfx.cy;
      pT->sizfx.fcx   /= (float)pwi->usWidth;
      pT->sizfx.fcy   /= (float)pwi->usHeight;
   }
   return bResult;
}
/*------------------------------------------------------------------------*/
/* FilePutBlockText.                                                      */
/*                                                                        */
/*  Description : Writes the variable part of the blocktext    to file.   */
/*                Could not be done in the general file function in       */
/*                drwutl.c since the the size of the struct is variable.  */
/*                                                                        */
/*                                                                        */
/*  Parameters : pLoadinfo - pointer to  filestructure defined in dlg.h   */
/*               POBJECT * - pointer to BlockTextStructure.               */
/*                                                                        */
/*  Return     : int: Number of bytes written to file or -1 on error.     */
/*------------------------------------------------------------------------*/
int FilePutBlockText(pLoadinfo pli, POBJECT pObj)
{
   PVOID p;
   int   i = 0;
   blocktext *pT;
   

   pT = (blocktext *)pObj;

   if (pT->lTextlen >  0)
   {
      p = (PVOID)pT->pszText;
      i = write(pli->handle,(PVOID)p,pT->lTextlen);
      if (i > 0)
      {
        write(pli->handle, (PVOID)EOF_BLOCKTEXT,strlen(EOF_BLOCKTEXT));
      }
   }
   return i;
}
/*------------------------------------------------------------------------*/
/* FileGetBlockText.                                                      */
/*------------------------------------------------------------------------*/
int FileGetBlockText(pLoadinfo pli, POBJECT pObj)
{
   ULONG ulBytes;
   int   i = 0;
   blocktext *pT;

   pT = (blocktext *)pObj;
   ulBytes = pT->lTextlen;

   if ( ulBytes >  0 )
   {
      pT->pszText = (char *)calloc((ULONG)ulBytes,sizeof(char));
      i = read( pli->handle,(PVOID)pT->pszText,ulBytes);
   }
   return i;
}
/*-----------------------------------------------------------------------*/
ULONG BlockTextGetAlign(POBJECT pObj)
{
   blocktext *pT = (blocktext *)pObj;
   return (ULONG)pT->nAlign;
}
/*-----------------------------------------------------------------------*/
void BlockTextSetAlign(POBJECT pObj,USHORT usAlign)
{
   blocktext *pT = (blocktext *)pObj;
   pT->nAlign = usAlign;
   return;
}
/*-----------------------------------------------------------------------*/
ULONG BlockTextGetSpacing(POBJECT pObj)
{
   blocktext *pT = (blocktext *)pObj;
   return (ULONG)pT->nSpace;
}
/*-----------------------------------------------------------------------*/
void BlockTextSetSpacing(POBJECT pObj,USHORT usSpace)
{
   blocktext *pT = (blocktext *)pObj;

   if (usSpace >= SPACE_NONE && usSpace <= SPACE_DOUBLE)
      pT->nSpace = usSpace;
   return;
}
/*-------------------------------------------------------------------------*/
/* showBlockTextFont.                                                      */
/*                                                                         */
/* Description  : Gets sizefx and fontname and calls the                   */
/*                showFontInStatusLine function in drwmain to get the stuff*/
/*                in the status line.                                      */
/*                                                                         */
/* Returns      : NONE.                                                    */
/*-------------------------------------------------------------------------*/
void showBlockTextFont(POBJECT pObj,WINDOWINFO *pwi)
{
   blocktext *pT = (blocktext *)pObj;
   SIZEF sizfx;

   sizfx.cx = (FIXED)(pT->sizfx.fcx * pwi->usWidth );
   sizfx.cy = (FIXED)(pT->sizfx.fcy * pwi->usHeight);
   showFontInStatusLine(sizfx,pT->fattrs.szFacename);
}
/*-----------------------------------------------[ public ]----------------*/
/* setColumnColor.                                                         */
/*                                                                         */
/* Description  : Sets the column color of the given blocktext object.     */
/*                Called whenever the user selects a color in the blocktext*/
/*                popupmenu. [ see drwmenu.c]                              */
/*-------------------------------------------------------------------------*/
void setColumnColor(POBJECT pObj, ULONG ulColor)
{
   blocktext *pT = (blocktext *)pObj;

   if (!pObj)
      return;

   if (pObj->usClass != CLS_BLOCKTEXT)
      return;
   pT->ulColColor = ulColor;

}