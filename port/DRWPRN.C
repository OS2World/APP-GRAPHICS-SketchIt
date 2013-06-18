/*------------------------------------------------------------------------*/
/*  Name: drwprn.c.                                                       */
/*                                                                        */
/*  Description : Functions for handling printing.                        */
/*                of objects.                                             */
/*                                                                        */
/* Private functions:                                                     */
/* EnumQueues         : Enumerates the printer queues.                    */
/* QueryJobProperties : Queries the jobproperties of the selected printer.*/
/* PrnGetFormSize     : Get formsizes of the selected printer.            */
/* CreatePrnBmpHdcHps : Creates printer compatible memory PS for blitting */
/* DeletePS           : Deletes the printer ps's.                         */
/*                                                                        */
/* Public functions:                                                      */
/* QueryPrintDlgProc  : Shows the list of avialable printers.             */
/* PrintWrite         : Our working thread which finally does the printing*/
/* ClosePrinter       : Total cleanup.                                    */
/* PrintOpen          : Starts the printer stuff with a dialog etc.       */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_DEV
#define INCL_DOSMEMMGR
#define INCL_BASE
#define INCL_SPLERRORS       //defined if INCL_ERRORS defined
#define INCL_SPL             //file system emulation calls
#define INCL_SPLDOSPRINT     //DosPrint APIs
#define INCL_WINWINDOWMGR
#define INCL_GPI
#include <os2.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwprn.h"
#include "drwform.h"
#include "dlg_file.h"
#include "dlg_fnt.h"
#include "dlg_hlp.h"
#include "resource.h"
#include "drwutl.h"

PPRQINFO3     pPQInfoStruct;   /* Pointer to the PQRINFO3 struct          */
PVOID         pbuf;            /* Pointer to the buffer of info structs   */

UCHAR       pszDriverName [150];
UCHAR       pszDeviceName [150];
UCHAR       pszPrinterName[150];
UCHAR       pszQueueName[50];
USHORT      usIndex = 0;
USHORT      QueueCount;        /* Number of found queue's                 */

LONG     lFormTopClip;
LONG     lFormBottomClip;
LONG     lFormLeftClip;
LONG     lFormRightClip;
/*
** Local data
*/
static DEVOPENSTRUC dop;   /* Used to get our hpsMem comp with printer    */
static HPS    hpsPrn;      /* PrinterPS                                   */
static HDC    hdcPrn;      /* printer devicecontext                       */
static HDC    hdcBitmap;   /* For blitting the bitmaps on the printer     */
static BOOL   bDraft;      /* draft printing Y|N                          */
static BOOL   bFillPage;   /* stretch printout over printer page.         */
static WINDOWINFO PrInfo;  /* Holds all relevant info about the printer   */

#ifdef DEMOVERSION
char demo1[] = "Printed with a demo version of DrawIt for OS/2 Warp";
char demo2[] = "Register at http://www.bmtmicro.com/catalog/drawit.html";
#endif
/*-----------------------------------------------[ DeletePS ]-------------*/
/*  DeletePS.                                                             */
/*                                                                        */
/*  Description : Deletes the printer PS without destroying our queue     */
/*                info.                                                   */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void DeletePS(void)
{
   if (hpsPrn)
   {
      GpiDestroyPS(hpsPrn);
      hpsPrn = 0;
   }
   if (hdcPrn)
   {
      DevCloseDC(hdcPrn);
      hdcPrn = 0;
   }

   if (hdcBitmap)
   {
      DevCloseDC(hdcBitmap);
      hdcBitmap = 0;
   }

   if (PrInfo.hpsMem)
   {
      GpiDestroyPS(PrInfo.hpsMem);
      PrInfo.hpsMem = 0;
   }
   return;
}
/*-----------------------------------------------[ private ]--------------*/
/* CreatePrnBmpHdcHps.                                                    */
/*                                                                        */
/* Creates a printer compatible device context plus MEMORY PS.            */
/*------------------------------------------------------------------------*/
static BOOL CreatePrnBmpHdcHps( PHDC phdc, PHPS phps)
{
   SIZEL sizl;
   HDC   hdc;
   HPS   hps;

   hdc = DevOpenDC(hab,OD_MEMORY,"*",3L,(PDEVOPENDATA)&dop,
                   hdcPrn);                      /* Printer compatibility */

   sizl.cx = 2000; /* 20 cm */
   sizl.cy = 3000; /* 30 cm max?*/

   hps = GpiCreatePS(hab,hdc,&sizl,
                     GPIT_NORMAL|PU_LOMETRIC | GPIF_DEFAULT | GPIA_ASSOC );
   if( !hps)
   {
       printf("[Line %d] Could not create printer presentation space\n",__LINE__);
      return( FALSE);
   }

   *phdc = hdc;
   *phps = hps;
   return( TRUE);
}
/*-----------------------------------------------[ private ]--------------*/
/* Name : EnumQueues.                                                     */
/*                                                                        */
/* Description: Query's the printernames, printerdevicedriver             */
/*                                                                        */
/*      Globals : pszDriverName : printer drivername                      */
/*                pszDeviceName :    "    devicename                      */
/*                pszPrinterName:    "    name                            */
/*                pBuf : points to a buffer containing the printqueue     */
/*                       info structs.                                    */
/*                Queuecount: The number of queues found.                 */
/*------------------------------------------------------------------------*/
static BOOL EnumQueues(WINDOWINFO *pwi)
{
   ULONG        usRetEnt;        /* Returned number of queue info 3 structs */
   ULONG        usAvailEnt;      /* Number of available queues              */
   ULONG        cbNeeded;        /*                                         */
   SPLERR       usErr;           /* Return code                             */
   ULONG        cbBuf;
   ULONG        uLevel;
   PSZ          pszComputerName;

   /*Allocate space for queunames */
   /*Enumerate the queue's        */

   pszComputerName = (PSZ)NULL;
   uLevel = 3L;
   pbuf   = NULL;
   usErr = DosPrintQEnum(pszComputerName,  /* NULL = Local machine        */
                         uLevel,           /* Information level           */
                         pbuf,             /* pBuf - NULL for first call  */
                         0L,               /* 0 to get size needed        */
                         &usRetEnt,        /* number of queues returned   */
                         &usAvailEnt,      /* total number of queues      */
                         &cbNeeded,        /* number bytes needed to store*/
                         NULL);            /* reserved                    */

   if (usErr == ERROR_MORE_DATA || usErr == NERR_BufTooSmall )
   {
      /*
      ** There is more data
      */
      DosAllocMem(&pbuf,cbNeeded,PAG_READ | PAG_WRITE | PAG_COMMIT );
      /*
      ** Enumerate the queues again using infolevel 3
      */
      cbBuf = cbNeeded;

      usErr = DosPrintQEnum(pszComputerName,uLevel,pbuf,cbBuf,&usRetEnt,
                            &usAvailEnt,&cbNeeded,NULL);
   }
   else
   {
      /*
      ** If we are here there is probably no printer installed!!
      */
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                    "No printer installed",
                    "Error on printing",
                    0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                    MB_ICONEXCLAMATION);

      return FALSE;
   }

   if (!usErr)
   {
      pPQInfoStruct = (PPRQINFO3)pbuf;

      /*
      ** Remember always the first printer by default.
      ** and the number of available printers.
      */

      QueueCount = (USHORT)usAvailEnt;

      if (pPQInfoStruct->pszDriverName)
      {
         strcpy(pszDriverName,pPQInfoStruct->pszDriverName);
         strcpy(pszDeviceName, pPQInfoStruct->pDriverData->szDeviceName);
         strcpy(pszPrinterName,pPQInfoStruct->pszPrinters);

         printf("pszDriverName =%s\n",pPQInfoStruct->pszDriverName);
         printf("pszDeviceName =%s\n",pPQInfoStruct->pDriverData->szDeviceName);
         printf("pszPrinterName=%s\n",pPQInfoStruct->pszPrinters);
      }
       else
           printf("[Line %d] pPQInfoStruct->pszDriverName:NULL\n",__LINE__);
   }
   else
   {
       printf("[Line %d] EnumQueues :DosPrintQEnum returned:%d\n",__LINE__,usErr);
   }
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/* QueryJobProperties.                                                    */
/*                                                                        */
/* Desciption : Queries the job properties  of the current selected       */
/*              printer and creates the printer device context and        */
/*              associates the printer PS with it.                        */
/*                                                                        */
/* Pre condition:                                                         */
/*    The global variables: pszDriverName,pszDeviceName,pszPrinterName,   */
/*    should be filled with the apropriate values.                        */
/*    This means that the function Enumqueues must be called before this  */
/*    function is used!                                                   */
/*                                                                        */
/* Parameters : ULONG ulOption defines how the job properties are queried.*/
/*              DPDM_POSTJOBPROP the printer driver shows property sheet. */
/*              DPDM_QUERYJOBPROP the printer driver is only queried.     */
/*                                                                        */
/* Returns : PRNERR_NOPRINT, if the user pressed in the jobproperties     */
/*                           dialog on cancel.                            */
/*           DPDM_ERROR, memory allocation error.                         */
/*           PRNERR_DC, invallid printer HDC.                             */
/* Warning:                                                               */
/*    This function must be called before any printing can be done!       */
/*------------------------------------------------------------------------*/
static int QueryJobProperties(ULONG  ulOption, BOOL bCreatePSHDC)
{
   DEVOPENSTRUC DevData;
   PDRIVDATA    pDrivData;
   SIZEL  sizl;
   LONG   buflen;
   UCHAR  ProcParams[50];
   CHAR   *p;
   USHORT usCopies;

   sizl.cx = 0L;
   sizl.cy = 0L;

   if (hpsPrn || hdcPrn)
      DeletePS();

   p=strchr(pszPrinterName,',');
   if (p)
      *p=0;

   /* Use just the driver name from the driver.device string */
   p=strchr(pszDriverName,'.');
   if (p)
   {
      *p=0;
      p++;
      strcpy(pszDeviceName,p);
   }

   /*
   ** Get the buffer size needed to load the job properties.
   */
   buflen = DevPostDeviceModes( hab,(PDRIVDATA)NULL,pszDriverName,
                                pszDeviceName,pszPrinterName,
                                DPDM_QUERYJOBPROP);

   /* allocate some memory for larger job properties and */

   if ( (pDrivData = (PDRIVDATA)calloc(buflen+4096,sizeof(char))) == NULL)
      return(DPDM_ERROR);

   /* display job properties dialog and get updated */
   /* job properties from driver                    */

   pDrivData->cb = sizeof(DRIVDATA);

   DevPostDeviceModes( hab,pDrivData,pszDriverName,
                       pszDeviceName,pszPrinterName,
                       ulOption);

   /* If the user pressed CANCEL pDrivData.szDeviceName[0] = 0 */

   if (!pDrivData->szDeviceName[0])
   {
  
        strcpy(pDrivData->szDeviceName,pszDeviceName);
        pDrivData->cb = sizeof(DRIVDATA);
//      free((void *)pDrivData);
//      return 1;
   }

   /*
   ** Fill in the DEVOPENSTRUCT
   */
   DevData.pszDriverName   = pszDriverName;
   DevData.pdriv           = pDrivData;
   DevData.pszDataType     ="PM_Q_STD"; /*Standard printing (not raw) */
   DevData.pszComment      ="DrawIt";
   DevData.pszQueueProcName=NULL;

   usCopies = 1;
   sprintf(ProcParams,"XFM=0 COP=%d",usCopies);

   DevData.pszQueueProcParams=ProcParams;
   DevData.pszSpoolerParams=NULL;
   DevData.pszNetworkParams=NULL;
   DevData.pszLogAddress=pszQueueName;

   if (bCreatePSHDC)
   {
      hdcPrn   = DevOpenDC(hab,                   /* Anchor block handle   */
                           OD_QUEUED,              /* queued printing by def*/
                           "*",                    /* default token         */
                           9L,                     /* 9 Items in DevData    */
                           (PDEVOPENDATA)&DevData, /* pointer to DevData    */
                           (HDC)NULL);          /* No compatible DC      */

      hpsPrn = GpiCreatePS(hab,
                           hdcPrn,
                           &sizl,
                           GPIT_NORMAL | PU_LOMETRIC | GPIF_DEFAULT|GPIA_ASSOC);
      /*
      ** We should use RGB values instead of a 16 entry colortable.
      */
      GpiCreateLogColorTable (hpsPrn, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
   }
   free((void *)pDrivData);
   return (int)0;
}
/*-----------------------------------------------------------------------*/
MRESULT EXPENTRY QueryPrintDlgProc( HWND hwnd,USHORT msg,MPARAM mp1,MPARAM mp2)
{
   int                 i, index;
   PSZ                 psz;
   HWND                hwndListbox;
   int                 RetValue;
   SWP swp;            /* Screen Window Position Holder */

   switch (msg)
   {
   case WM_INITDLG:
       /*
       ** Centre dialog on the screen
       */
       WinQueryWindowPos(hwnd, (PSWP)&swp);
       WinSetWindowPos(hwnd, HWND_TOP,
                       ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);


      /* Fill listbox with print objects */

      hwndListbox = WinWindowFromID( hwnd, IDD_QPLISTBOX );

      /*
      ** Set pointer at the beginning of the buffer.
      */

      pPQInfoStruct = (PPRQINFO3)pbuf;

      for (i = 0; i < QueueCount; i++)
      {
         psz = pPQInfoStruct->pszComment;
         if (!psz)
            psz = pPQInfoStruct->pszName;
         index = WinInsertLboxItem( hwndListbox, LIT_END, psz );
         pPQInfoStruct++;
      }
      /*
      ** one must be selected
      */
      WinSendMsg(hwndListbox,LM_SELECTITEM,(MPARAM)0,(MPARAM)TRUE );
      WinFocusChange( HWND_DESKTOP, hwnd, 0L);
      return (MRESULT)1;

   case WM_COMMAND:
      switch(SHORT1FROMMP(mp1))
      {
       case IDD_QPJOBPROP:
          /*
          ** Flag that we visited the job properties dialog. Since
          ** DevPostDeviceModes (OS2_PM_DRV_DEVMODES) option
          ** DPDM_POSTJOBPROP has no return code or standard way to
          ** indicate user cancel, we have to assume something might
          ** have changed.
          */
          hwndListbox = WinWindowFromID( hwnd, IDD_QPLISTBOX );
          index = WinQueryLboxSelectedItem( hwndListbox );
          pPQInfoStruct = (PPRQINFO3)pbuf;
          pPQInfoStruct += index; /* Go the selected info struct */
          strcpy(pszDriverName,pPQInfoStruct->pszDriverName);
          strcpy(pszDeviceName, pPQInfoStruct->pDriverData->szDeviceName);
          strcpy(pszPrinterName,pPQInfoStruct->pszPrinters);
          strcpy(pszQueueName,pPQInfoStruct->pszName);
          QueryJobProperties(DPDM_POSTJOBPROP,FALSE);
          return 0;

       case DID_OK:
          hwndListbox = WinWindowFromID( hwnd, IDD_QPLISTBOX );
          index = WinQueryLboxSelectedItem( hwndListbox );
          pPQInfoStruct = (PPRQINFO3)pbuf;
          pPQInfoStruct += index;
          /*
          ** Go the selected info struct
          */
          strcpy(pszDriverName, pPQInfoStruct->pszDriverName);
          strcpy(pszDeviceName, pPQInfoStruct->pDriverData->szDeviceName);
          strcpy(pszPrinterName,pPQInfoStruct->pszPrinters);
          strcpy(pszQueueName,  pPQInfoStruct->pszName);
          QueryJobProperties(DPDM_QUERYJOBPROP,TRUE);
          WinDismissDlg( hwnd, DID_OK );
          return 0;
       case DID_CANCEL:
          WinDismissDlg(hwnd,DID_CANCEL);
          return (MRESULT)NULL;
       case DID_HELP:
          ShowDlgHelp(hwnd);
          return (MRESULT)0;
    }
      break;
   }
   return( WinDefDlgProc(hwnd, msg, mp1, mp2) );
} /* End of QueryPrintDlgProc */
/*-----------------------------------------------[ private ]----------------*/
/* PrnGetFormSize.                                                          */
/*                                                                          */
/* Description   : Gets the size of the active form of the selected printer */
/*                                                                          */
/* Post cond     : hpsPrn and HdcPrn needs to be set before this function   */
/*                 can be called.                                           */
/*                                                                          */
/* Returns       : int error number.                                        */
/*--------------------------------------------------------------------------*/
static int PrnGetFormSize(WINDOWINFO *PrtInfo)
{
   PHCINFO phciForms;                  /* pointer to forms array     */
   LONG    NumForms;
   USHORT  i;                          /* Loop counter               */

   if (!hpsPrn)
      return DPDM_ERROR;           /* No presentation space ! */

   /*---------------------------------------------------------------------*/
   /* Query hard copy capabilities of the printer.                        */
   /* We only want the first ten formcodes.                               */
   /* 1. Query number of forms.                                           */
   /* 2. Allocate Memory.                                                 */
   /* 3. Query the forms.                                                 */
   /* 4. Locate the current form.                                         */
   /* 5. Convert the forms information from mm to PU_LOMETRICS of 0.1mm   */
   /* 6. Free the memory.                                                 */
   /*---------------------------------------------------------------------*/

   NumForms = DevQueryHardcopyCaps(hdcPrn,          /* Device context      */
                                   0L,              /* Start forms-code    */
                                   0L,              /* Nr of forms to query*/
                                   NULL);           /* Details of form     */

   phciForms = (HCINFO *)calloc((SHORT)NumForms*sizeof(HCINFO),1);


   NumForms = DevQueryHardcopyCaps(hdcPrn,          /* Device context      */
                                   0L,              /* Start forms-code    */
                                   NumForms,        /* Nr of forms to query*/
                                   phciForms);      /* Details of form     */


   for (i=0; i < NumForms-1 &&
                 !(phciForms[i].flAttributes & HCAPS_CURRENT); i++);

   lFormTopClip    = phciForms[i].yTopClip ;
   lFormBottomClip = phciForms[i].yBottomClip;
   lFormLeftClip   = phciForms[i].xLeftClip;
   lFormRightClip  = phciForms[i].xRightClip;
   /*
   ** Form width and height in mm convert to 0.1 milimeters.
   */

//   PrtInfo->usFormHeight = (USHORT)phciForms[i].cy * 10;
//   PrtInfo->usFormWidth  = (USHORT)phciForms[i].cx * 10;

   PrtInfo->usFormHeight = (USHORT)((lFormTopClip - lFormBottomClip) * 10);
   PrtInfo->usFormWidth  = (USHORT)((lFormRightClip - lFormLeftClip) * 10);
   PrtInfo->usHeight     = PrtInfo->usFormHeight;
   PrtInfo->usWidth      = PrtInfo->usFormWidth;

        printf("Formname: %s\n",phciForms[i].szFormname);
        printf("Width  in 0.1 milimeters  %d\n",PrtInfo->usFormWidth);
        printf("Height in 0.1 milimeters  %d\n",PrtInfo->usFormHeight);
        printf("yTopClip in   milimeters  %d\n",lFormTopClip);
        printf("yBottomClip   milimeters  %d\n",lFormBottomClip);
        printf("xLeftClip     milimeters  %d\n",lFormLeftClip);
        printf("xRightClip    milimeters  %d\n",lFormRightClip);


   for (i=0; i < NumForms-1; i++)
   {
     if (phciForms[i].flAttributes & HCAPS_SELECTABLE)
     {
        printf("Formname: %s\n",phciForms[i].szFormname);
        printf("Width  in 0.1 milimeters  %d\n",PrtInfo->usFormWidth);
        printf("Height in 0.1 milimeters  %d\n",PrtInfo->usFormHeight);
        printf("yTopClip in   milimeters  %d\n",lFormTopClip);
        printf("yBottomClip   milimeters  %d\n",lFormBottomClip);
        printf("xLeftClip     milimeters  %d\n",lFormLeftClip);
        printf("xRightClip    milimeters  %d\n",lFormRightClip);
     }
   }

   free(phciForms);
   return 0;
}
/* --------------------------------------------------------------------------*/
/* - ClosePrinter:  Destroys the gobal Printer PS and the device context.    */
/* --------------------------------------------------------------------------*/
void ClosePrinter ( void )
{
   DeletePS();

   if (pbuf) 
      DosFreeMem(pbuf);
}
/*-------------------------------------------------------------------*/
#ifdef DEMOVERSION
void printDemoString(WINDOWINFO *pr, HWND hwnd,HWND hStatus)
{
   POINTL ptl;
   SIZEF  sizfx;
   FATTRS fattrs;

   ptl.x = 100;
   ptl.y = 80;

   /*
   ** Check whether the exe has been patched to get rid of the 
   ** strings at the bottom of each printout.
   */
   if (demo1[0] != 80 || demo1[1] != 114 )
   {
      WinPostMsg(hwnd,UM_PATCHED,(MPARAM)0,(MPARAM)0);
      WinPostMsg(hStatus,UM_READWRITE,(MPARAM)0,(MPARAM)0);
      DevEscape(hdcPrn,DEVESC_ENDDOC,0L,NULL,0L,NULL);
      /** Cleanup **/
      ClosePrinter ();
      DosExit(EXIT_THREAD,0); /* Kill printing.... */
   }
   sizfx.cx = 250 * pr->usFormWidth;
   sizfx.cy = 250 * pr->usFormHeight;

   memset(&fattrs,0,sizeof(FATTRS));

   fattrs.usRecordLength    = sizeof(FATTRS);
   fattrs.fsSelection       = 0;
   fattrs.lMatch            = 0;
   fattrs.idRegistry        = 0;
   fattrs.usCodePage        = GpiQueryCp(pr->hps);
   fattrs.fsType            = 0;
   fattrs.fsFontUse         = FATTR_FONTUSE_OUTLINE;
   fattrs.lMaxBaselineExt   = 55;
   fattrs.lAveCharWidth     = 55;
   strcpy (fattrs.szFacename, "Courier");

   setFont(pr,&fattrs,sizfx);

   GpiCharStringAt(pr->hps,&ptl,(LONG)strlen(demo1),demo1);
   ptl.y -= 50;
   GpiCharStringAt(pr->hps,&ptl,(LONG)strlen(demo2),demo2);
}
#endif
/*===================================================================*/
/* printwrite........                                                */
/*                                                                   */
/*===================================================================*/
void _System PrintWrite(WINDOWINFO *pwi)
{
   POINTL ptl;

   PrnGetFormSize(&PrInfo);

   DevEscape(hdcPrn,DEVESC_STARTDOC,(ULONG)strlen("DrawIt"),
             (PBYTE)"DrawIt",0L,(PBYTE)NULL);

   PrInfo.cyClient     = 0;
   PrInfo.bSuppress    = FALSE;
   PrInfo.lcid         = 4L;
   PrInfo.usdrawlayer  = pwi->uslayer;
   PrInfo.uslayer      = pwi->uslayer;
   PrInfo.lBackClr     = pwi->lBackClr;
   PrInfo.ulUnits      = PU_LOMETRIC;
   PrInfo.uYfactor      = (float)1;
   PrInfo.uXfactor      = (float)1;

   /*
   ** If the user does not wanna fill out the page because it would like
   ** to print a cd-cover (for instance) on a A4/Letter than we should 
   ** override the printer page size with the form size. 
   */
   if (!bFillPage)
   {
      PrInfo.usFormHeight = pwi->usHeight;
      PrInfo.usFormWidth  = pwi->usWidth;
      PrInfo.usHeight     = pwi->usHeight;
      PrInfo.usWidth      = pwi->usWidth;
   }
   /*
   ** Get our memory PS which will be associated with the printer device
   */
   if (!hdcBitmap || !PrInfo.hpsMem)
      CreatePrnBmpHdcHps(&hdcBitmap,&PrInfo.hpsMem);

   PrInfo.hps       = hpsPrn;
   PrInfo.hpsScreen = pwi->hpsScreen;
   PrInfo.bPrinter  = TRUE;
   PrInfo.bDBCS     = pwi->bDBCS;

#ifdef DEMOVERSION
   printDemoString(&PrInfo,pwi->hwndClient,pwi->hwndMain);
#endif

   if (!bDraft)
   {
      for ( PrInfo.usdrawlayer = MINLAYER; PrInfo.usdrawlayer <= MAXLAYER; PrInfo.usdrawlayer++)
      {
         ObjDrawSegment(hpsPrn,&PrInfo,(POBJECT)0, (RECTL *)0);
      }
   }
   else
   {
      for ( PrInfo.usdrawlayer = MINLAYER; PrInfo.usdrawlayer <= MAXLAYER; PrInfo.usdrawlayer++)
      {
         ObjMultiDrawOutline(&PrInfo,(POBJECT)0,FALSE);
      }
   }
   if (!bFillPage)
   {
      /*
      ** And here we print (when the page is not filled out) a box around the 
      ** printed area to give the user the possibility to easily cut out the
      ** print.
      */
      ptl.x = 1;
      ptl.y = 1;
      GpiMove(hpsPrn,&ptl);
      ptl.x = PrInfo.usWidth;
      ptl.y = PrInfo.usHeight;
      GpiBox(hpsPrn,DRO_OUTLINE,&ptl,0L,0L);
   }
   DevEscape(hdcPrn,DEVESC_ENDDOC,0L,NULL,0L,NULL);
   /*
   ** Cleanup
   */
   ClosePrinter ();
   /*
   ** Tell status line that we are done.
   */
   WinPostMsg(pwi->hwndMain, UM_READWRITE,
                          (MPARAM)0,(MPARAM)0);

}
/*----------------------------PRINT PREVIEW DIALOG PROCEDURE ---------------*/
MRESULT EXPENTRY PrintPrevDlgProc( HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
   SWP swp;            /* Screen Window Position Holder */
   static WINDOWINFO *pwi;    /* Screen window info struct!    */
   CHAR  szBuffer[15];
   CHAR  szFrmname[50];
   static HWND hPreview;

   switch (msg)
   {
      case WM_INITDLG:
         pwi       = (WINDOWINFO *)mp2;

             /* Centre dialog   on the screen         */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);
         WinSendDlgItemMsg(hwnd,ID_PREVOUTLINE,BM_SETCHECK,
                           (MPARAM)0,(MPARAM)0);

         WinSendDlgItemMsg(hwnd,ID_PRNSTRETCH,BM_SETCHECK,
                           (MPARAM)1,(MPARAM)1);

         hPreview = WinWindowFromID(hwnd,ID_PREVWND);

         bDraft    = FALSE;
         bFillPage = TRUE;
         /*
         ** Display form info.
         */
         if (pwi->paper == IDM_MM)
         {
            /*
            ** 0.1 mm precision.
            */
            WinSetDlgItemText (hwnd,TXT_FORMWIDTH,_itoa(pwi->usWidth, szBuffer, 10));
            WinSetDlgItemText (hwnd,TXT_FORMHEIGHT,_itoa(pwi->usHeight, szBuffer, 10));
         }
         else
         {
            int iInch =(int)(pwi->usWidth/2.54);
            WinSetDlgItemText (hwnd,TXT_FORMWIDTH,_itoa(iInch,szBuffer,10));
            iInch =(int)(pwi->usHeight/2.54);
            WinSetDlgItemText (hwnd,TXT_FORMHEIGHT,_itoa(iInch,szBuffer,10));
         }


         if (Size2Name(pwi,szFrmname, sizeof(szFrmname)))
            WinSetDlgItemText (hwnd,TXT_FORMNAME,szFrmname);
         return 0;
      case WM_CONTROL:
         switch(LOUSHORT(mp1))
         {
            case ID_PREVOUTLINE:
               bDraft = !bDraft;
               setDraftPreview( bDraft );
               WinInvalidateRect(hPreview,(RECTL *)0,TRUE);
               break;
            case ID_PRNSTRETCH:
               bFillPage = !bFillPage;
               break;
         }
         return (MRESULT)0;


      case WM_COMMAND:
         switch(LOUSHORT(mp1))
         {
              case DID_OK:
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
/* NAME : PrintOpen.                                                      */
/*                                                                        */
/* Input: NONE.                                                           */
/*                                                                        */
/* Returns : 0 on success.                                                */
/*           DPDM_ERROR if there are problems with the printer.           */
/*                                                                        */
/* Creates : A printer device context : hdcPrn                            */
/*           A printer presentation space of PU_LOMETRIC : hpsPrn.        */
/* Description:                                                           */
/*       First gets the device, driver and printername. And displays the  */
/*       printer setup dialog since we are using the DPDM_POSTJOBPROP mode*/
/*------------------------------------------------------------------------*/
BOOL PrintOpen(WINDOWINFO *pwi)
{
   if (!EnumQueues(pwi))
      return FALSE;

   if (WinDlgBox(HWND_DESKTOP,hwndFrame,(PFNWP)QueryPrintDlgProc,
                (HMODULE)0,IDD_QUERYPRINT,(PVOID)0) == DID_CANCEL)
      return FALSE;
   else
      return TRUE;
}
