/*------------------------------------------------------------------------*/
/*  Name: DLG_CLR.C                                                       */
/*                                                                        */
/*  Description : Contains the code for the dialog which enable remixing  */
/*                of a doubleclicked color in the palette at the bottom   */
/*                of the application. Plus the stuff for gradientfill can */
/*                here be found.                                          */
/*                                                                        */
/* Functions:                                                             */
/* TrueClrDlgProc     : Dlgproc for dialog with three scrollbars for      */
/*                      mixing colors                                     */
/* RedGreenBlueWndProc: show result of the position of the scrollbars.    */
/* GradientDlgProc    : The gradient dialog procedure.                    */
/* GradientFill       : Fills a given rect on a given hps with a gradient */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_clr.h"
#include "dlg_hlp.h"
#include "drwmenu.h"
#include "resource.h"

#ifdef __IBMC__
#define ltoa _ltoa
#endif

#define MINSATURATION      10      /* Minimum color saturation 10% */
#define MAXSATURATION      100     /* Maximum color saturation 100%*/

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800


extern ULONG    ColorTab[];       /* SEE dlg_val.c                    */
static GRADIENT gradient;
static WINDOWINFO *pWI;
/* --- lHSBtoRGB --------------------------------------	[ Private ] ---	*/
/*									*/
/*     This function is	used to	determine the RGB value	for a given	*/
/*     hue and saturation.  The	function is based upon the algorithm	*/
/*     presented in the	Foly and van Dam book "Computer Graphics:       */
/*     Principles and Practice" Second Addition, page 593 Figure        */
/*     13.34.  The routine has been adapted to ignore the brightness	*/
/*     since it	will be	constant within	this implementation.		*/
/*     The Hue value corresponds to the	angle and the Saturation	*/
/*     corresponds to the radius of the	circle.	 The Hue value is	*/
/*     from 0 to 360ø and the Saturation value is from 0 to 100.  The	*/
/*     RGB values are defined by the system to be from 0 to 255.	*/
/*									*/
/*     Upon Entry:							*/
/*									*/
/*     LONG lHue;	 = Hue Value					*/
/*     LONG lSaturation; = Saturation Value				*/
/*									*/
/*     Upon Exit:							*/
/*									*/
/*     lHSBtoRGB = Resultant RGB Colour					*/
/*									*/
/* --------------------------------------------------------------------	*/

static LONG lHSBtoRGB(LONG lHue, LONG lSaturation)

{
RGB2   rgb2;			   /* RGB Colour Holder			*/
PLONG  plClr = (PLONG)&rgb2;	   /* Long Pointer to RGB Colour Holder	*/
		
		       /* Initialize the options component of the RGB	*/
		       /* holder since it is a reserved	value		*/
rgb2.fcOptions = 0;
		       /* Check	to see if the saturation level is 0	*/
		       /* in which case	the colour should be white	*/
if ( lSaturation == 0 )
   {
   rgb2.bRed   =
   rgb2.bBlue  =
   rgb2.bGreen = 255;
   }
else
   {
		       /* Check	to see if the hue is at	its maximum	*/
		       /* value	in which case the hue should revert to	*/
		       /* its lower limit				*/
   if (	lHue ==	360L )
       lHue = 0L;
		       /* Break	the hue	into 6 wedges which corresponds	*/
		       /* to red, yellow, green, cyan, blue and	magenta	*/
   switch ( lHue / 60L )
       {
		       /* Red wedge					*/
       case 0 :
	   rgb2.bRed   = (BYTE)255;
	   rgb2.bGreen = (BYTE)((((100 - ((lSaturation * (100 -	((lHue % 60L) *	100L) /	60)) / 100L))) * 255L) / 100L);
	   rgb2.bBlue  = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   break;
		       /* Yellow wedge					*/
       case 1 :
	   rgb2.bRed   = (BYTE)((((100 - ((lSaturation * ((lHue	% 60L) * 100L) / 60) / 100L))) * 255L) / 100L);
	   rgb2.bGreen = (BYTE)255;
	   rgb2.bBlue  = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   break;
		       /* Green	wedge					*/
       case 2 :
	   rgb2.bRed   = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   rgb2.bGreen = (BYTE)255;
	   rgb2.bBlue  = (BYTE)((((100 - ((lSaturation * (100 -	((lHue % 60L) *	100L) /	60)) / 100L))) * 255L) / 100L);
	   break;
		       /* Cyan wedge					*/
       case 3 :
	   rgb2.bRed   = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   rgb2.bGreen = (BYTE)((((100 - ((lSaturation * ((lHue	% 60L) * 100L) / 60) / 100L))) * 255L) / 100L);
	   rgb2.bBlue  = (BYTE)255;
	   break;
		       /* Blue wedge					*/
       case 4 :
	   rgb2.bRed   = (BYTE)((((100 - ((lSaturation * (100 -	((lHue % 60L) *	100L) /	60)) / 100L))) * 255L) / 100L);
	   rgb2.bGreen = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   rgb2.bBlue  = (BYTE)255;
	   break;
		       /* Magenta wedge					*/
       case 5 :
	   rgb2.bRed   = (BYTE)255;
	   rgb2.bGreen = (BYTE)(((100 -	lSaturation) * 255L) / 100L);
	   rgb2.bBlue  = (BYTE)((((100 - ((lSaturation * ((lHue	% 60L) * 100L) / 60) / 100L))) * 255L) / 100L);
	   break;
       }
   }
return(*plClr);
}
/*------------------------------------------------------------------------*/
/*  Name : InitializeSlider                                               */
/*                                                                        */
/*  Description :  Set the Sliders Tick size and Scale Text.              */
/*                                                                        */
/*  Concepts : Called each time a demo Slider controls ar initialized     */
/*             Ses the he initail value of the sliders output display     */
/*             to 0. A for 0 to 10 for loop sets the ruler text and       */
/*             tick size for both the Horizontal and Vertical Sliders     */
/*             via SLM_SETSCALETEXT and SLM_SETTICKSIZE message.          */
/*                                                                        */
/*  Parameters : hwnd - Handle of the Slider dialog.                      */
/*                                                                        */
/*  Returns: TRUE if Sliders are initialized successfully, FALSE otherwise*/
/*------------------------------------------------------------------------*/
BOOL InitializeSlider(HWND hwnd)
{
   USHORT usIndex;
   USHORT usTickNum = 0;
   CHAR   acBuffer[4];
   PSZ    cData;

   cData = (PSZ)ltoa(0,acBuffer,10);

   WinSetDlgItemText(hwnd,ID_GRADSTART, cData);
   WinSetDlgItemText(hwnd,ID_GRADSWEEP, cData);
   /*
   ** First place 37 ticks.
   */
   for (usTickNum = 0;usTickNum <= 36; usTickNum++)
   {

      if ( !WinSendDlgItemMsg(hwnd, ID_GRADSTART, SLM_SETTICKSIZE,
               MPFROM2SHORT(usTickNum, 10), NULL))
        return FALSE;

      if ( !WinSendDlgItemMsg(hwnd, ID_GRADSWEEP, SLM_SETTICKSIZE,
               MPFROM2SHORT(usTickNum, 10), NULL))
        return FALSE;
   }

   usTickNum=0;

   for (usIndex = 0;usIndex <= 360;  usIndex += 10, usTickNum++)
   {
      ltoa(usIndex,acBuffer,10);

      if (!(usIndex % 20))
      {
      if ( !WinSendDlgItemMsg(hwnd, ID_GRADSTART, SLM_SETSCALETEXT,
               MPFROMSHORT(usTickNum), MPFROMP(acBuffer)))
        return FALSE;

      if ( !WinSendDlgItemMsg(hwnd,ID_GRADSWEEP, SLM_SETSCALETEXT,
               MPFROMSHORT(usTickNum), MPFROMP(acBuffer)))
        return FALSE;
     }
   }

   WinSendDlgItemMsg(hwnd,ID_GRADSWEEP, SLM_SETSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                     MPFROMLONG((gradient.ulSweep/10)));

   WinSendDlgItemMsg(hwnd,ID_GRADSTART, SLM_SETSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                     MPFROMLONG((gradient.ulStart/10)));

   return TRUE;
}                                       /* End of InitializeSlider      */

/*------------------------Gradient window procedure------------------------*/

MRESULT EXPENTRY GradientWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
   RECTL rcl;
   HPS   hps;
   static SWP swp;		   /* Screen Window Position Holder	*/

   switch(msg)
   {
      case WM_CREATE:
        WinQueryWindowPos(hwnd, (PSWP)&swp);
	return 0;
      case WM_PAINT:
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiErase(hps);
         GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         WinQueryWindowRect(hwnd,&rcl);
         GradientFill((WINDOWINFO *)0,hps,&rcl,&gradient);
         WinEndPaint(hps);
         return 0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------Gradient dialog procedure------------------------*/
MRESULT EXPENTRY GradientDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
   static HwndGrad;
   static GRADIENT *pgr;  
   SWP swp;		   /* Screen Window Position Holder   */
   CHAR  szBuffer[10];
   ULONG ulStepSize;      /* Used for setting the labels.     */
   ULONG ulValue;
   ULONG ulStorage[2];    /* To get the vals out of the spins */
   PVOID pStorage;        /* idem spinbutton.                 */

   switch (msg)
   {
      case WM_INITDLG:
		       /* Centre dialog	on the screen			*/

         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

        pgr = (GRADIENT *)mp2;

        gradient.ulSweep = pgr->ulSweep;            /* sweeping angle    */
        gradient.ulSaturation = pgr->ulSaturation;  /* Color saturation. */
        gradient.ulStart = pgr->ulStart;            /* Start angle       */


        InitializeSlider(hwnd);

        HwndGrad = WinWindowFromID(hwnd, ID_GRADIENTWND);

        /* setup the spin button */

        WinSendDlgItemMsg( hwnd, ID_GRDSAT, SPBM_SETLIMITS,
                           MPFROMLONG(MAXSATURATION), MPFROMLONG(MINSATURATION));

        WinSendDlgItemMsg( hwnd, ID_GRDSAT, SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)MAXSATURATION), NULL);

        /*
        ** If there is no direction set yet than set the direction
        ** bottom->top. We make ulDirection the same as the radio
        ** button id.
        */
        if (!pgr->ulDirection)
           pgr->ulDirection = ID_GRADBOTTOM;

        gradient.ulDirection  = pgr->ulDirection;

        /*
        ** Set the radiobutton for the gradient direction.
        */
        WinSendDlgItemMsg(hwnd, pgr->ulDirection,BM_SETCHECK,
                          (MPARAM)1,(MPARAM)0);


        WinSetDlgItemText (hwnd,ID_GRADRANGE,
                           itoa(gradient.ulSweep, (PSZ)szBuffer, 10));

        /*
        ** Set the labels at the rightside of the window according
        ** to the startAngle and the SweepAngle.
        */
        WinSetDlgItemText (hwnd,ID_TXT0,(PSZ)itoa(gradient.ulStart, szBuffer, 10));
        ulStepSize = (gradient.ulSweep / 4);
        ulValue = ulStepSize + gradient.ulStart;
        WinSetDlgItemText (hwnd,ID_TXT90,(PSZ)itoa(ulValue, szBuffer, 10));
        ulValue += ulStepSize;
        WinSetDlgItemText (hwnd,ID_TXT180,(PSZ)itoa(ulValue, szBuffer, 10));
        ulValue += ulStepSize;
        WinSetDlgItemText (hwnd,ID_TXT270,(PSZ)itoa(ulValue, szBuffer, 10));
        ulValue += ulStepSize;
        WinSetDlgItemText (hwnd,ID_TXT360,(PSZ)itoa(ulValue, szBuffer, 10));
        WinInvalidateRect (HwndGrad,NULL, FALSE);
        return 0;

     case WM_CONTROL:
        /*
        ** Process the WM_CONTROL messages for the radiobuttons
        ** which define the startpoint of the gradientfill.
        */
        switch (LOUSHORT(mp1))
        {
           case ID_GRADLEFT:
              gradient.ulDirection = ID_GRADLEFT;
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              return 0;
           case ID_GRADRIGHT:
              gradient.ulDirection = ID_GRADRIGHT;
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              return 0;
           case ID_GRADTOP:
              gradient.ulDirection = ID_GRADTOP;
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              return 0;
           case ID_GRADBOTTOM:
              gradient.ulDirection = ID_GRADBOTTOM;
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              return 0;
           case ID_GRADCENTER:
              gradient.ulDirection = ID_GRADCENTER;
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              return 0;

        }
        /*
         * Process the WM_CONTROL messages for the slider and valueset
         * set controls.
         */
        switch(SHORT2FROMMP(mp1))
        {
           case SLN_CHANGE:
              if (SHORT1FROMMP(mp1) == ID_GRADSTART)
              {
                 gradient.ulStart = (ULONG) WinSendDlgItemMsg(hwnd, ID_GRADSTART,
                     SLM_QUERYSLIDERINFO,
                     MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                     NULL);
                 gradient.ulStart *= 10;
              }
              else
              {
                 gradient.ulSweep = (ULONG) WinSendDlgItemMsg(hwnd, ID_GRADSWEEP,
                    SLM_QUERYSLIDERINFO,
                    MPFROM2SHORT(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
                    NULL);
                 gradient.ulSweep *= 10;
                 WinSetDlgItemText (hwnd,ID_GRADRANGE,
                                    itoa(gradient.ulSweep,(PSZ)szBuffer, 10));

              }
              /*
              ** Set the labels at the rightside of the window according
              ** to the startAngle and the SweepAngle.
              */
              WinSetDlgItemText (hwnd,ID_TXT0,(PSZ)itoa(gradient.ulStart, szBuffer, 10));
              ulStepSize = (gradient.ulSweep / 4);
              ulValue = ulStepSize + gradient.ulStart;
              WinSetDlgItemText (hwnd,ID_TXT90,(PSZ)itoa(ulValue, szBuffer, 10));
              ulValue += ulStepSize;
              WinSetDlgItemText (hwnd,ID_TXT180,(PSZ)itoa(ulValue, szBuffer, 10));
              ulValue += ulStepSize;
              WinSetDlgItemText (hwnd,ID_TXT270,(PSZ)itoa(ulValue, szBuffer, 10));
              ulValue += ulStepSize;
              WinSetDlgItemText (hwnd,ID_TXT360,(PSZ)itoa(ulValue, szBuffer, 10));
              WinInvalidateRect (HwndGrad,NULL, FALSE);
              break;

        case SPBN_CHANGE:
           if (SHORT1FROMMP(mp1) == ID_GRDSAT) /* Is saturation changed? */
           {
              /*-- get value  out of the spin --*/

              pStorage = (PVOID)ulStorage;

              WinSendDlgItemMsg(hwnd,
                                ID_GRDSAT,
                                SPBM_QUERYVALUE,
                                (MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= 10 && ulStorage[0] <= 100 )
                 gradient.ulSaturation = (USHORT)ulStorage[0];
              WinInvalidateRect (HwndGrad,NULL, FALSE);
           }
           break;
        }
        return 0;

     case WM_COMMAND:
        switch(LOUSHORT(mp1))
	{
           case DID_OK:
              pgr->ulStart        = gradient.ulStart;
              pgr->ulSweep        = gradient.ulSweep;
              pgr->ulSaturation   = gradient.ulSaturation;
              pgr->ulDirection    = gradient.ulDirection;
              WinDismissDlg(hwnd,DID_OK);
              return 0;
           case DID_CANCEL:
              WinDismissDlg(hwnd,DID_CANCEL);
              return 0;
           case DID_HELP:
              ShowDlgHelp(hwnd);
              return 0;
        }
        return 0;

   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  Name: GradientFill.                                                   */
/*                                                                        */
/*  Description : Fills the given rectangle with the defined gradient.    */
/*                                                                        */
/*                Algoritme example:                                      */
/*                Room in the rectanle is 100 lines while the range       */
/*                or sweepangle is 360 or 360 diff colors.                */
/*                The biggest value which lies under 100 and gives an     */
/*                integer value on division is 90. 360/90=4.              */
/*                So we take one color from every 4 degrees shift.        */
/*                But we miss 10 lines still.                             */
/*                                                                        */
/*  Parameters : HPS hps       presentation space.                        */
/*               RECTL *prcl   pointer to rectangle structure which       */
/*                             defines the area to fill.                  */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
void GradientFill(WINDOWINFO *pwi, HPS hps, RECTL *rcl, GRADIENT *pGrad)
{
   ULONG ulLines;       /* Number of lines too draw       */
   ULONG y,i;
   LONG  lColor;
   POINTL ptl,ptlCenter;
   ULONG  ulSwp;        /* SweepAnge (range) in degrees   */
   ULONG  ulLinesize;   /* How thick should the line be to fill the rect?*/
   ARCPARAMS  arcParms; /* used if the filling starts at the center      */
   ULONG  Multiplier = 0x0000ff00L;
   ULONG  StartAngle = 0;
   ULONG  SweepAngle = MAKEFIXED(360,0);


   if (!pGrad->ulSweep)
      return;
   /*
   ** Is the gradient fill suppressed for window drawing?
   */
   if (pwi && pwi->bSuppress)
      return;

   switch (pGrad->ulDirection)
   {
      default:         /* ID_GRADBOTTOM */
         ulLines = (rcl->yTop - rcl->yBottom);
         /*
         ** Minimum linesize is one pixel...
         */
         ulLinesize = (ulLines / pGrad->ulSweep) +1;
         /*
         ** Here we start looping through the define range...
         */
         ptl.y = rcl->yBottom;
         y= pGrad->ulStart; /* starting point in degrees */
         for (ulSwp = 0; ulSwp < pGrad->ulSweep; y++,ulSwp++)
         {
            lColor = lHSBtoRGB(y % 360,pGrad->ulSaturation);
            ptl.x = rcl->xLeft;
            GpiMove(hps, &ptl);
            GpiSetColor(hps,lColor);
            ptl.x = rcl->xRight;
            ptl.y += ulLinesize;
            GpiBox(hps,DRO_FILL,&ptl,0,0);
         }
         break;
      case ID_GRADTOP:
         ulLines = (rcl->yTop - rcl->yBottom);
         /*
         ** Minimum linesize is one pixel...
         */
         ulLinesize = (ulLines / pGrad->ulSweep) +1;
         /*
         ** Here we start looping through the define range...
         */
         ptl.y = rcl->yTop;
         y= pGrad->ulStart; /* starting point in degrees */
         for (ulSwp = 0; ulSwp < pGrad->ulSweep; y++,ulSwp++)
         {
            lColor = lHSBtoRGB(y % 360,pGrad->ulSaturation);
            ptl.x = rcl->xLeft;
            GpiMove(hps, &ptl);
            GpiSetColor(hps,lColor);
            ptl.x = rcl->xRight;
            ptl.y -= ulLinesize;
            GpiBox(hps,DRO_FILL,&ptl,0,0);
         }
         break;
      case ID_GRADLEFT:
         ulLines = (rcl->xRight - rcl->xLeft);
         /*
         ** Minimum linesize is one pixel...
         */
         ulLinesize = (ulLines / pGrad->ulSweep) +1;
         /*
         ** Here we start looping through the define range...
         */
         ptl.x = rcl->xLeft;
         y= pGrad->ulStart; /* starting point in degrees */
         for (ulSwp = 0; ulSwp < pGrad->ulSweep; y++,ulSwp++)
         {
            lColor = lHSBtoRGB(y % 360,pGrad->ulSaturation);
            ptl.y = rcl->yBottom;
            GpiMove(hps, &ptl);
            GpiSetColor(hps,lColor);
            ptl.y = rcl->yTop;
            ptl.x += ulLinesize;
            GpiBox(hps,DRO_FILL,&ptl,0,0);
         }
         break;
      case ID_GRADRIGHT:
         ulLines = (rcl->xRight - rcl->xLeft);
         /*
         ** Minimum linesize is one pixel...
         */
         ulLinesize = (ulLines / pGrad->ulSweep) +1;
         /*
         ** Here we start looping through the define range...
         */
         ptl.x = rcl->xRight;
         y= pGrad->ulStart; /* starting point in degrees */
         for (ulSwp = 0; ulSwp < pGrad->ulSweep; y++,ulSwp++)
         {
            lColor = lHSBtoRGB(y % 360,pGrad->ulSaturation);
            ptl.y = rcl->yBottom;
            GpiMove(hps, &ptl);
            GpiSetColor(hps,lColor);
            ptl.y = rcl->yTop;
            ptl.x -= ulLinesize;
            GpiBox(hps,DRO_FILL,&ptl,0,0);
         }
         break;
       case ID_GRADCENTER:
         ulLines = (rcl->yTop - rcl->yBottom);
         /*
         ** Minimum linesize is one pixel...
         */
         ulLinesize = (ulLines / pGrad->ulSweep);
         if (!ulLinesize) ulLinesize++;
         /*
         ** Circular filling so get center..
         */
         ptlCenter.x = rcl->xLeft   + (rcl->xRight - rcl->xLeft)/2;
         ptlCenter.y = rcl->yBottom + (rcl->yTop - rcl->yBottom)/2;
         ptl.x = ptlCenter.x;
         ptl.y = ptlCenter.y;
         arcParms.lR = 0L;
         arcParms.lS = 0L;

         arcParms.lP = ulLinesize;
         arcParms.lQ = ulLinesize;

         GpiSetCurrentPosition(hps,&ptlCenter);
         y= pGrad->ulStart; /* starting point in degrees */
         for (ulSwp = 0; ulSwp < pGrad->ulSweep; ulSwp++, y++)
         {
            lColor = lHSBtoRGB(y % 360,pGrad->ulSaturation);
            GpiSetColor(hps,lColor);
            for (i = 0; i < ulLinesize; i++)
            {
               GpiSetArcParams(hps,&arcParms);
               GpiSetCurrentPosition(hps,&ptlCenter);
               GpiSetLineType(hps,LINETYPE_INVISIBLE);
               GpiPartialArc(hps,
                             &ptl,
                             Multiplier,
                             StartAngle,
                             SweepAngle);
               GpiSetLineType(hps,LINETYPE_SOLID);
               GpiSetArcParams(hps,&arcParms);
               GpiPartialArc(hps,
                             &ptl,
                             Multiplier,
                             StartAngle,
                             SweepAngle);
               arcParms.lP++;
               arcParms.lQ++;
            }
       }
       break;
   }
}
/*------------------------COLOR MENU ITEM ROUTINES---------------------------*/
MRESULT ColorPopupMenu(HWND hwnd,HWND hMenu, POINTL ptl)
{
   WinPopupMenu ( hwnd,
                  hwnd, 
                  hMenu,
                  ptl.x,
                  ptl.y,
                  0,
                  PU_NONE |
                  PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD |
                  PU_HCONSTRAIN | PU_VCONSTRAIN);

    return (MRESULT)0;
}
/*------------------------SHADING ROUTINES-----------------------------------*/
MRESULT EXPENTRY ShadingDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 )
{
   SWP swp;
   static HWND hButton;
   static HWND hwndClrMenu;
   static SHADE  shd, *ps;
   static POINTL ptlBtn;    /* Remember button pos for popup */
   static USHORT usLastButton;
   ULONG  ulColor;
   static BOOL bInit;

   switch(msg)
   {
     case WM_INITDLG:
         bInit = TRUE;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         hButton = WinWindowFromID(hwnd,ID_BTNSHADECOLOR);
         /*
         ** Get button position to center colored menu.
         */
         WinQueryWindowPos(hButton,(PSWP)&swp);
         ptlBtn.x = swp.x;
         ptlBtn.y = swp.y + swp.cy;

         ps = (SHADE *)mp2; /* Get our info.... */
         shd.lShadeType  = ps->lShadeType;   /* NONE,TOPLEFT,BOTTOMLEFT,BOTTOMRIGHT,TOPRIGHT */
         shd.lShadeColor = ps->lShadeColor;  /* Shading color.                               */
         shd.lUnits      = ps->lUnits;	     /* Number of Units used for shading             */
         WinSetPresParam(hButton,PP_BACKGROUNDCOLOR,sizeof(RGB),
                         (void *)&shd.lShadeColor);

         hwndClrMenu = WinLoadMenu(hwnd,0,IDM_FOUNTAINCOLOR);

         /*
         ** Setup the radio buttons
         */
         switch(shd.lUnits)
         {
            case SHADOWMIN:
               WinSendDlgItemMsg(hwnd,ID_RADSHADEMIN,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               break;
            case SHADOWMAX:
               WinSendDlgItemMsg(hwnd,ID_RADSHADELARGE,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               break;
            default:
               WinSendDlgItemMsg(hwnd,ID_RADSHADEMED,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
               break;
         }
         bInit = FALSE;
         break;

      /*-------------------------------POPUPMENU DRAWING ------------------*/
      case WM_MEASUREITEM:
         return measureColorItem(mp2);
      case WM_DRAWITEM:
         return drawColorItem(mp2);

       case WM_CONTROL:
          switch (LOUSHORT(mp1))
          {
             case ID_RADSHADEMIN:
                shd.lUnits = SHADOWMIN;
                break;
             case ID_RADSHADEMED:
                shd.lUnits = SHADOWMED;
                break;
             case ID_RADSHADELARGE:
                shd.lUnits = SHADOWMAX;
                break;
          }
          return (MRESULT)0;

     case WM_COMMAND:
        switch(LOUSHORT(mp1))
	{
	   case ID_BTNSHADECOLOR:
              usLastButton = LOUSHORT(mp1);
              ColorPopupMenu(hwnd,hwndClrMenu,ptlBtn);
              return (MRESULT)0;
           case DID_OK:
              ps->lShadeType  = shd.lShadeType;   /* NONE,TOPLEFT,BOTTOMLEFT,BOTTOMRIGHT,TOPRIGHT */
              ps->lShadeColor = shd.lShadeColor;  /* Shading color.                               */
              ps->lUnits      = shd.lUnits;       /* Number of Units used for shading             */
              WinDismissDlg(hwnd,DID_OK);
              return 0;
           case DID_CANCEL:
              WinDismissDlg(hwnd,DID_CANCEL);
              return 0;
        }

        if ( LOUSHORT(mp1) >= IDM_CLR1 && LOUSHORT(mp1) <= IDM_CLR40)
        {
            ulColor = ColorTab[ LOUSHORT(mp1) - IDM_CLR1 ];
            if (usLastButton == ID_BTNSHADECOLOR)
            {
               WinSetPresParam(hButton,PP_BACKGROUNDCOLOR,sizeof(RGB),
                               (void *)&ulColor);
               shd.lShadeColor = ulColor;
            }
         }
         return (MRESULT)0;

     case WM_DESTROY:
        WinDestroyWindow(hwndClrMenu);
        break;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}

void setupShading( WINDOWINFO *pwi)
{
   pWI = pwi;

   WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)ShadingDlgProc,
             (HMODULE)0,DLG_SHADING,(void *)&pwi->Shade);
}
/*---------------------------------------------------------------------------*/
/* setShadingOffset.                                                         */
/*                                                                           */
/* Description  : Calculates the shading offset which depends on the current */
/*                zooming factor. So thats why it is that complex....        */
/*---------------------------------------------------------------------------*/
void setShadingOffset( WINDOWINFO *pwi,ULONG ulShadeType, ULONG ulUnits, POINTL *ptl, ULONG ulPoints)
{
   float f1,f2,fdx,fdy;
   ULONG i;
   long  xOff,yOff;

   if (!pwi->usWidth || !pwi->usHeight)
      return;

   f1 = (float)ulUnits;
   f2 = (float)pwi->usWidth;
   fdx= (float)(f1/f2);
   f2 = (float)pwi->usHeight;
   fdy= (float)(f1/f2);

   xOff = (LONG)(fdx * pwi->usFormWidth);
   yOff = (LONG)(fdy * pwi->usFormHeight);

   if (pwi->ulUnits == PU_PELS)
   {
      xOff = (xOff * pwi->xPixels)/10000;
      yOff = (yOff * pwi->xPixels)/10000;
   }

   for (i = 0; i < ulPoints; i++)
   {
      switch (ulShadeType)
      {
         case SHADE_NONE:
            return;
         case SHADE_LEFTTOP:
            ptl->x -= xOff;
            ptl->y += yOff;
            break;
         case SHADE_LEFTBOTTOM:
            ptl->x -= xOff;
            ptl->y -= yOff;
            break;
         case SHADE_RIGHTTOP:
            ptl->x += xOff;
            ptl->y += yOff;
            break;
         case SHADE_RIGHTBOTTOM:
            ptl->x += xOff;
            ptl->y -= yOff;
            break;
      }
      ptl++;
   }
}