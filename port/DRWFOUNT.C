/*------------------------------------------------------------------------*/
/*  drwfount.c                                                            */
/*                                                                        */
/*  Description : Contains all the functions and dlg procs for the        */
/*                fountain fill.                                          */
/*                                                                        */
/* public functions :                                                     */
/* fountainfill     : fills the given area with the fountain fill.        */
/* RegisterFountain : Registers the window used in the dialog.            */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdlib.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_clr.h"    /* For the color menu item routines.... */
#include "drwmenu.h"
#include "resource.h"

#define FILL_HORIZ 1
#define FILL_VERT  2

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#define FOUNT_TYPE_SQUARE   1
#define FOUNT_TYPE_CIRCULAR 2
#define FOUNT_TYPE_HORIZONTAL 3
#define FOUNT_TYPE_VERTICAL 4

#define MINCENTEROFF -50     /* Minimal center offset is -50% l */
#define MAXCENTEROFF  50     /* Minimal center offset is  50% l */

#define MINANGLE  -90
#define MAXANGLE   90


extern ULONG   ColorTab[];       /* SEE dlg_val.c                    */

static FOUNTAIN fnt;


typedef struct _fount
{
   long lRedSteps;                /* Number of steps in red to take. */
   long lGreenSteps;
   long lBlueSteps;
   long lGrStart,lGrEnd;         /* Start and end green              */
   long lRStart,lREnd;           /* Start and end red                */
   long lBStart,lBEnd;           /* Start and end blue.              */
   long lMaxSteps;               /* The max of all color steps       */
   BOOL bRepeatGreen;            /* Should we repeat green.....      */
   BOOL bRepeatRed;              /* Should we repeat red             */
   BOOL bRepeatBlue;             /* Should we repeat blue            */
} FOUNT;
/*-------------------------------------------------------------------------*/
static long getGradient(FOUNT *pGrd,ULONG ulStart,ULONG ulEnd,LONG lArea)
{
   long lStart = 0;

   pGrd->bRepeatGreen = FALSE;
   pGrd->bRepeatRed   = FALSE;
   pGrd->bRepeatBlue  = FALSE;

   pGrd->lRStart     =  (ulStart >> 16);
   pGrd->lREnd       =  (ulEnd >> 16);
   pGrd->lRedSteps   =  pGrd->lREnd - pGrd->lRStart;
   pGrd->lMaxSteps   =  abs(pGrd->lRedSteps);

   pGrd->lGrEnd      =   ( ulEnd & 0x0000FF00 );
   pGrd->lGrEnd    >>= 8;
   pGrd->lGrStart    =   ( ulStart & 0x0000FF00 );
   pGrd->lGrStart  >>= 8;
   pGrd->lGreenSteps =   pGrd->lGrEnd - pGrd->lGrStart;

   if (abs(pGrd->lGreenSteps) > pGrd->lMaxSteps )
      pGrd->lMaxSteps = abs(pGrd->lGreenSteps);

   pGrd->lBStart     = (ulStart & 0x000000FF );
   pGrd->lBEnd       = (ulEnd   &  0x000000FF);
   pGrd->lBlueSteps  = (ulEnd   &  0x000000FF) - (ulStart & 0x000000FF );

   if (abs(pGrd->lBlueSteps) > pGrd->lMaxSteps )
      pGrd->lMaxSteps = abs(pGrd->lBlueSteps);
   /*
   ** If there are more pixels to handle than color difference we have to
   ** center the overgang in the middle of the rectangle and fill the start
   ** and end with the given startcolor and endcolor.
   */
   if (pGrd->lMaxSteps < lArea)
   {
      lStart = (lArea - pGrd->lMaxSteps) /2;
   }

   if (abs(pGrd->lRedSteps) > 0  && lArea > abs(pGrd->lRedSteps))
   {
     pGrd->lRedSteps   = lArea / pGrd->lRedSteps;   /* iedere n pixels 1 ophogen */
     pGrd->bRepeatRed = TRUE;
   }
   else if (abs(pGrd->lRedSteps) > 0 && lArea < abs(pGrd->lRedSteps))
   {
     pGrd->lRedSteps  = pGrd->lRedSteps / lArea;    /* iedere pixel n ophogen    */
   }

   if (abs(pGrd->lGreenSteps) > 0 && lArea > abs(pGrd->lGreenSteps))
   {
     pGrd->lGreenSteps  = lArea / pGrd->lGreenSteps;
     pGrd->bRepeatGreen = TRUE;
   }
   else if (abs(pGrd->lGreenSteps) > 0 && lArea < abs(pGrd->lGreenSteps))
   {
     pGrd->lGreenSteps  = pGrd->lGreenSteps / lArea;
   }

   if (abs(pGrd->lBlueSteps) > 0 && lArea > abs(pGrd->lBlueSteps))
   {
     pGrd->lBlueSteps  = lArea / pGrd->lBlueSteps;
     pGrd->bRepeatBlue = TRUE;
   }
   else if (abs(pGrd->lBlueSteps) > 0 && lArea < abs(pGrd->lBlueSteps))
   {
     pGrd->lBlueSteps  = pGrd->lBlueSteps / lArea;
   }
   return lStart;
}
/*---------------------------------------------------------------------------*/
static void getNewRect(long lAngle, RECTL *pSource, RECTL *pTarget, POINTL *pCenter)
{
   float    fcx,fcy;
   float    fdx,fdy;
   long     lx;
   long     ly;

   *pTarget = *pSource;

   fcx = (float)(pSource->xRight - pSource->xLeft);
   fcx *= 1.41; /* max is cos 45 ... */
   fcy = (float)(pSource->yTop - pSource->yBottom);
   fcy *= 1.41; /* max is cos 45 ... */
   fdx = fcx /2;
   lx  = (long)fdx + 1;
   pTarget->xLeft  -= lx;
   pTarget->xRight += lx;
   fdy = fcy /2;
   ly  = (long)fdy + 1;
   pTarget->yBottom -= ly;
   pTarget->yTop    += ly;
   pCenter->x = pTarget->xLeft   + ( pTarget->xRight - pTarget->xLeft)/2;
   pCenter->y = pTarget->yBottom + ( pTarget->yTop - pTarget->yBottom)/2;
}
/*---------------------------------------------------------------------------*/
static void incColor(LONG *lStart, LONG lEnd, LONG lStep)
{
   if (lStep == 0)
      return;

   if (lStep < 0 && (*lStart + lStep) >= lEnd )
    {
          *lStart  += lStep;
    }
   else if (lStep > 0 && (*lStart + lStep) <= lEnd)
          *lStart  += lStep;
}
/*------------------------------------------------------------------------*/
/*  Name        : LinearFountFill                                         */
/*                                                                        */
/*  Description : Functions fills the given rectangle with a fountain     */
/*                type coloring.                                          */
/*                                                                        */
/*  Parameters  : HPS hps - presentation space to draw in.                */
/*                RECTL * - pointer to the square to fill.                */
/*                ULONG ulStart - Starting color in RGB format.           */
/*                ULONG ulEnd   - End color in RGB format.                */
/*                LONG  lOffset - Center offset (plus of minus 50 % )     */
/*                                                                        */
/*  Returns     : MRESULT.                                                */
/*------------------------------------------------------------------------*/
static MRESULT LinearFountFill(HPS hps, RECTL *prcl,
                      ULONG ulStart, ULONG ulEnd, LONG lOffset)
{
   FOUNT grd;
   long n;
   long i,yStart = 0;
   long yEnd;
   long lColor;
   POINTL ptl;
   long cyPels = prcl->yTop   - prcl->yBottom; /* nr of steps to take */
   long cxPels = prcl->xRight - prcl->xLeft;


   if (cxPels <= 0 || cyPels <= 0)
      return (MRESULT)0;


   yStart = getGradient(&grd,ulStart,ulEnd,cyPels);

   lOffset = (lOffset * cyPels)/100;

   GpiSetColor(hps,ulStart);
   ptl.x = prcl->xLeft;
   ptl.y = prcl->yBottom;
   GpiMove(hps,&ptl);
   ptl.x += cxPels;
   ptl.y += cyPels;
   GpiBox(hps,DRO_FILL,&ptl,0,0);

   i = yStart;


   if (yStart)
   {
      yEnd = cyPels - yStart;
   }
   else
      yEnd = cyPels;

    for (n=0; i < yEnd; i++,n++)
    {
       ptl.y = prcl->yBottom + i + lOffset;
       ptl.x = prcl->xLeft;
       if (ptl.y >= prcl->yBottom)
         GpiMove(hps,&ptl);
       ptl.x += cxPels;

       if (grd.bRepeatRed && !(n % grd.lRedSteps))
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);
       else if (!grd.bRepeatRed)
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);

       if (grd.bRepeatBlue && !(n % grd.lBlueSteps))
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);
       else if (!grd.bRepeatBlue)
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);

       if (grd.bRepeatGreen && !(n % grd.lGreenSteps))
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);
       else if (!grd.bRepeatGreen)
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);

       lColor  = (grd.lRStart << 16);
       lColor |= grd.lBStart;
       lColor |= (grd.lGrStart << 8);
       if (ptl.y >= prcl->yBottom)
       {
          GpiSetColor(hps, lColor);
          GpiBox(hps,DRO_FILL,&ptl,0,0);
       }
    }
    /*
    ** If the difference between the color is to small we fill the
    ** square out with the end color.
    */
    if (yStart)
    {
       ptl.y--;
       for (n = 0; n <= yStart; n++)
       {
          ptl.y++;
          ptl.x = prcl->xLeft;
          GpiMove(hps,&ptl);
          ptl.x += cxPels;
          GpiBox(hps,DRO_FILL,&ptl,0,0);
       }
    }

    if (lOffset < 0)
    {
      ptl.y--;
      for (n = lOffset; n <= 1; n++)
      {
          ptl.x = prcl->xLeft;
          GpiMove(hps,&ptl);
          ptl.x += cxPels;
          GpiBox(hps,DRO_FILL,&ptl,0,0);
          ptl.y++;
      }
    }
    return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name        : VerticalFountFill                                       */
/*                                                                        */
/*  Description : Functions fills the given rectangle with a fountain     */
/*                type coloring.                                          */
/*                                                                        */
/*  Parameters  : HPS hps - presentation space to draw in.                */
/*                RECTL * - pointer to the square to fill.                */
/*                ULONG ulStart - Starting color in RGB format.           */
/*                ULONG ulEnd   - End color in RGB format.                */
/*                LONG  lOffset - Center offset (plus of minus 50 % )     */
/*                                                                        */
/*  Returns     : MRESULT.                                                */
/*------------------------------------------------------------------------*/
static MRESULT VerticalFountFill(HPS hps, RECTL *prcl,
                      ULONG ulStart, ULONG ulEnd, LONG lOffset)
{
   FOUNT grd;
   long n;
   long i,xStart = 0;
   long xEnd;
   long lColor;
   POINTL ptl;

   long cyPels = prcl->yTop   - prcl->yBottom; /* nr of steps to take */
   long cxPels = prcl->xRight - prcl->xLeft;


   if (cxPels <= 0 || cyPels <= 0)
      return (MRESULT)0;

   lOffset = (lOffset * cxPels)/100;

   GpiSetColor(hps,ulStart);
   ptl.x = prcl->xLeft;
   ptl.y = prcl->yBottom;
   GpiMove(hps,&ptl);
   ptl.x += cxPels;
   ptl.y += cyPels;
   GpiBox(hps,DRO_FILL,&ptl,0,0);

   xStart = getGradient(&grd,ulStart,ulEnd,cxPels);

   i = xStart;

   if (xStart)
   {
      xEnd = cxPels - xStart;
   }
   else
      xEnd = cxPels;

    for (n=0; i < xEnd; i++,n++)
    {
       ptl.y = prcl->yBottom;
       ptl.x = prcl->xLeft + i + lOffset;
       if (ptl.x >= prcl->xLeft)
         GpiMove(hps,&ptl);
       ptl.y += cyPels;

       if (grd.bRepeatRed && !(n % grd.lRedSteps))
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);
       else if (!grd.bRepeatRed)
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);

       if (grd.bRepeatBlue && !(n % grd.lBlueSteps))
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);
       else if (!grd.bRepeatBlue)
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);

       if (grd.bRepeatGreen && !(n % grd.lGreenSteps))
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);
       else if (!grd.bRepeatGreen)
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);

       lColor  = (grd.lRStart << 16);
       lColor |=  grd.lBStart;
       lColor |= (grd.lGrStart << 8);
       if (ptl.x >= prcl->xLeft)
       {
          GpiSetColor(hps, lColor);
          GpiBox(hps,DRO_FILL,&ptl,0,0);
       }
    }
    /*
    ** If the difference between the color is to small we fill the
    ** square out with the end color.
    */
    if (xStart)
    {
       ptl.x--;
       for (n = 0; n <= xStart; n++)
       {
          ptl.x++;
          ptl.y = prcl->yBottom;
          GpiMove(hps,&ptl);
          ptl.y += cyPels;
          GpiBox(hps,DRO_FILL,&ptl,0,0);
       }
    }

    if (lOffset < 0)
    {
      ptl.x--;
      for (n = lOffset; n <= 1; n++)
      {
          ptl.y = prcl->yBottom;
          GpiMove(hps,&ptl);
          ptl.y += cyPels;
          GpiBox(hps,DRO_FILL,&ptl,0,0);
          ptl.x++;
      }
    }
    return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name        : CircularFountFill                                       */
/*                                                                        */
/*  Description : Functions fills the given rectangle with a fountain     */
/*                type coloring.                                          */
/*                                                                        */
/*  Parameters  : HPS hps - presentation space to draw in.                */
/*                RECTL * - pointer to the square to fill.                */
/*                ULONG ulStart - Starting color in RGB format.           */
/*                ULONG ulEnd   - End color in RGB format.                */
/*                LONG  lOffset - Offset in a percentage (-50% -- 50%)    */
/*                                                                        */
/*  Returns     : MRESULT.                                                */
/*------------------------------------------------------------------------*/
static MRESULT SquareFountFill(HPS hps, RECTL *prcl,
                        ULONG ulStart, ULONG ulEnd, LONG lxOffset,LONG lyOffset)
{
   FOUNT grd;
   long n;
   long i;
   long yEnd,xEnd;
   long lColor;
   POINTL ptl1,ptl2;
   POINTL ptl;

   long cyPels = prcl->yTop   - prcl->yBottom; /* nr of steps to take */
   long cxPels = prcl->xRight - prcl->xLeft;

   if (cxPels <= 0 || cyPels <= 0)
      return (MRESULT)0;

   i = 0;

   if (lxOffset)
      lxOffset = (lxOffset * cxPels)/100;
   if (lyOffset)
      lyOffset = (lyOffset * cyPels)/100;

   GpiSetColor(hps,ulStart);
   ptl.x = prcl->xLeft;
   ptl.y = prcl->yBottom;
   GpiMove(hps,&ptl);
   ptl.x += cxPels;
   ptl.y += cyPels;
   GpiBox(hps,DRO_FILL,&ptl,0,0);

   getGradient(&grd,ulStart,ulEnd,cyPels);

   yEnd = cyPels/2;
   xEnd = cxPels/2;
   ptl1.x  = -1;
   ptl2.x = cxPels;

   grd.lGreenSteps *=2;
   grd.lRedSteps   *=2;
   grd.lBlueSteps  *=2;

    for (n=0; i < yEnd; i++,n++)
    {
       ptl1.y = i;
       if (ptl1.x < xEnd)
          ptl1.x++;

       ptl = ptl1;
       ptl.x += prcl->xLeft + lxOffset;
       ptl.y += prcl->yBottom + lyOffset;

       GpiMove(hps,&ptl);

       if (grd.bRepeatRed && !(n % grd.lRedSteps))
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);
       else if (!grd.bRepeatRed)
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);

       if (grd.bRepeatBlue && !(n % grd.lBlueSteps))
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);
       else if (!grd.bRepeatBlue)
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);

       if (grd.bRepeatGreen && !(n % grd.lGreenSteps))
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);
       else if (!grd.bRepeatGreen)
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);

       lColor  = (grd.lRStart << 16);
       lColor |= grd.lBStart;
       lColor |= (grd.lGrStart << 8);

       GpiSetColor(hps, lColor);

       ptl2.y = cyPels - i;
       if (ptl2.x > xEnd)
          ptl2.x--;

       ptl = ptl2;
       ptl.x += prcl->xLeft + lxOffset;
       ptl.y += prcl->yBottom + lyOffset;
       GpiBox(hps,DRO_FILL,&ptl,0,0);
    }
    return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name        : CircularFountFill                                       */
/*                                                                        */
/*  Description : Functions fills the given rectangle with a fountain     */
/*                type coloring.                                          */
/*                                                                        */
/*  Parameters  : HPS hps - presentation space to draw in.                */
/*                RECTL * - pointer to the square to fill.                */
/*                ULONG ulStart - Starting color in RGB format.           */
/*                ULONG ulEnd   - End color in RGB format.                */
/*                LONG  lOffset - Offset in a percentage (-50% -- 50%)    */
/*                                                                        */
/*  Returns     : MRESULT.                                                */
/*------------------------------------------------------------------------*/
static MRESULT CircularFountFill(HPS hps, RECTL *prcl,
                        ULONG ulStart, ULONG ulEnd, LONG lxOffset,LONG lyOffset)
{
   FOUNT grd;
   long n;
   long i;
   long yEnd,xEnd;
   long lColor;
   POINTL ptl;
   POINTL ptlCenter;
   ARCPARAMS arcp = {0,0,0,0};

   long cyPels = prcl->yTop   - prcl->yBottom; /* nr of steps to take */
   long cxPels = prcl->xRight - prcl->xLeft;

   if (cxPels <= 0 || cyPels <= 0)
      return (MRESULT)0;

   ptlCenter.x = cxPels/2;
   ptlCenter.y = cyPels/2;

   ptlCenter.x += prcl->xLeft;
   ptlCenter.y += prcl->yBottom;

   GpiSetColor(hps,ulStart);
   ptl.x = prcl->xLeft;
   ptl.y = prcl->yBottom;
   GpiMove(hps,&ptl);
   ptl.x = prcl->xLeft + cxPels;
   ptl.y = prcl->yBottom + cyPels;
   GpiBox(hps,DRO_FILL,&ptl,0,0);

   i = 0;

   if (lxOffset)
      lxOffset = (lxOffset * cxPels)/100;
   if (lyOffset)
      lyOffset = (lyOffset * cyPels)/100;


   ptlCenter.x += lxOffset;
   ptlCenter.y += lyOffset;


   getGradient(&grd,ulStart,ulEnd,cyPels);

   yEnd = cyPels/2;
   xEnd = cxPels/2;
   ptl.x  = xEnd;
   ptl.y =  yEnd;

   if (ptl.x > ptl.y)
      ptl.x = ptl.y;
   else
      ptl.y = ptl.x;

   grd.lGreenSteps *=2;
   grd.lRedSteps   *=2;
   grd.lBlueSteps  *=2;

   for (n=0; i < yEnd; i++,n++)
   {
      if (ptl.x > 0)
         ptl.x--;

       if (ptl.y > 0)
          ptl.y--;


       arcp.lP =  ptl.x;
       arcp.lQ =  ptl.y;

       GpiSetCurrentPosition(hps,&ptlCenter);

       if (grd.bRepeatRed && !(n % grd.lRedSteps))
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);
       else if (!grd.bRepeatRed)
          incColor(&grd.lRStart,grd.lREnd,grd.lRedSteps);

       if (grd.bRepeatBlue && !(n % grd.lBlueSteps))
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);
       else if (!grd.bRepeatBlue)
          incColor(&grd.lBStart,grd.lBEnd,grd.lBlueSteps);

       if (grd.bRepeatGreen && !(n % grd.lGreenSteps))
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);
       else if (!grd.bRepeatGreen)
          incColor(&grd.lGrStart,grd.lGrEnd,grd.lGreenSteps);

       lColor  = (grd.lRStart << 16);
       lColor |= grd.lBStart;
       lColor |= (grd.lGrStart << 8);

       GpiSetColor(hps, lColor);
       GpiSetArcParams(hps,&arcp);
       GpiFullArc(hps,DRO_FILL,MAKEFIXED(1,0));
    }
    return (MRESULT)0;
}
/*--------------------------------------------------------------------------*/
/* FountainWndProc.                                                         */
/*                                                                          */
/* Description  : Window procedure for the window in the dialog.            */
/*                In this window the results are shown.                     */
/*--------------------------------------------------------------------------*/
MRESULT EXPENTRY FountainWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   HPS    hps;
   RECTL  rcl;
   MATRIXLF matlf;
   POINTL   ptlCenter;
   static USHORT cxClient,cyClient;

   switch(msg)
   {
      case WM_CREATE:
         WinQueryWindowRect(hwnd,&rcl);
         cxClient = rcl.xRight - rcl.xLeft;
         cyClient = rcl.yTop - rcl.yBottom;
         return (MRESULT)0;

      case WM_PAINT:
         WinQueryWindowRect(hwnd,&rcl);
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiErase(hps);
         GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         rcl.xLeft   = 0;
         rcl.xRight  = cxClient;
         rcl.yBottom = 0;
         rcl.yTop    = cyClient;
         switch (fnt.ulFountType)
         {
            case FOUNT_TYPE_SQUARE:
               SquareFountFill(hps,&rcl,fnt.ulStartColor,fnt.ulEndColor,fnt.lHorzOffset,fnt.lVertOffset);
               break;
            case FOUNT_TYPE_CIRCULAR:
               CircularFountFill(hps,&rcl,fnt.ulStartColor,fnt.ulEndColor,fnt.lHorzOffset,fnt.lVertOffset);
               break;
            case FOUNT_TYPE_VERTICAL:
               VerticalFountFill(hps,&rcl,fnt.ulStartColor,fnt.ulEndColor,fnt.lHorzOffset);
               break;
            default:
               if (fnt.lAngle)
               { 
                  getNewRect(fnt.lAngle,&rcl,&rcl,&ptlCenter);
                  GpiRotate(hps,&matlf,TRANSFORM_REPLACE,
                            MAKEFIXED(fnt.lAngle,0), &ptlCenter);
                  GpiSetModelTransformMatrix(hps,9L,
                                             &matlf,TRANSFORM_REPLACE);
               }
               LinearFountFill(hps,&rcl,fnt.ulStartColor,fnt.ulEndColor,fnt.lHorzOffset);
               break;
         }
         WinEndPaint(hps);
         return 0;
   }
   return (WinDefWindowProc(hwnd,msg,mp1,mp2));
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY FountainDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   SWP swp;                        /* Screen Window Position Holder    */
   static FOUNTAIN *ft;            /* Pointer to the fountain   struct */
   static HWND hwndClrMenu;
   static HWND hwndBtnTo;
   static HWND hwndBtnFrom;
   static HWND hFountain;          /* Shows the fountain fill.      */
   static POINTL ptlFrom,ptlTo;    /* Remember button pos for popup */
   static USHORT usLastButton;
   static BOOL bInit;

   POINTL ptl = { 0,0 };
   ULONG  ulColor;

   switch (msg)
   {
      case WM_INITDLG:
         bInit = TRUE;
         /* Centre dialog on the screen */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
         ((WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN) - swp.cx) / 2),
         ((WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - swp.cy) / 2),
         0, 0, SWP_MOVE);

         hwndClrMenu = WinLoadMenu(hwnd,0,IDM_FOUNTAINCOLOR);

         hwndBtnFrom = WinWindowFromID(hwnd,ID_PBFROM);
         WinQueryWindowPos(hwndBtnFrom, (PSWP)&swp);
         ptlFrom.x = swp.x;
         ptlFrom.y = swp.y + swp.cy;

         hwndBtnTo = WinWindowFromID(hwnd,ID_PBTO);
         WinQueryWindowPos(hwndBtnTo, (PSWP)&swp);
         ptlTo.x = swp.x;
         ptlTo.y = swp.y + swp.cy;

         hFountain = WinWindowFromID(hwnd,ID_FOUNTAINWND);

         ft = (FOUNTAIN *)mp2; /* Get our info.... */

         fnt.ulStartColor = ft->ulStartColor;
         fnt.ulEndColor   = ft->ulEndColor;
         fnt.lHorzOffset  = ft->lHorzOffset;
         fnt.lVertOffset  = ft->lVertOffset;
         fnt.lAngle       = ft->lAngle;
         /*
         ** Setup the colors in the buttons
         */
         WinSetPresParam(hwndBtnTo,PP_BACKGROUNDCOLOR,sizeof(RGB),
                         (void *)&fnt.ulEndColor);
         WinSetPresParam(hwndBtnFrom,PP_BACKGROUNDCOLOR,sizeof(RGB),
                         (void *)&fnt.ulStartColor);

         /*
         ** Setup the center offset spinbuttons
         */
         WinSendDlgItemMsg(hwnd,ID_SPINHORZ,SPBM_SETLIMITS,
                           MPFROMLONG(MAXCENTEROFF), MPFROMLONG(MINCENTEROFF));
         WinSendDlgItemMsg(hwnd,ID_SPINVERT,SPBM_SETLIMITS,
                           MPFROMLONG(MAXCENTEROFF), MPFROMLONG(MINCENTEROFF));

         WinSendDlgItemMsg(hwnd,ID_SPINHORZ,SPBM_SETCURRENTVALUE,
                           MPFROMLONG(fnt.lHorzOffset), NULL);
         WinSendDlgItemMsg(hwnd,ID_SPINVERT,SPBM_SETCURRENTVALUE,
                           MPFROMLONG(fnt.lVertOffset), NULL);


         WinSendDlgItemMsg(hwnd,ID_SPINANGLE,SPBM_SETLIMITS,
                           MPFROMLONG(MAXANGLE), MPFROMLONG(MINANGLE));

         WinSendDlgItemMsg(hwnd,ID_SPINANGLE,SPBM_SETCURRENTVALUE,
                           MPFROMLONG(fnt.lAngle), NULL);
         /*
         ** Set the appropraite radio button.
         */
         switch(fnt.ulFountType)
         {
            case FOUNT_TYPE_SQUARE:
               WinSendDlgItemMsg(hwnd,ID_RADSQR,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
            case FOUNT_TYPE_CIRCULAR:
               WinSendDlgItemMsg(hwnd,ID_RADCIR,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
            case FOUNT_TYPE_VERTICAL:
               WinSendDlgItemMsg(hwnd,ID_RADVERTICAL,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
            default:
               WinSendDlgItemMsg(hwnd,ID_RADHORIZ,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),TRUE );
               break;
         }
         bInit = FALSE;
         return (MRESULT)0;

      /*-------------------------------POPUPMENU DRAWING ------------------*/
      case WM_MEASUREITEM:
         return measureColorItem(mp2);

      case WM_DRAWITEM:
         return drawColorItem(mp2);

       case WM_CONTROL:
          switch (LOUSHORT(mp1))
          {
             case ID_RADCIR:
               fnt.ulFountType = FOUNT_TYPE_CIRCULAR;
               WinInvalidateRect (hFountain,NULL, FALSE);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
             case ID_RADHORIZ:
               fnt.ulFountType = FOUNT_TYPE_HORIZONTAL;
               WinInvalidateRect (hFountain,NULL, FALSE);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),TRUE );
               break;
             case ID_RADVERTICAL:
               fnt.ulFountType = FOUNT_TYPE_VERTICAL;
               WinInvalidateRect (hFountain,NULL, FALSE);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
             case ID_RADSQR:
               fnt.ulFountType = FOUNT_TYPE_SQUARE;
               WinInvalidateRect (hFountain,NULL, FALSE);
               WinEnableWindow(WinWindowFromID(hwnd,ID_SPINANGLE),FALSE );
               break;
            case ID_SPINHORZ:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINHORZ,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&fnt.lHorzOffset),MPFROM2SHORT(0,0));
                  if (fnt.lHorzOffset < 0)
                     fnt.lHorzOffset = max(MINCENTEROFF,fnt.lHorzOffset);
                  else
                     fnt.lHorzOffset = min(MAXCENTEROFF,fnt.lHorzOffset);

                  WinInvalidateRect (hFountain,NULL, FALSE);
               }
               break;
            case ID_SPINVERT:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINVERT,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&fnt.lVertOffset),MPFROM2SHORT(0,0));

                  if (fnt.lVertOffset < 0)
                     fnt.lVertOffset = max(MINCENTEROFF,fnt.lVertOffset);
                  else
                     fnt.lVertOffset = min(MAXCENTEROFF,fnt.lVertOffset);

                  WinInvalidateRect (hFountain,NULL, FALSE);
               }
               break;
            case ID_SPINANGLE:
               if (HIUSHORT(mp1) == SPBN_CHANGE && !bInit)
               {
                  WinSendDlgItemMsg(hwnd,ID_SPINANGLE,SPBM_QUERYVALUE,
                                     (MPARAM)((VOID *)&fnt.lAngle),MPFROM2SHORT(0,0));

                  if (fnt.lAngle < 0)
                     fnt.lAngle = max(MINANGLE,fnt.lAngle);
                  else
                     fnt.lAngle = min(MAXANGLE,fnt.lAngle);

                  WinInvalidateRect (hFountain,NULL, FALSE);
               }
               break;
          }
          return (MRESULT)0;

       case WM_COMMAND:
          switch(LOUSHORT(mp1))
          {
             case ID_PBFROM:
                usLastButton = LOUSHORT(mp1);
                ColorPopupMenu(hwnd,hwndClrMenu,ptlFrom);
                return (MRESULT)0;
             case ID_PBTO:
                usLastButton = LOUSHORT(mp1);
                ColorPopupMenu(hwnd,hwndClrMenu,ptlTo);
                return (MRESULT)0;
             case DID_OK:
                ft->ulStartColor = fnt.ulStartColor;
                ft->ulEndColor   = fnt.ulEndColor;
                ft->lHorzOffset  = fnt.lHorzOffset;
                ft->lVertOffset  = fnt.lVertOffset;
                ft->ulFountType  = fnt.ulFountType;
                ft->lAngle       = fnt.lAngle;
                WinDismissDlg(hwnd,DID_OK);
                return 0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               return 0;
            case DID_HELP:
               return 0;
         }

         if ( LOUSHORT(mp1) >= IDM_CLR1 && LOUSHORT(mp1) <= IDM_CLR40)
         {
            ulColor = ColorTab[ LOUSHORT(mp1) - IDM_CLR1 ];
            if (usLastButton == ID_PBTO)
            {
               WinSetPresParam(hwndBtnTo,PP_BACKGROUNDCOLOR,sizeof(RGB),
                               (void *)&ulColor);
               fnt.ulEndColor = ulColor;
               WinInvalidateRect (hFountain,NULL, FALSE);
            }
            else
            {
               WinSetPresParam(hwndBtnFrom,PP_BACKGROUNDCOLOR,sizeof(RGB),
                               (void *)&ulColor);
               WinInvalidateRect (hFountain,NULL, FALSE);
               fnt.ulStartColor = ulColor;

            }
         }
         return (MRESULT)0;

      case WM_DESTROY:
        WinDestroyWindow(hwndClrMenu);
        break;

   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*-----------------------------------------------[ public ]---------------*/
/*  Name        : FountainFill.                                           */
/*                                                                        */
/*  Description : Functions fills the given rectangle with a fountain     */
/*                type coloring.                                          */
/*                                                                        */
/*  Parameters  : HPS hps - presentation space to draw in.                */
/*                RECTL * - pointer to the square to fill.                */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
void FountainFill(WINDOWINFO *pwi,HPS hps,RECTL *prcl,FOUNTAIN *fnt)
{
   MATRIXLF matlf,matOrg;
   POINTL   ptlCenter;
   RECTL    rcl;

   if (pwi && pwi->bSuppress)
      return;

   switch (fnt->ulFountType)
   {
      case FOUNT_TYPE_SQUARE:
         SquareFountFill(hps,prcl,fnt->ulStartColor,fnt->ulEndColor,fnt->lHorzOffset,fnt->lVertOffset);
         break;
      case FOUNT_TYPE_CIRCULAR:
         CircularFountFill(hps,prcl,fnt->ulStartColor,fnt->ulEndColor,fnt->lHorzOffset,fnt->lVertOffset);
         break;
      case FOUNT_TYPE_VERTICAL:
         VerticalFountFill(hps,prcl,fnt->ulStartColor,fnt->ulEndColor,fnt->lHorzOffset);
         break;
      default:
         rcl = *prcl;
         if (fnt->lAngle)
         { 
            getNewRect(fnt->lAngle,prcl,&rcl,&ptlCenter);
            GpiQueryModelTransformMatrix(hps,9L,&matOrg);
            GpiRotate(hps,&matlf,TRANSFORM_REPLACE,
                      MAKEFIXED(fnt->lAngle,0), &ptlCenter);
            GpiSetModelTransformMatrix(hps,9L,
                                       &matlf,TRANSFORM_REPLACE);
         }
         LinearFountFill(hps,&rcl,fnt->ulStartColor,fnt->ulEndColor,fnt->lHorzOffset);
         if (fnt->lAngle)
         {
            GpiSetModelTransformMatrix(hps,9L,
                                       &matOrg,TRANSFORM_REPLACE);
         }
         break;
   }
   return;
}
/*---------------------------------------------------------------------------*/
/*  Description : Register the windowprocedures for the window living        */
/*                in the "fountain fill" dialog.                             */
/*---------------------------------------------------------------------------*/
void RegisterFountain(HAB hab)
{

   WinRegisterClass(hab,                    // Another block handle
                    (PSZ)"FOUNTAIN",        // Name of class being registered
                    (PFNWP)FountainWndProc, // Window procedure for class
                    CS_SIZEREDRAW,          // Class style
                    0L);                    // Extra bytes to reserve

   return;
}
