/*------------------------------------------------------------------------*/
/* Module : dlg_val.c                                                     */
/*                                                                        */
/* Description : in this module are the valueset created.                 */
/*               ColorValueset & Bitmap valueset.                         */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_sqr.h"
#include "dlg_txt.h"
#include "dlg_clr.h"
#define IDR_VSPALETTE 400

extern BOOL bOutLineColor; /* declared in main module.... */

HWND CreateColorValueSet(HWND hwnd);

       ULONG    ColorTab[] ={ 0X00000000, //black
                              0X00FFFFFF, //white
                              0X00808080, //Gray
                              0X00A0A0A0, //pale gray

                              0x00FFFF00, //yellow
                              0x00DCDC00, 
                              0x00C8C800, 
                              0X00B4B400,
                              0X00969600,

                              0X0000FF00, // GREEN
                              0X0000C800,
                              0X00009600,
                              0X00008000,
                              0X00B4FFB4,
                              0X0091C891,
                              0X00649164,


                              0x0000C4FF, //BLUE
                              0X000080FF,
                              0X000000FF, 
                              0X000000C8, 
                              0X00000080,
                              0X00B4B4FF,
                              0X00646491,
                              0x0000FFFF,
                              0X00008080,

                              0X00FF00FF,  //PURPLE
                              0X00C800C8,
                              0X00AA00AA,
                              0X00960096,

                              0X00FF0000, //RED
                              0X00E00000,
                              0X00B00000,
                              0X00900000,
                              0X00800000 };




static HWND hwndclr;

MRESULT EXPENTRY FrameWndProc(HWND,USHORT,MPARAM,MPARAM);

PFNWP fnframeWndProc;

HWND CreateColorValueSet(HWND hwnd)
{
   VSCDATA  vscdata;
   ULONG    vsStyle;
   USHORT   idxRow;
   USHORT   idxCol;
   RECTL    rcl;

   vscdata.cbSize = sizeof(VSCDATA);
   vscdata.usRowCount    = 1;
   vscdata.usColumnCount = 32;

   vsStyle =  VS_RGB;

   /*
    * Create a value set control for the color palette selections.
    */

   hwndclr   = WinCreateWindow (hwnd,
                                WC_VALUESET,
                                (PSZ)NULL,
                                vsStyle,
                                0,
                                0,
                                100,
                                50,
                                hwnd,
                                HWND_BOTTOM, 
                                IDR_VSPALETTE,
                                &vscdata,
                                (PVOID)NULL);


   fnframeWndProc = WinSubclassWindow(hwndclr,(PFNWP)FrameWndProc);
   /*
    * Place the color indices into the cells.
    */
   for (idxCol=1; idxCol<=vscdata.usColumnCount; idxCol++)
      for (idxRow=1; idxRow<=vscdata.usRowCount; idxRow++)
         WinSendMsg(hwndclr,
                    VM_SETITEM,
                    MPFROM2SHORT(idxRow,idxCol),
                    MPFROMLONG(ColorTab[ (8 * (idxRow-1))+(idxCol-1) ]));

   return (HWND)hwndclr;
}               
/*------------------------------------------------------------------------*/
/*  FrameWndProc.                                                         */
/*                                                                        */
/*  Description : Frame window procedure of the subclasses color valueset */
/*                which lives at the bottom of the application.           */
/*                Handles only the WM_BUTTON1DBLCLK for bringing up the   */
/*                color mix dialog with three scrollbars                  */
/*                for remixing the RGB color.                             */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY FrameWndProc(HWND hwnd,USHORT msg,MPARAM mp1,MPARAM mp2)
{
   switch(msg)
   {
      case WM_BUTTON1DBLCLK:
         WinLoadDlg(HWND_DESKTOP,hwndClient,(PFNWP)TrueClrDlgProc,
		    0L,700,NULL);
         return 0;

      case WM_BUTTON2UP:
         bOutLineColor=TRUE;
         WinPostMsg(hwnd,WM_BUTTON1UP,mp1,MPFROMLONG(1L));
         break;

      case WM_BUTTON2DOWN:
         WinPostMsg(hwnd,WM_BUTTON1DOWN,mp1,mp2);
         break;

      case WM_BUTTON1UP:
         if (!mp2)
         {
            /*
            ** Not posted by me so....
            */
            bOutLineColor=FALSE;
         }
         break;

   }
return fnframeWndProc(hwnd,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
/* Name : SetClrValueSetItem.                                             */
/*                                                                        */
/* Description : This function sets the color of the selected item in the */
/*               colorvalueset after it has been changed via the RGB      */
/*               window. Called from DrwSetWinValue().                    */
/*                                                                        */
/* Parameters : ULONG ulItem = the item which must be updated.            */
/*              ULONG ulColor  The color which is set into the valueset   */
/*                                                                        */
/* Precondition : HwndClr which is the window handel of the valueset must */
/*                be set.                                                 */
/* Returns : NONE.                                                        */
/*------------------------------------------------------------------------*/
void SetClrValueSetItem(ULONG ulItem, ULONG ulColor)
{
   WinSendMsg(hwndclr,
              VM_SETITEM,
              MPFROMLONG(ulItem),
              MPFROMLONG(ulColor));
}
