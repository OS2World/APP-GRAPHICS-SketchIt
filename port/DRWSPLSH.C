/*------------------------------------------------------------------------*/
/*  Name: Splash.c                                                        */
/*                                                                        */
/*  Author : Jasper de Keijzer.                                           */
/*  date   : 22 Oct 1994.                                                 */
/*                                                                        */
/*  Description : Starts the drawing program with the Leonardo da Vinci   */
/*                image.                                                  */
/*                                                                        */
/*                                                                        */
/*-pr--bug---ddmmyy----who------summary-----------------------------------*/
/* 1   ----  221094    JdK      Initial version.                          */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdio.h>
#include "dlg_hlp.h"
#include "resource.h"

#define ID_SPLASH    1300
#define IDB_MONALISA 1340
#define IDB_LOGO     1341

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

MRESULT EXPENTRY ClientDlgProc (HWND, USHORT, MPARAM, MPARAM);
MRESULT EXPENTRY SplashWndProc (HWND, USHORT, MPARAM, MPARAM);

void Splash(HAB hab,HWND hwnd)
{
   static CHAR szClientClass[]    = "SPLASHCLASS";

   WinRegisterClass( hab,			// Another block handle
                    (PSZ)szClientClass,         // Name of class being registered
                    (PFNWP)SplashWndProc,       // Window procedure for class
                    CS_SIZEREDRAW,		// Class style
                    (BOOL)0);			// Extra bytes to reserve


   WinDlgBox(HWND_DESKTOP,hwnd,
	     (PFNWP)ClientDlgProc,
	     (HMODULE)0,
             ID_SPLASH,
             (PVOID)0);

   return;
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ClientDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   SWP   swp;
  
   switch(msg)
   {
      case WM_INITDLG:
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);
         return (MRESULT)0;

      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
            case DID_OK:
               WinDismissDlg( hwnd, DID_OK);
               break;
            case ID_SPLHLP: /* Tell them how to register this version */
               ShowDlgHelp(hwnd);
               break;
         }
         return (MRESULT)0;
   }
   return( WinDefDlgProc(hwnd, msg, mp1, mp2) );
}
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY SplashWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   static HBITMAP hbm;
   static SWP     swp;
   HPS    hps;
   RECTL  rcl;

   switch(msg)
   {
      case WM_CREATE:
         hps = WinGetPS (hwnd);
         hbm = GpiLoadBitmap(hps,
                             (HMODULE)0L,
                             IDB_LOGO,
                             (ULONG)0,
                             (ULONG)0);
         WinReleasePS (hps);

         WinQueryWindowPos(hwnd, (PSWP)&swp);
         return (MRESULT)0;

      case WM_PAINT:
         hps = WinBeginPaint (hwnd, 
                              NULLHANDLE,  /* Get a cached PS */
                              (RECTL *)0);
         GpiErase (hps);
         
         GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_RGB,
                                 0L, 0L, NULL) ;
         rcl.xLeft   = 0;
         rcl.yBottom = 0;
         rcl.xRight  = swp.cx;
         rcl.yTop    = swp.cy;

         if ( hbm )
            WinDrawBitmap(hps,hbm, NULL, (PPOINTL) &rcl,
                          CLR_NEUTRAL, CLR_BACKGROUND, DBM_STRETCH );

         WinDrawBorder(hps, &rcl,2L,2L,0L, 0L, DB_DEPRESSED);
         WinEndPaint(hps);
         return (MRESULT)0;


   }
   return( WinDefWindowProc(hwnd, msg, mp1, mp2) );
}
