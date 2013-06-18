/*---------------------------------------------------------------------------*/
/*  Name: drwcbar.c                                                          */
/*                                                                           */
/*  Description : Implements the colorbar at the bottom of the main window.  */
/*                                                                           */
/*---------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdlib.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwcbar.h"

#define CLRSIZE     42 /* Number of entries in the colortable */
#define CBARHEIGHT  20
#define CBARXOFFSET 40 /* offset in the parent window. x-position */
#define CBARYOFFSET 20 /* offset in the patent window, y-position */
#define CLRBORDER    4 /* 3D-border with and outher border of bar */

#define SCRLBUTTONSIZE 20 /* Scroll button with. Heigh == cbarheight - CLRBORDER */

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#ifdef __IBMC__
#define ltoa _ltoa
#endif

static SHORT MaxRgb[3]  = { 255,         // R
                            255,         // G
                            255};        // B

static SHORT IncVals[3] = { 16,
                            16,
                            16};

static USHORT Colors[3]; /*The colors in the the window...*/

static HWND  hParent;

static long lCurColor;
static int  iCurIndex;

       ULONG    ColorTab[] ={ 0X00000000, //black
                              0X00FFFFFF, //white
                              0X00808080, //Gray
                              0X00A0A0A0, //pale gray

                              0x00FFFF8C, // 2 NEW YELLOW ELEMENTS
                              0X00FFFF7F,
                              0x00FFFF00, //yellow
                              0x00DCDC00, 
                              0x00C8C800, 
                              0X00B4B400,
                              0X00969600,

                              0X00ADFFB3, // 2 NEW GREEN.
                              0X007FFF7F,
                              0X0000FF00, // GREEN
                              0X0000C800,
                              0X00009600,
                              0X00008000,
                              0X00B4FFB4,
                              0X0091C891,
                              0X00649164,


                              0X00C8FFFF, // 2 NEW BLUE
                              0X008CFFFF,
                              0x0000FFFF,
                              0x0000C4FF, //BLUE
                              0X000080FF,
                              0X000000FF, 
                              0X000000C8, 
                              0X00000080,
//                              0X00B4B4FF,
//                              0X00646491,
//                              0X00008080,

                              0X00FFBEFF, //3 NEW PURPLE
                              0X00E0A1E0, 
                              0X00ff50ff,

                              0X00FF00FF,  //PURPLE
                              0X00C800C8,
                              0X00AA00AA,
                              0X00960096,

                              0X00FFB9B9, // 2 NEW RED
                              0X00FF7F7F,
                              0X00FF0000, //RED
                              0X00E00000,
                              0X00B00000,
                              0X00900000,
                              0X00800000 };

char szClass[]="cbar";
char szColorClass[]="RGBCLASS";

static RECTL rclColors[CLRSIZE+1];

MRESULT EXPENTRY cbarWndProc(HWND,ULONG,MPARAM,MPARAM);


/*------------------------RGB-Window procedure-----------------------------*/

MRESULT EXPENTRY RedGreenBlueWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   RECTL rcl;
   HPS   hps;
   static SWP  swp;

   switch(msg)
   {
      case WM_CREATE:
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         return (MRESULT)0;

      case WM_PAINT:
         WinQueryWindowRect(hwnd,&rcl);
         hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
         GpiCreateLogColorTable ( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
         WinFillRect (hps, &rcl, (ULONG) Colors[0] << 16 |
                                 (ULONG) Colors[1] <<  8 |
                                 (ULONG) Colors[2]);
         rcl.xLeft   = 0;
         rcl.yBottom = 0;
         rcl.xRight  = swp.cx;
         rcl.yTop    = swp.cy;
         WinDrawBorder(hps, &rcl,2L,2L,0L, 0L, DB_DEPRESSED);
         WinEndPaint(hps);
         return 0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/* TrueClrDlgProc.                                                        */
/*                                                                        */
/* Description : This dialog procedure shows three scroll bars, one for   */
/*               each prim color. RED,GREEN,BLUE. This is mainly used     */
/*               when the user double clicks on the main color value set  */
/*               at the bottom of the application.                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY TrueClrDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   SHORT id;
   CHAR  szBuffer[20];


   SWP   swp;               /* Screen Window Position Holder */
   static HWND  HwndTrueColor;

   switch (msg)
   {
      case WM_INITDLG:
         HwndTrueColor = WinWindowFromID(hwnd, 707);
         Colors[0] = (USHORT)((lCurColor & 0x00ff0000) >> 16);
         Colors[1] = (USHORT)((lCurColor & 0x0000ff00) >> 8);
         Colors[2] = (USHORT)(lCurColor & 0x000000ff);

         /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
                         ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
                         ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
                         0, 0, SWP_MOVE);

        for (id=0; id < 3; id++)
              WinSendDlgItemMsg(hwnd,701+id, SBM_SETSCROLLBAR,
                                MPFROM2SHORT (0, 0), MPFROM2SHORT (0, MaxRgb[id]));

        for (id=0; id < 4; id++)
           WinSendDlgItemMsg(hwnd,701+id, SBM_SETPOS,
                             MPFROM2SHORT (Colors[id], 0), NULL) ;

        WinSetDlgItemText (hwnd,704,itoa (Colors[0], szBuffer, 10));
        WinSetDlgItemText (hwnd,705,itoa (Colors[1], szBuffer, 10));
        WinSetDlgItemText (hwnd,706,itoa (Colors[2], szBuffer, 10));
        return 0;
      case WM_VSCROLL :
         id = LOUSHORT(mp1)-701;          // ID of scroll bar
         switch (HIUSHORT(mp2))
         {
            case SB_LINEDOWN :
               Colors[id] = min (MaxRgb[id], Colors[id] + 1) ;
               break ;
            case SB_LINEUP :
               Colors[id] = max (0, Colors[id] - 1) ;
               break ;
            case SB_PAGEDOWN :
               Colors[id] = min (MaxRgb[id], Colors[id] + IncVals[id]) ;
               break ;
            case SB_PAGEUP :
               Colors[id] = max (0, Colors[id] - IncVals[id]) ;
               break ;
            case SB_SLIDERTRACK :
               Colors[id] = LOUSHORT(mp2) ;
               break ;
            default :
               return 0 ;
         }
         WinSendDlgItemMsg(hwnd,id+701, SBM_SETPOS,MPFROM2SHORT (Colors[id], 0), NULL) ;
         WinSetDlgItemText (hwnd,id+704,itoa (Colors[id], szBuffer, 10));
         WinInvalidateRect(HwndTrueColor,(RECTL *)0,TRUE);
         return 0;
      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               lCurColor = (ULONG) Colors[0] << 16 |
                           (ULONG) Colors[1] <<  8 |
                           (ULONG) Colors[2];
               WinDismissDlg(hwnd,DID_OK);
               return 0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               return 0;
         }
         return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*---------------------------------------------------------------------------*/
/*  cbar::registerClass.                                                     */
/*                                                                           */
/*  Description : Registers the cbar window class.  [ static member ]        */
/*---------------------------------------------------------------------------*/
BOOL RegisterColorBar(HAB hab)
{
   BOOL bReg = FALSE;
   ULONG ulStyle = (CS_SIZEREDRAW | CS_SAVEBITS );

   bReg = WinRegisterClass(hab,(PSZ)szClass,
                           (PFNWP)cbarWndProc,
                           ulStyle,4L);

   bReg = WinRegisterClass(hab,(PSZ)szColorClass,
                           (PFNWP)RedGreenBlueWndProc,
                           ulStyle,4L);

   return bReg;
}


HWND cbar(HWND hWndParent)
{
   HWND hCbar = WinCreateWindow(hWndParent,szClass,NULL,
                           WS_VISIBLE,
                           0,0,
                           0,0,
                           hWndParent,
                           HWND_TOP,
                           0x1000,
                           (void *)NULL,
                           NULL);
   hParent = hWndParent;
   return hCbar;
}
/*------------------------------------------------------------------------*/
static  void paintBar(HPS hps, long cx, long cy)
{
   int i;

   POINTL ptl1,ptl2;
   RECTL rclFrame;
   LONG  lVertSideWidth;
   LONG  lHorizSideWidth;
   LONG  flCmd;
   LONG  lFrameColor;
   LONG  cxColorBox;
   LONG  cyColor,cxColor;
   LONG  cyFrame,cxFrame;

   cxColorBox = (cx / CLRSIZE);      /* size incl frame and space in between */
   cxFrame    = cxColorBox - CLRBORDER; /* 3d frame width                       */
   cxColor    = cxFrame    - CLRBORDER; /* Color itself in the frame.           */

   cyFrame    = cy     - CLRBORDER;     /* leave 2 pixels at bottom and top.    */
   cyColor    = cyFrame -CLRBORDER;     /* Color size minus 3d border.          */

   ptl1.x = CLRBORDER;
   ptl1.y = CLRBORDER;
   ptl2.x = CLRBORDER + cxColor;
   ptl2.y = CLRBORDER + cyColor;

   rclFrame.xLeft   = 2;
   rclFrame.yBottom = 2;
   rclFrame.xRight  = 2 + cxFrame;
   rclFrame.yTop    = 2 + cyFrame;

   lVertSideWidth  = 2;
   lHorizSideWidth = 2;
   flCmd           = DB_DEPRESSED;
   lFrameColor     = WinQuerySysColor(HWND_DESKTOP,SYSCLR_BUTTONLIGHT,0);
   GpiSetPattern(hps,PATSYM_SOLID);

   for (i=0; i <  CLRSIZE; i++)
   {
      GpiSetColor(hps,lFrameColor);
      WinDrawBorder(hps, &rclFrame,lVertSideWidth,lHorizSideWidth,
                    0L, 0L, flCmd);

      GpiSetColor(hps,ColorTab[i]);
      GpiMove(hps,&ptl1);
      GpiBox(hps,DRO_FILL,&ptl2,0,0);
      /*
      ** For later use we remember the rect of each color.
      */
      rclColors[i].xLeft    = ptl1.x;
      rclColors[i].yBottom  = ptl1.y;
      rclColors[i].xRight   = ptl2.x;
      rclColors[i].yTop     = ptl2.y;
      ptl1.x += cxColorBox;
      ptl2.x += cxColorBox;
      rclFrame.xLeft  += cxColorBox;
      rclFrame.xRight += cxColorBox;
    }
    return;
}
/*------------------------------------------------------------------------*/
int cBarGetColor(POINTL *pptl, ULONG *lColor)
{
   int i;

   for (i = 0; i < CLRSIZE; i++)
   {
      if (WinPtInRect((HAB)0,&rclColors[i],pptl))
      {
         *lColor = ColorTab[i];
         return i;
      }
   }
   return -1;
}
/*------------------------------------------------------------------------*/
MRESULT onDropColor(HWND hBar, MPARAM mp1)
{

   POINTL ptlPosn;
   ULONG  lColor;
   int    iIndex;
   int    fnd;                          /* presparam */
   char   buffer[100];

   WinQueryPointerPos(HWND_DESKTOP,&ptlPosn);
   WinMapWindowPoints(HWND_DESKTOP,hBar,&ptlPosn,1);

   iIndex = cBarGetColor(&ptlPosn,&lColor);

   if ( iIndex < 0)
      return (MRESULT)0;

   WinQueryPresParam(hBar, (int)mp1, 0, (PULONG)&fnd,
                           sizeof(buffer)-1, buffer, 0);

   ColorTab[iIndex] = *(ULONG*)buffer;
   WinInvalidateRect(hBar,(RECTL *)0,TRUE);
   return (MRESULT)0;
}
/*---------------------------------------------------------------------------*/
/*  cbarWndProc.                                                             */
/*                                                                           */
/*  Description : Window Procedure.                                          */
/*---------------------------------------------------------------------------*/
MRESULT EXPENTRY cbarWndProc(HWND hwnd,ULONG ulMsg,MPARAM mp1,MPARAM mp2)
{
   HPS hps;
   RECTL rcl;
   ULONG  lColor;
   ULONG  ulResult;
   int    i;
   static LONG cx,cy;

    switch (ulMsg)
    {
      case WM_PRESPARAMCHANGED:
         switch ((long)mp1)
         {
            case PP_FOREGROUNDCOLOR:
            case PP_BACKGROUNDCOLOR:
               return onDropColor(hwnd,mp1);
         }
         break;

       case WM_BUTTON1DOWN:
          {
             POINTL ptl;
             ptl.x =(LONG)(SHORT)SHORT1FROMMP(mp1);
             ptl.y =(LONG)(SHORT)SHORT2FROMMP(mp1);

             if ((i = cBarGetColor(&ptl,&lColor)) >= 0)
             {
                lCurColor = lColor;
                iCurIndex = i;
                WinPostMsg(hParent,UM_FORECOLORCHANGED,MPFROMLONG(lCurColor),
                           (MPARAM)0);
             }
          }
          return (MRESULT)0;
       case WM_BUTTON2DOWN:
          {
             POINTL ptl;
             ptl.x =(LONG)(SHORT)SHORT1FROMMP(mp1);
             ptl.y =(LONG)(SHORT)SHORT2FROMMP(mp1);

             if ((i = cBarGetColor(&ptl,&lColor)) >= 0)
             {
                lCurColor = lColor;
                iCurIndex = i;
                WinPostMsg(hParent,UM_BACKCOLORCHANGED,MPFROMLONG(lCurColor),
                           (MPARAM)0);
             }
          }
          return (MRESULT)0;

       case WM_BUTTON1DBLCLK:
          ulResult = WinDlgBox(HWND_DESKTOP,hParent,TrueClrDlgProc,
                               (HMODULE)0,700,(VOID *)0);
          if (ulResult == DID_OK)
          {
             ColorTab[iCurIndex] = lCurColor;
             WinInvalidateRect(hwnd,(RECTL *)0,TRUE);
          }
          return (MRESULT)0;

       case WM_SIZE:
          cx = (LONG)SHORT1FROMMP(mp2);
          cy = (LONG)SHORT2FROMMP(mp2);
          return (MRESULT)0;
       case WM_PAINT:
          WinQueryWindowRect(hwnd,&rcl);
          hps = WinBeginPaint (hwnd,(HPS)0,&rcl);
          WinFillRect(hps,&rcl,CLR_PALEGRAY);
          GpiCreateLogColorTable (hps, LCOL_RESET,LCOLF_RGB,0,0,NULL);
          paintBar(hps,cx,cy);
          WinEndPaint(hps);
          return (MRESULT)0;
   }
   return (MRESULT)WinDefWindowProc(hwnd,ulMsg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
MRESULT setCurBarColor(HWND hBar,ULONG ulNewColor)
{

   ColorTab[iCurIndex] = ulNewColor;
   WinInvalidateRect(hBar,(RECTL *)0,TRUE);
   return (MRESULT)0;
}
