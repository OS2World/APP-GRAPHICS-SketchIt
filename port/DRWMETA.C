/*------------------------------------------------------------------------*/
/*  Name: DRWMETA.C                                                       */
/*                                                                        */
/*  Description : OS/2 Metafile functions.                                */
/*                                                                        */
/* Private functions:                                                     */
/*    FileGet            : Filebox for the demo version of drawit.        */
/*    CreateMetHandle    :                                                */
/*                                                                        */
/* Public functions:                                                      */
/*    copyMetaFileObject : Makes a copy of the given metafile object.     */
/*    FileGetMetaData    :                                                */
/*    FilePutMetaData    :                                                */
/*    MetaDlgProc        :                                                */
/*    MetaDetail         :                                                */
/*    SetMetaSizes       :                                                */
/*    OpenMetaSegment    :                                                */
/*    DrawMetaSegment    :                                                */
/*    MetaPictOutLine    :                                                */
/*    MetaStretch        :                                                */
/*    MetaPictInvArea    :                                                */
/*    MetaPictSelect     :                                                */
/*    MetaMoveOutline    :                                                */
/*    MetaPictMove       :                                                */
/*    setMetaPosition    :                                                */
/*    ScaleMetaFile      :                                                */
/*    GetMetaFile        :                                                */
/*    saveMetaPicture    :                                                */
/*    RecordMetaFile     :                                                */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DEV
#define INCL_GPIERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <direct.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_sqr.h"
#include "dlg_txt.h"  /* To get floating rectl struct. */
#include "dlg_fnt.h"
#include "drwutl.h"
#include "drwmeta.h"
#include "resource.h"

#define METAPICSIZE  720.0    /* Nr of units the picture gets initially */

static WINDOWINFO MetaInfo;  /* Holds all relevant info for the metafile */
static BOOL DlgLoaded;


#ifdef DEMOVERSION
/*-----------------------------------------------------------------------*/
/*  Name:   FileGet()                                                    */
/*-----------------------------------------------------------------------*/
static BOOL FileGet(pLoadinfo pli, WINDOWINFO *pwi )
{
   PSZ apszITL[]={"DrawIt", (PSZ)NULL};
   struct stat bf;
   char     buf[CCHMAXPATH];
   char     szExtension[20]; /* for open files */
   USHORT   usResponse;
   BOOL     bDone=FALSE;
   FILEDLG  fdg;
   char     TitleBuf[50];
   char     szTitle[100]; /* title for the messagebox loaded from resource */
   char  *p;

   memset(&fdg, 0,sizeof(FILEDLG));
   /*
   ** set up structure to pass to standard file dialog
   */
   fdg.cbSize = sizeof(FILEDLG);
   fdg.fl = pli->dlgflags | FDS_CENTER;

   strcpy(TitleBuf,"DrawIt - Default extension = ");
   strcat(TitleBuf,pli->szExtension);

   strcpy(szExtension,"*"); /* to make later *.net,*.jsp etc or *.* */

   fdg.pszTitle = TitleBuf;

   fdg.pszOKButton = "OK";
   fdg.ulUser = 0L;
   fdg.pfnDlgProc = (PFNWP)NULL;        
   fdg.lReturn = 0L;
   fdg.lSRC = 0;
   fdg.hMod = 0;
   fdg.usDlgId = 0;
   fdg.x = 0;
   fdg.y = 0;
   fdg.pszIType = (PSZ)NULL;
   fdg.papszITypeList = (PAPSZ)NULL;
   fdg.pszIDrive = (PSZ)NULL;
   fdg.papszIDriveList = (PAPSZ)NULL;
   fdg.sEAType = (SHORT)0;
   fdg.papszFQFilename = (PAPSZ)NULL;
   fdg.ulFQFCount = 0L;
   fdg.szFullFile[0]=0;
 
   /*
   ** if dialog has been previously invoked, then ensure the
   ** dialog is brought with the last user-selected directory
   ** start with FDS_OPEN_DIALOG overwrite if not this....
   */

   if (pli->dlgflags & FDS_OPEN_DIALOG)
   {
      if (!strlen(pli->szdefdir))
      {
         pli->szdefdir[0]=0;
         strcat(szExtension,pli->szExtension);
         strcpy(fdg.szFullFile,szExtension);
      }
      else
      {
         strcpy(fdg.szFullFile,pli->szdefdir);
         strcat(fdg.szFullFile,"\\");
         strcat(szExtension,pli->szExtension);
         strcat(fdg.szFullFile,szExtension);
      }
   }

   if (pli->dlgflags & FDS_SAVEAS_DIALOG)
   {
      if (pli->szFileName[0])
         strcpy(fdg.szFullFile, pli->szFileName);
   }
   fdg.sEAType = -1;
   fdg.papszFQFilename = 0;
   fdg.ulFQFCount = 0;

   /*
   ** invoke the standard file dialog and get a filename
   ** If we are getting a filename for saving data we check
   ** if the file already exists. And if true we ask the
   ** if it is ok to use this filename.
   */

   if (pli->dlgflags & FDS_SAVEAS_DIALOG)
   {
      do
      {
         if (!WinFileDlg(HWND_DESKTOP,pwi->hwndMain,(PFILEDLG)&fdg))
            return FALSE;
         /*
         ** Don't touch the pli->szFileName if the cancel button
         ** is pressed. So quit.
         */
         if (fdg.lReturn == DID_CANCEL)
            return FALSE;

         strcpy(pli->szFileName, fdg.szFullFile);
         if (!(p = strchr(pli->szFileName,'.')) )
            strcat(pli->szFileName,pli->szExtension);
         else if (stricmp(p,pli->szExtension))
         {
            /*
            ** If the extension is not as it should be
            ** we simply overwrite it with the default.
            */
            *p = 0;
            strcat(pli->szFileName,pli->szExtension);
         }

         bDone = FALSE;
         if (_stat(pli->szFileName,&bf) == 0)
         {
            WinLoadString(hab, (HMODULE)0, IDS_WARNING,100,szTitle);

            sprintf(buf,"File: %s \nalready exists!\n"\
                    "Do you want to overwrite it?",pli->szFileName);

            usResponse = WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                                       (PSZ)buf,(PSZ)szTitle, 0,
                                       MB_YESNOCANCEL | MB_APPLMODAL |
                                       MB_MOVEABLE | 
                                       MB_ICONQUESTION);
            bDone = TRUE;
         }
     } while (usResponse == MBID_NO && bDone);

     if (usResponse == MBID_CANCEL)
        return FALSE;
   }
   else
   {
      /*
      ** OPEN FILEDIALOG...
      */
      if (!WinFileDlg(HWND_DESKTOP,pwi->hwndMain,(PFILEDLG)&fdg))
         return FALSE;

      if (fdg.lReturn == DID_CANCEL)
         return FALSE;
   }

   /*
   **  Upon sucessful return of a filename in the struct *loadinfo struct.
   */

   if (fdg.lReturn == DID_OK)
   {
      strcpy(pli->szFileName, fdg.szFullFile);

      if (pli->dlgflags & FDS_SAVEAS_DIALOG)
      {
         if (!(p = strchr(pli->szFileName,'.')) )
            strcat(pli->szFileName,pli->szExtension);
         else if (stricmp(p,pli->szExtension))
         {
         /*
         ** If the extension is not as it should be
         ** we simply overwrite it with the default.
         */
         *p = 0;
         strcat(pli->szFileName,pli->szExtension);
      }
   }
   else
   {
      /*
      ** Help the user who forgot the extension for opening
      ** the file.
      */
      if (!(p = strchr(pli->szFileName,'.')) )
         strcat(pli->szFileName,pli->szExtension);
   }
   p = strrchr(fdg.szFullFile,'\\');
   /*
   ** Finally save the directory in the loadinfo struct.
   ** So whenever the user comes back he/she would not
   ** have to travel all the way back to the directory.
   */
   if (p)
   {
      *p=0;
      strcpy(pli->szdefdir,fdg.szFullFile);
   }
   return TRUE;
 }
 else
    return FALSE;

}   /* end FileGet() */
#endif
/*-----------------------------------------------[ public ]---------------*/
/*  Name: copyMeteFileObject.                                             */
/*                                                                        */
/*  Make a copy of the given metafile object and returns the reference    */
/*  to the new created object on success. Otherwise NULL.                 */
/*------------------------------------------------------------------------*/
POBJECT copyMetaFileObject( POBJECT pObj)
{
   POBJECT   pCopy;
   pMetaimg  pMetaOrg = (pMetaimg)pObj;
   pMetaimg  pMetaCopy;

   if (!pObj)
      return NULL;
   if (pObj->usClass != CLS_META)
      return NULL;

   pCopy = pObjNew(NULL,CLS_META);

   memcpy(pCopy,pObj,ObjGetSize(CLS_META));

   pMetaCopy = (pMetaimg)pCopy;

   pMetaCopy->hmf      = GpiCopyMetaFile(pMetaOrg->hmf);
   pMetaCopy->pbBuffer = NULL;

   pCopy->paint			= DrawMetaSegment;
   pCopy->moveOutline	        = MetaMoveOutline;
   pCopy->getInvalidationArea   = MetaPictInvArea;

   if (!pMetaCopy->hmf)
   {
      free(pCopy);
      return NULL;
   }
   return pCopy;
}
/*-----------------------------------------------[ private ]--------------*/
/*  Name: CreateMetHandle.                                                */
/*                                                                        */
/*  Creates a metafile memory handle to put in the metafiledata when      */
/*  loaded from a JSP file.                                               */
/*------------------------------------------------------------------------*/
static HMF CreateMetHandle(HAB hab )
{
   HDC  hdc;
   DEVOPENSTRUC dopData;

   dopData.pszLogAddress       = NULL;
   dopData.pszDriverName       = "DISPLAY";
   dopData.pdriv               = NULL;
   dopData.pszDataType         = NULL;
   dopData.pszComment          = NULL;
   dopData.pszQueueProcName    = NULL;
   dopData.pszQueueProcParams  = NULL;
   dopData.pszSpoolerParams    = NULL;
   dopData.pszNetworkParams    = NULL;

   hdc = DevOpenDC(hab,OD_METAFILE,"*",9L,
                   (PDEVOPENDATA)&dopData,(HDC)0);

   if (hdc != DEV_ERROR)
   {
      return (HMF)DevCloseDC(hdc);
   }
   else
      return (HMF)0;
}
/*------------------------------------------------------------------------*/
/*  Name: FileGetMetaData.                                                */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL FileGetMetaData(pLoadinfo pli, POBJECT pObj)
{
   pMetaimg  pMeta = (pMetaimg)pObj;
   BOOL      bRet  = FALSE;
   int       i;
   /*
   ** Allocate the buffer for the metafile data.
   */
   pMeta->pbBuffer = (PBYTE)calloc((ULONG)pMeta->cBytes,1);

   if (!pMeta->pbBuffer)
   {
      pMeta->cBytes = 0;
      pMeta->hmf    =(HMF)0;
      return FALSE;
   }

   pMeta->hmf = CreateMetHandle( hab );

   if (!pMeta->hmf) return FALSE;

   i = read(pli->handle,(PVOID)pMeta->pbBuffer,pMeta->cBytes);

   if (i > 0)
   {
      bRet = TRUE;
      GpiSetMetaFileBits(pMeta->hmf,
                         0L,
                         pMeta->cBytes,
                         pMeta->pbBuffer);
   }
   free((void *)pMeta->pbBuffer);

   pObj->paint                 = DrawMetaSegment;
   pObj->moveOutline	       = MetaMoveOutline;
   pObj->getInvalidationArea   = MetaPictInvArea;

   return bRet;
}
/*------------------------------------------------------------------------*/
/*  Name: FilePutMetaData.                                                */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL FilePutMetaData(pLoadinfo pli, POBJECT pObj)
{
   pMetaimg  pMeta = (pMetaimg)pObj;

   if (!pMeta)  return FALSE;

   if (!pMeta->cBytes)
      return FALSE;

   /* Allocate the buffer for the metafile data. */

   pMeta->pbBuffer = (PBYTE)calloc((ULONG)pMeta->cBytes,1);

   GpiQueryMetaFileBits(pMeta->hmf,       /* handle of metafile              */
                        0L,               /* offset of next byte to retrieve */
                        pMeta->cBytes,    /* length of data                  */
                        pMeta->pbBuffer); /* buffer to receive metafile data */

   _write(pli->handle,(PVOID)pMeta->pbBuffer,pMeta->cBytes);

   free((void *)pMeta->pbBuffer);
   pMeta->pbBuffer = NULL;
   return TRUE;
}
/*------------------------------------------------------------------------*/
/*  Name: MetaDlgProc.                                                    */
/*                                                                        */
/*  Description : Dialog procedure handles the standard dialog for        */
/*                changing the layer of the selected metafile.            */
/*                Uses the same dialog as for the square/circle.          */
/*                Only the procedure is different.                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY MetaDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   ULONG  ulLayer;
   ULONG  ulStorage[2];    /* To get the vals out of the spins  */
   PVOID  pStorage;
   SWP    swp;
   static pMetaimg pMeta;

   switch (msg)
   {
      case WM_INITDLG:
         pMeta = (pMetaimg)mp2;
         /* Centre dialog on the screen	*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         WinSetWindowText(hwnd,"Detail Metafile Object");
         ulLayer = (ULONG)pMeta->uslayer;
         /* Set the layer spinbutton */

         WinSendDlgItemMsg( hwnd, 864, SPBM_SETLIMITS,
                            MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

         WinSendDlgItemMsg( hwnd, 864, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)ulLayer), NULL);
         return 0;

      case WM_COMMAND:
	   switch(LOUSHORT(mp1))
	   {
              case DID_OK:
                 pStorage = (PVOID)ulStorage;
                 WinSendDlgItemMsg( hwnd,864,SPBM_QUERYVALUE,
                                    (MPARAM)(pStorage),MPFROM2SHORT(0,0));
                 pMeta->uslayer= (USHORT)ulStorage[0];
                 WinDismissDlg(hwnd,TRUE);
                 DlgLoaded = FALSE;
                 return 0;
              case DID_CANCEL:
                 WinDismissDlg(hwnd,TRUE);
                 DlgLoaded = FALSE;
                 return 0;
           }
           return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  Name: MetaDetail.                                                     */
/*------------------------------------------------------------------------*/
void MetaDetail(HWND hwnd,POBJECT pObj )
{
   if (pObj && !DlgLoaded)
   {
      if (WinLoadDlg(HWND_DESKTOP,hwnd,(PFNWP)MetaDlgProc,(HMODULE)0,
	             850,(PVOID)pObj))
         DlgLoaded = TRUE;
      else
         DlgLoaded = FALSE;
   }
   else
      WinAlarm(HWND_DESKTOP, WA_WARNING);
   return;
}
/*------------------------------------------------------------------------*/
/*  Name: SetMetaSizes.                                                   */
/*                                                                        */
/*  Description : Calculates the initial sizes when a metafile is loaded. */
/*                Used when a metafile is loaded from disk or when        */
/*                created from a Lotus pic file for instance. (see        */
/*                drwpic.c or this file.                                  */
/*                                                                        */
/*                WINDOWINFO * - Who is calling us...                     */
/*                pMetaimg   -  Pointer to an metafile object.            */
/*                                                                        */
/*------------------------------------------------------------------------*/
void SetMetaSizes(pMetaimg pMeta, WINDOWINFO *pwi)
{
   HWND hParent;
   POINTL pt;

   if (pMeta && pMeta->hmf)
   {
      /*
      ** From client to frame of drawing area.
      */
      hParent = WinQueryWindow(pwi->hwndClient,QW_PARENT);
      /*
      ** From frame to viewport or canvas.
      */
      hParent = WinQueryWindow(hParent,QW_PARENT);
      pt.x = 0;
      pt.y = 0;
      WinMapWindowPoints(hParent,pwi->hwndClient,&pt,1L);
      pt.x += 200; /* 200 * 0.1 mm */
      pt.y += 200; /* 200 * 0.1 mm */
      GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1L,&pt);

      pMeta->rclf.xLeft   = (float) pt.x;
      pMeta->rclf.xLeft   /=(float) pwi->usFormWidth;
      pMeta->rclf.yBottom = (float) pt.y;
      pMeta->rclf.yBottom /=(float) pwi->usFormHeight;
      pMeta->rclf.xRight  = pMeta->rclf.xLeft +
                            (METAPICSIZE / (float) pwi->usFormWidth );
      pMeta->rclf.yTop    = pMeta->rclf.yBottom +
                            (METAPICSIZE / (float) pwi->usFormHeight);

      pMeta->ustype       = CLS_META;
      pMeta->uslayer      = pwi->uslayer;
      /*
      ** Get length of metafile used when the object is saved.
      */
      pMeta->cBytes = GpiQueryMetaFileLength(pMeta->hmf);
   }
}
/*------------------------------------------------------------------------*/
/*  Name: OpenMetaSegment.                                                */
/*                                                                        */
/*  Description : Creates an metafile segment in the main chain.          */
/*                                                                        */
/*  Parameters  : POINTL ptl - where should we put the picture?           */
/*                WINDOWINFO * - Who is calling us...                     */
/*                HMF        - NULL if called from the main mod or        */
/*                             a valid handle when called from the clip-  */
/*                             board paste routine.                       */
/*                                                                        */
/*------------------------------------------------------------------------*/
POBJECT OpenMetaSegment(POINTL ptl,char *pszFile,WINDOWINFO *pwi, HMF hmf)
{
   POBJECT pObj;
   pMetaimg  pMeta = (pMetaimg)0;
   pMeta = (pMetaimg)pObjCreate(NULL,CLS_META);

   if (pMeta)
   {
      if (!hmf) /* Are we called via the file import???? */
         pMeta->hmf = GetMetaFile(pMeta,pszFile,pwi);
      else
         pMeta->hmf = hmf; /* No from the clipboard, ukkel */
   }
   SetMetaSizes(pMeta,pwi);

   pObj = (POBJECT)pMeta;
   pObj->paint                 = DrawMetaSegment;
   pObj->moveOutline	       = MetaMoveOutline;
   pObj->getInvalidationArea   = MetaPictInvArea;

   return (POBJECT)pMeta;
}
/*--------------------------------------------------------------------------*/
VOID DrawMetaSegment(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl)
{
   RECTL     rcl,rclDest;
   ULONG     idPs;
   pMetaimg  pMeta = (pMetaimg)pObj;

   if (!pMeta || !pMeta->hmf)
      return;

   /*
   ** Since metafile drawing can be very time consuming,
   ** we first look (only if prect is a vallid pointer)
   ** if we should redraw it.
   */
   if (prcl)
   {
      MetaPictOutLine(pObj,&rcl,pwi);
      if (!WinIntersectRect(hab,&rclDest,prcl,&rcl))
        return;
   }

   idPs = GpiSavePS(hps);

   if (pwi->usdrawlayer == pMeta->uslayer)
   {
      MetaPictOutLine(pObj,&rcl,pwi);
      ScaleMetaFile(hps,pMeta->hmf,&rcl);
   }

   GpiRestorePS(hps,idPs);
}
/*------------------------------------------------------------------------*/
/* Returns the picture outline in window coords.                          */
/*------------------------------------------------------------------------*/
void MetaPictOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi)
{
   pMetaimg pMeta = (pMetaimg)pObj;

   rcl->xLeft   = (LONG)(pMeta->rclf.xLeft * pwi->usFormWidth );
   rcl->yBottom = (LONG)(pMeta->rclf.yBottom * pwi->usFormHeight);
   rcl->xRight  = (LONG)(pMeta->rclf.xRight * pwi->usFormWidth );
   rcl->yTop    = (LONG)(pMeta->rclf.yTop * pwi->usFormHeight);
}
/*------------------------------------------------------------------------*/
/* Stretches the metafilepicture into the new given rectl.                */
/*------------------------------------------------------------------------*/
void MetaStretch(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,ULONG ulMsg)
{
   pMetaimg pMeta = (pMetaimg)pObj;
   POINTL ptl;

   switch(ulMsg)
   {
      case WM_BUTTON1UP:
         pMeta->rclf.xLeft   =  (float) rcl->xLeft;
         pMeta->rclf.xLeft   /= (float) pwi->usFormWidth;

         pMeta->rclf.yBottom =  (float) rcl->yBottom;
         pMeta->rclf.yBottom /= (float) pwi->usFormHeight;

         pMeta->rclf.xRight  =  (float) rcl->xRight;
         pMeta->rclf.xRight  /= (float) pwi->usFormWidth;

         pMeta->rclf.yTop    =  (float) rcl->yTop;
         pMeta->rclf.yTop   /=  (float) pwi->usFormHeight;
         break;
      case WM_MOUSEMOVE:
         GpiSetLineType(pwi->hps, LINETYPE_DOT);
         ptl.x = rcl->xLeft;
         ptl.y = rcl->yBottom;
         GpiMove(pwi->hps, &ptl);
         ptl.x = rcl->xRight;
         ptl.y = rcl->yTop;
         GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0L,0L);
         break;
      default:
         break;
   }
   return;
}
/*------------------------------------------------------------------------*/
/* Name : MetaPictInvArea.                                                */
/*                                                                        */
/* Returns : The Invalid area in window coords of the selected picture.   */
/*------------------------------------------------------------------------*/
void MetaPictInvArea(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc)
{
   if (!pObj)
      return;

   MetaPictOutLine(pObj,rcl,pwi);
   /*
   ** Add extra space for selection handles.
   */
   if (bInc)
   {
      rcl->xLeft    -= 10;
      rcl->xRight   += 10;
      rcl->yBottom  -= 10;
      rcl->yTop     += 10;
   }
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);
}
/*------------------------------------------------------------------------*/
VOID * MetaPictSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj)
{
   pMetaimg pMeta = (pMetaimg)pObj;
   RECTL rcl;

   if (pMeta->uslayer == pwi->uslayer || pwi->bSelAll)
   {
      MetaPictOutLine(pObj,&rcl,pwi);
      if (ptl.x > rcl.xLeft && ptl.x < rcl.xRight)
         if (ptl.y > rcl.yBottom && ptl.y < rcl.yTop)
            return (void *)pMeta;
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/* MetaMoveOutline.                                                       */
/*                                                                        */
/* Description : Shows the outline during a move action. Called during    */
/*               the WM_MOUSEMOVE message.                                */
/*------------------------------------------------------------------------*/
void MetaMoveOutline(POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   RECTL  rcl;
   POINTL ptl;
   /*
   ** Show a movingbox
   */
   MetaPictOutLine(pObj,&rcl,pwi);
   ptl.x = rcl.xLeft    + (LONG)dx;
   ptl.y = rcl.yBottom  + (LONG)dy;
   GpiMove(pwi->hps,&ptl);
   ptl.x = rcl.xRight   + (LONG)dx;
   ptl.y = rcl.yTop     + (LONG)dy;
   GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0,0);
}
/*------------------------------------------------------------------------*/
void MetaPictMove(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy,fcx,fcy;
   pMetaimg pMeta = (pMetaimg)pObj;
   if (!pMeta)
      return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;

   pMeta->rclf.xLeft   += (fdx /fcx);
   pMeta->rclf.yBottom += (fdy /fcy);
   pMeta->rclf.xRight  += (fdx /fcx);
   pMeta->rclf.yTop    += (fdy /fcy);
}
/*---------------------------------------------------------------------------*/
void setMetaPosition(WINDOWINFO *pwi, POBJECT pObj, LONG x, LONG y)
{
   LONG lMetacx,lMetacy;
   pMetaimg pMeta = (pMetaimg)pObj;

   if (!pObj)
      return;
   lMetacx = (LONG)(pMeta->rclf.xRight * pwi->usFormWidth) - 
                   (pMeta->rclf.xLeft  * pwi->usFormWidth);

   lMetacy = (LONG)(pMeta->rclf.yTop   * pwi->usFormHeight) - 
                   (pMeta->rclf.yBottom* pwi->usFormHeight);

   pMeta->rclf.xLeft   = (float) x;
   pMeta->rclf.xLeft   /=(float) pwi->usFormWidth;
   pMeta->rclf.yBottom = (float) y;
   pMeta->rclf.yBottom /=(float) pwi->usFormHeight;
   pMeta->rclf.xRight  = pMeta->rclf.xLeft +
                            (lMetacx / (float) pwi->usFormWidth );
   pMeta->rclf.yTop    = pMeta->rclf.yBottom +
                            (lMetacy / (float) pwi->usFormHeight);
}
/*------------------------------------------------------------------------*/
/* function : ScaleMetafile.                                              */
/*                                                                        */
/* Description: Plays the specified MetaFile to the PS using the default  */
/* view matrix to scale and translate the output to fit a specified       */
/* rectangle in the PS page, while preserving the aspectratio of the      */
/* of the picture.                                                        */
/*                                                                        */
/* inputs : hps          PS handle                                        */
/*          hmf          MetaFile handle                                  */
/*          prclTarget   Target rectangle in page coordinates.            */
/*                                                                        */
/* returns: TRUE or FALSE.                                                */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ScaleMetaFile(HPS hps, HMF hmf, PRECTL prclTarget)
{

   BOOL     fret;
   LONG     lSegCount;
   UCHAR    abDesc[256];
   LONG     alOpns[PMF_DEFAULTS+1];
   POINTL   ptlPosn;
   RECTL    rclBoundary;
   MATRIXLF matlfXform;
   FIXED    afxScale[2];

   fret = GpiSetDrawControl(hps,            /* Set display control off    */
                            DCTL_DISPLAY,
                            DCTL_OFF);



   if (fret)
      fret = GpiSetDrawControl(hps,         /* Set bounds accumulation on */
                               DCTL_BOUNDARY,
                               DCTL_ON);

   if (fret)
      fret = GpiResetBoundaryData(hps);

   alOpns[PMF_SEGBASE]         = 0L;
   alOpns[PMF_LOADTYPE]        = LT_NOMODIFY;
   alOpns[PMF_RESOLVE]         = 0L;
   alOpns[PMF_LCIDS]           = LC_LOADDISC;
   alOpns[PMF_RESET]           = RES_NORESET;
   alOpns[PMF_SUPPRESS]        = SUP_NOSUPPRESS;
   alOpns[PMF_COLORTABLES]     = CTAB_REPLACE;
   alOpns[PMF_COLORREALIZABLE] = CREA_NOREALIZE;
   alOpns[PMF_DEFAULTS]        = DDEF_LOADDISC;


   fret = (BOOL)GpiPlayMetaFile(hps,          /* accumulate boundary data */
                                hmf,
                                PMF_DEFAULTS+1,   /* Options count         */
                                alOpns,          /* options               */
                                &lSegCount,      /* ret segcount reserved */
                                256L,            /* size of descriptive []*/
                                abDesc);         /* returned  " array     */

   fret = GpiQueryBoundaryData(hps,
                               &rclBoundary);

   /* Determine scale parameters to scale from boundary dimensions to targe */
   /* dimensions about bottom left of boundary. Ensure that both scale      */
   /* factors are equal and set to the smaller of the two possible values   */
   /* to preserve the aspect ratio of the picture.                          */

   afxScale[0] = (prclTarget->xRight - prclTarget->xLeft) * 0x10000
                 / (rclBoundary.xRight - rclBoundary.xLeft);

   afxScale[1] = (prclTarget->yTop - prclTarget->yBottom) * 0x10000
                 / (rclBoundary.yTop - rclBoundary.yBottom);

   if (afxScale[0] < afxScale[1])
      afxScale[1] = afxScale[0];
   else
      afxScale[0] = afxScale[1];

   ptlPosn.x = rclBoundary.xLeft;
   ptlPosn.y = rclBoundary.yBottom;

   fret = GpiScale(hps,               /* PS handle             */
                   &matlfXform,       /* transform matrix      */
                   TRANSFORM_REPLACE, /* options.              */
                   afxScale,          /* x and y scale factors */
                   &ptlPosn);

   ptlPosn.x = prclTarget->xLeft - rclBoundary.xLeft;
   ptlPosn.y = prclTarget->yBottom - rclBoundary.yBottom;

   fret = GpiTranslate(hps,            /* PS handle               */
                       &matlfXform,    /* transform matrix        */
                       TRANSFORM_ADD,  /* options                 */
                       &ptlPosn);      /* translations            */

   fret = GpiSetDefaultViewMatrix(hps,
                                  9L,
                                  &matlfXform,
                                  TRANSFORM_REPLACE);

   fret = GpiSetDrawControl(hps,     /* set bounds accumulation off */
                            DCTL_BOUNDARY,
                            DCTL_OFF);

   fret = GpiSetDrawControl(hps,         /* Set display control on */
                            DCTL_DISPLAY,
                            DCTL_ON);

   fret = (BOOL)GpiPlayMetaFile( hps,
                                 hmf,
                                 PMF_DEFAULTS+1,
                                 alOpns,
                                 &lSegCount,
                                 256L,
                                 abDesc);

   afxScale[0] = 0x10000;
   afxScale[1] = 0x10000;

   fret = GpiScale(hps,               /* PS handle             */
                   &matlfXform,       /* transform matrix      */
                   TRANSFORM_REPLACE, /* options.              */
                   afxScale,          /* x and y scale factors */
                   &ptlPosn);

   return(fret);
}
/*-----------------------------------------------------------------------*/
HMF GetMetaFile(pMetaimg pMeta, char *filename,WINDOWINFO *pwi)
{
   static Loadinfo  li;
   static CHAR szFllPath[CCHMAXPATH] = "";
   BOOL   bNew = FALSE;

   li.dlgflags = FDS_OPEN_DIALOG;

   szFllPath[0] = 0;

   strcpy(li.szExtension,".MET");        /* Set default extension */

   if (!filename)
   {
      bNew = TRUE;

#ifdef DEMOVERSION
      if (!FileGet(&li,pwi))
#else
      if (!FileGetName(&li,pwi))
#endif
         return (HMF)0;
   }
   else
   {
       if (filename[1] != ':')
       {
          getcwd(szFllPath,CCHMAXPATH);
          strcat(szFllPath,"\\");
       }
       strcat(szFllPath,filename);

       if (!strchr(szFllPath,'.'))
          strcat(szFllPath,".MET");
       strcpy(li.szFileName,szFllPath);
   }

   if (bNew)
      strcpy(pMeta->szMetaFileName,li.szFileName);

   return GpiLoadMetaFile(hab,li.szFileName);
}
/*------------------------------------------------------------------------*/
/* Name        : saveMetaPicture.                                         */
/*                                                                        */
/* Description : saves the selected metafile objects to disk. Called from */
/*               the metafile popupmenu. Very usefull when you wanna do   */
/*               conversion of windows metafile to the OS/2 metafile.     */
/*                                                                        */
/* Returns     : NONE.                                                    */
/*------------------------------------------------------------------------*/
void saveMetaPicture(POBJECT pObj,WINDOWINFO *pwi)
{
   static Loadinfo  li;

   if (!pObj)
      return;

   if (pObj->usClass != CLS_META)
      return;

   li.dlgflags = FDS_SAVEAS_DIALOG;
   strcpy(li.szExtension,".MET");        /* Set default extension */
   if (!FileGetName(&li,pwi))
      return;

   if ( (li.handle = open(li.szFileName, 
                          O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, 
                          S_IREAD|S_IWRITE)) == -1 )
      return;

   FilePutMetaData(&li,pObj);
   close(li.handle);
   li.handle = 0;
   return;
}
/*------------------------------------------------------------------------*/
/*  RecordMetaFile                                                        */
/*                                                                        */
/*  Description : Draws all objects into a metafile PS and saves the      */
/*                metafile to disk.  Dependant on the mode, all or only   */
/*                the selected objects are recorded.                      */
/*                                                                        */
/*  Parameters : HAB - program anchor blockhandle.                        */
/*               WINDOWINFO *pwi.                                         */
/*               RECTL * : NULL for all objects, else only selected.      */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
BOOL RecordMetaFile(HAB hab,WINDOWINFO *pwi,RECTL *prcl)
{
   HDC 		  hdc;		/* DC handle  			*/
   HPS            hps;          /* Presentation space handle   	*/
   HMF 		  hmf;		/* Metafile handle		*/
   static         Loadinfo  li;
   DEVOPENSTRUC   dopData;     	/* DEVOPENSTRUC structure   	*/
   SIZEL          sizl;         /* max client area size         */

   static MATRIXLF matlf = {0x20000,0L,0L,0L,0x20000,0L,20L,20L,1L};

   /*
   ** Try to get a filename...
   */
   li.dlgflags = FDS_SAVEAS_DIALOG;
   li.szFileName[0]=0;
   strcpy(li.szExtension,".MET");

#ifdef DEMOVERSION
   if (!FileGet(&li,pwi))
#else
   if (!FileGetName(&li,pwi))
#endif
      return FALSE;

   sizl.cx = pwi->usWidth;
   sizl.cy = pwi->usHeight;

   dopData.pszLogAddress	= NULL;
   dopData.pszDriverName	= (PSZ)"DISPLAY";
   dopData.pdriv              	= NULL;
   dopData.pszDataType        	= NULL;
   dopData.pszComment         	= NULL;
   dopData.pszQueueProcName   	= NULL;
   dopData.pszQueueProcParams 	= NULL;
   dopData.pszSpoolerParams   	= NULL;
   dopData.pszNetworkParams   	= NULL;

   hdc = DevOpenDC(hab,
                   OD_METAFILE,
                   "*",
                   9L,
                   (PDEVOPENDATA)&dopData,
                   (HDC)NULL);

   hps = GpiCreatePS(hab,hdc,
	             &sizl,
		     PU_LOMETRIC  |
		     GPIF_DEFAULT |
		     GPIT_NORMAL  |
		     GPIA_ASSOC);

   MetaInfo.cyClient     = 0;
   MetaInfo.uYfactor     = pwi->uYfactor;
   MetaInfo.uXfactor     = pwi->uXfactor;
   MetaInfo.hps          = hps;
   MetaInfo.bSuppress    = FALSE;
   MetaInfo.lcid         = getFontSetID(hps);
   MetaInfo.usdrawlayer  = pwi->uslayer;
   MetaInfo.uslayer      = pwi->uslayer;
   MetaInfo.lBackClr     = pwi->lBackClr;
   MetaInfo.usFormHeight = pwi->usHeight;
   MetaInfo.usFormWidth  = pwi->usWidth;
   MetaInfo.usHeight     = pwi->usHeight;
   MetaInfo.usWidth      = pwi->usWidth;
   MetaInfo.ulUnits      = PU_LOMETRIC;
   /*
   ** And since we can put also bitmaps in our metafile we
   ** should take with us the memory PS.
   */
   MetaInfo.hpsMem = pwi->hpsMem;

   GpiCreateLogColorTable ( hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );

   if (!prcl)
      for ( MetaInfo.usdrawlayer = MINLAYER; MetaInfo.usdrawlayer <= MAXLAYER; MetaInfo.usdrawlayer++)
      {
         ObjDrawSegment(hps,&MetaInfo,(POBJECT)0,NULL);
      }
   else
   {
      for ( MetaInfo.usdrawlayer = MINLAYER; MetaInfo.usdrawlayer <= MAXLAYER; MetaInfo.usdrawlayer++)
      {
         ObjDrawMetaSelected((POBJECT)0,hps,&MetaInfo);
      }
   }
   GpiAssociate(hps,(HDC)NULL);

   hmf = DevCloseDC(hdc);

   /*
   ** Try to remove the old file.
   */
   remove(li.szFileName);

   GpiSaveMetaFile(hmf,li.szFileName);
   GpiDestroyPS(hps);

   return TRUE;
}

POBJECT newMetaObject(void)
{
   POBJECT pObj;
   pObj = pObjNew(NULL,CLS_META);

   pObj->paint                 = DrawMetaSegment;
   pObj->moveOutline           = MetaMoveOutline;
   pObj->getInvalidationArea   = MetaPictInvArea;

   return pObj;
}