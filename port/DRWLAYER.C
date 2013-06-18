/*------------------------------------------------------------------------*/
/*  Name: drwlayer.c                                                      */
/*                                                                        */
/*  Description : Contains the windowprocedure for the printpreview       */
/*                and the layerwindow.                                    */
/*                                                                        */
/*  Functions  :                                                          */
/*                                                                        */
/* setDraftPreview : Called from the print preview dialog (drwprn.c)      */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_btn.h"
#include "resource.h"
#include "dlg_hlp.h"
#include "drwutl.h"

static WINDOWINFO LayerInfo; 
static WINDOWINFO *pwMain;
static HWND       hwndLayer;
static USHORT     opmode;       /* PRINTPREVIEW OR LAYER */
static BOOL       bDialogActive;
/*------------------------------------------------------------------------*/
void LayerConnect(WINDOWINFO *pw, USHORT option)
{
   pwMain = pw;
   opmode = option;
}
/*-------------------------------------------------------------------------*/
void setDraftPreview( BOOL bDraft)
{
   LayerInfo.bDraftPrinting = bDraft;
}
/*----------------------------LAYER DIALOG PROCEDURE ---------------------*/
MRESULT EXPENTRY LayerDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SWP swp;		   /* Screen Window Position Holder	*/
   char  szBuffer[10] ;

   switch (msg)
   {
      case WM_INITDLG:
         bDialogActive = TRUE;
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);
        
         hwndLayer = WinWindowFromID(hwnd, 1201);
         LayerInfo.usdrawlayer  = pwMain->uslayer;
         LayerInfo.uslayer      = pwMain->uslayer;
         WinSetDlgItemText (hwnd,1205,itoa(LayerInfo.uslayer, szBuffer, 10));
         /*
         ** May we select all objects on all different layers?
         ** show it in with a check box.
         */
         if (!pwMain->bSelAll)
         {
            WinSendDlgItemMsg(hwnd,ID_LAYERCHK,
                              BM_SETCHECK,(MPARAM)1,(MPARAM)0);
         }
         else
         {
            WinSendDlgItemMsg(hwnd,ID_LAYERCHK,BM_SETCHECK,
                              (MPARAM)0,(MPARAM)0);
         }

         WinSendDlgItemMsg(hwnd,ID_LAYERACT,BM_SETCHECK,
                          (MPARAM)pwMain->bActiveLayer,(MPARAM)0);

         
	 return 0;

      case WM_CONTROL:
           switch(LOUSHORT(mp1))
           {
              case ID_LAYERCHK:
                 pwMain->bSelAll = !pwMain->bSelAll;
                 break;
              case ID_LAYERACT:
                 pwMain->bActiveLayer = !pwMain->bActiveLayer;
                 break;
           }
           return (MRESULT)0;

      case WM_COMMAND:
	   switch(LOUSHORT(mp1))
	   {
              case DID_OK:
                 bDialogActive = FALSE;
                 pwMain->uslayer = LayerInfo.uslayer;
                 GpiDestroyPS(LayerInfo.hps);
                 WinPostMsg(pwMain->hwndMain,
                            UM_LAYERHASCHANGED,
                            (MPARAM)0,
                            (MPARAM)0);
                 WinDismissDlg(hwnd,TRUE);
                 WinInvalidateRect(pwMain->hwndClient,(RECTL *)0,TRUE);
                 if (pwMain->bActiveLayer)   /* If this is true, lock other layers */
                    pwMain->bSelAll = FALSE;
                 return 0;
              case ID_LAYERPREV:               /*previous*/
                 if (LayerInfo.uslayer > 1)
                 {
                    LayerInfo.uslayer --;
                    LayerInfo.usdrawlayer--;
                    WinSetDlgItemText (hwnd,1205,itoa(LayerInfo.uslayer, szBuffer, 10));
                    WinInvalidateRect(hwndLayer,(RECTL *)0,TRUE);
                 }  
                 else
                    WinAlarm(HWND_DESKTOP, WA_WARNING);
                 return 0;
              case ID_LAYERNEXT:               /*Next */
                 if (LayerInfo.uslayer < 10)
                 {
                    LayerInfo.uslayer ++;
                    LayerInfo.usdrawlayer++;
                    WinSetDlgItemText (hwnd,1205,itoa(LayerInfo.uslayer, szBuffer, 10));
                    WinInvalidateRect(hwndLayer,(RECTL *)0,TRUE);
                 }  
                 else
                    WinAlarm(HWND_DESKTOP, WA_WARNING);
                 return 0;
              case DID_HELP:
                 ShowDlgHelp(hwnd);
                 return 0;
           }
           return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*-------------------------------------------------------------------------*/
MRESULT EXPENTRY LayerWndProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   RECTL  rcl;
   POINTL ptl;
   float  fTarget,fSource;
   float  fScale[2],fAspect;
   HPS    hps;
   SWP    swp;
   SIZEL  sizlMaxClient;   /* max client area size */

   switch(msg)
   {
      case WM_CREATE:
         sizlMaxClient.cx = 0;
         sizlMaxClient.cy = 0;
         LayerInfo.hdcClient = WinOpenWindowDC(hwnd);


         LayerInfo.hps = GpiCreatePS( pwMain->hab, 
                                      LayerInfo.hdcClient,
                                      &sizlMaxClient,
		  	              PU_LOMETRIC  |
                     		      GPIF_DEFAULT |
			              GPIT_NORMAL  |
                                      GPIA_ASSOC);
         LayerInfo.hpsMem = pwMain->hpsMem;
         LayerInfo.hpsScreen = LayerInfo.hps;
         LayerInfo.bPrinter  = FALSE;
         GpiCreateLogColorTable (LayerInfo.hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );

         GpiQueryModelTransformMatrix(LayerInfo.hps,9L,&LayerInfo.matOrg);

         WinQueryWindowPos(hwnd, (PSWP)&swp);

         LayerInfo.cyClient     = 0;
         LayerInfo.usdrawlayer  = pwMain->uslayer;
         LayerInfo.uslayer      = pwMain->uslayer;
         LayerInfo.lBackClr     = pwMain->lBackClr;
         LayerInfo.bDraftPrinting = FALSE;
         LayerInfo.ulUnits      = PU_LOMETRIC;
     
         ptl.x = swp.cx;
         ptl.y = swp.cy;
         GpiConvert(LayerInfo.hps,CVTC_DEVICE,CVTC_WORLD,1,&ptl);

         fTarget   = (float)ptl.x;
         fSource   = (float)pwMain->usFormWidth;
         fScale[0] = fTarget / fSource;

         fTarget   = (float)ptl.y;
         fSource   = (float)pwMain->usFormHeight;
         fScale[1] = fTarget / fSource;

         if ( fScale[0] < fScale[1])
            fAspect = fScale[0];
         else
            fAspect = fScale[1];

         LayerInfo.usFormWidth  = (LONG)(fAspect * pwMain->usFormWidth);
         LayerInfo.usFormHeight = (LONG)(fAspect * pwMain->usFormHeight);

         ptl.x =  LayerInfo.usFormWidth; 
         ptl.y =  LayerInfo.usFormHeight;

         GpiConvert(LayerInfo.hps,CVTC_WORLD,CVTC_DEVICE,1,&ptl);
         /*
         ** Resize the preview window.
         */
         WinSetWindowPos(hwnd,HWND_TOP,0,0,ptl.x,ptl.y,SWP_SIZE);

         LayerInfo.lcid         = 3L;

         LayerInfo.uYfactor = 0;
         LayerInfo.uXfactor = 0;
	 return 0;

      case WM_PAINT:
	 WinQueryWindowRect(hwnd,&rcl);
         hps = WinBeginPaint (hwnd,LayerInfo.hps,&rcl);
         WinFillRect(hps,&rcl,pwMain->lBackClr);

         if (opmode == IDM_PRINTPREVIEW)
         {
            /*
            ** Show for the print preview all layers!!
            */
            if (!LayerInfo.bDraftPrinting)
            {
               for ( LayerInfo.usdrawlayer = MINLAYER; LayerInfo.usdrawlayer <= MAXLAYER; LayerInfo.usdrawlayer++)
               {
                  ObjDrawSegment(LayerInfo.hps,&LayerInfo,(POBJECT)0,(RECTL *)0);
               }
            }
            else
            {
               for ( LayerInfo.usdrawlayer = MINLAYER; LayerInfo.usdrawlayer <= MAXLAYER; LayerInfo.usdrawlayer++)
               {
                  ObjMultiDrawOutline(&LayerInfo,(POBJECT)0,FALSE);
               }
            }
         }
         else
            ObjDrawSegment(LayerInfo.hps,&LayerInfo,(POBJECT)0,(RECTL *)0);
         WinEndPaint(hps);
         return 0;
      case WM_DESTROY:
         GpiDestroyPS(LayerInfo.hps);
         return (MRESULT)0;
   }
   return WinDefWindowProc (hwnd ,msg,mp1,mp2);
}
/*------------------------------------------------------------------------*/
MRESULT layerDetail( HWND hOwner, WINDOWINFO *pwi)
{
   if (!bDialogActive)
   {

      LayerConnect(pwi,LAYERBUTTON);
      WinLoadDlg(HWND_DESKTOP,hOwner,(PFNWP)LayerDlgProc,
                 (HMODULE)0,ID_LAYER,NULL);
   }
   else
      WinAlarm(HWND_DESKTOP, WA_WARNING);
   return (MRESULT)0;
}
