/*------------------------------------------------------------------------*/
/*  Name: drwimg2.c                                                       */
/*                                                                        */
/*  Description : Contains general helper functions for image processing. */
/*                                                                        */
/*  Functions:                                                            */
/*    Bmp2Filedata    :    Converts HBITMAP to real saveable data....     */
/*    PasteData       :    Paste data from the clipboard.                 */
/*    DataOnClipBoard :    Tells the initmenu if there is data on the     */
/*                         clipboard which we can accept.                 */
/*    CopyData        :    Copy data on the clipboard.                    */
/*    DragOver        :    Show user if we can accept a dropping.         */
/*    Drop            :    Drop function called from a drop event DM_DROP */
/*    CopyPalette     :    Copies palette of sel img to internal clipbrd. */
/*    PastePalette    :    Paste the palette of int clipbrd into sel img. */
/* Private functions:                                                     */
/*    CopyBitmap      :    Copies a bitmap on the clipboard.              */
/*    CopyMetaFile    :    Copies a metafile on the clipboard             */
/*    PasteBitmap     :    Paste a bitmap from the clipboard.             */
/*    getDropAction   :    Checks the extension (see function dragover)   */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_WINERRORS
#define INCL_SHLERRORS
#define INCL_GPI
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>

#define  DRWIMG   1
#include "standard.h"
#include "gbm.h"
#include "gbmerr.h"
#include "gbmht.h"
#include "gbmhist.h"
#include "gbmmcut.h"
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_img.h"
#include "drwutl.h"
#include "drwmeta.h"
#include "dlg_fnt.h"
#include "drwtxtin.h"
#include "dlg_file.h"

#define CLR_DRAG_DATATYPE  "DRT_DRWRGB"
#define CLR_RENDERFORMAT   "<DRM_DRWPALETTE,DRF_DRWRGB>"
#define FNT_DRAG_DATATYPE  "DRT_DRWFONT"
#define FNT_RENDERFORMAT   "<DRM_FNTPALETTE_CELL,DRF_DRWFONT>"
#define DO_COLORDROP       DO_UNKNOWN + 150
#define DO_FONTDROP        DO_UNKNOWN + 155
#define DO_IMAGEDROP       DO_UNKNOWN + 160
#define DO_METAFILEDROP    DO_UNKNOWN + 165
#define DO_DRAWITFILEDROP  DO_UNKNOWN + 170
#define DO_WINDOWSWMFDROP  DO_UNKNOWN + 175

static char *suppfiles[] = {"BMP","TIF","GIF","PCX","JPG",NULL};

static   TID         ThreadID;       /* New thread ID (returned)                   */
static   ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
static   ULONG       StackSize;      /* Size in bytes of new thread's stack        */

  struct {
    PSZ   address;
    PSZ   driver_name;
    PSZ   driver_data;
    PSZ   data_type;
    PSZ   comment;
    PSZ   proc_name;
    PSZ   proc_params;
    PSZ   spl_params;
    PSZ   network_params;
   } dcdatablk = {0L, "DISPLAY", 0L, 0L, 0L, 0L, 0L, 0L, 0L};
/*-----------------------------------------------------[ private ]-----------*/
/*                                                                           */
/* Function        : Bmp2Filedata                                            */
/*                                                                           */
/* Description     : Gets the data out of a bitmap handle.                   */
/*                   Returns the datasize in ulImageSize.                    */
/*                   Imagedata should be allocated seperate from the header. */
/*                   The object deletes the image date seperate from the     */
/*                   header info.                                            */
/*                                                                           */
/*                                                                           */
/* Return values   : PBYTE - pointer to the imagedata.                       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
static BOOL Bmp2Filedata(HAB hab, HBITMAP hbm,ULONG *pImgData,ULONG *pbmp2 )
{
   BITMAPINFOHEADER bmpTemp; /* to get the size before creating the PS */
   PBITMAPINFO2 pbmi;   /* is infoheader2 + pointer to rgb table    */
   BITMAPINFOHEADER2  bmp2;
   HDC    hdcMem;
   HPS    hpsMem;
   SIZEL  sizl;
   PBYTE  pbScan,pbData; //,pColor;
   ULONG  lBmpDataSize;
   USHORT sScanLineSize;
   USHORT nColors;
   ULONG  sScans;
   BOOL   bRetValue = FALSE; /* Initial this function is not successfull */
   /*
   ** Init to zero for cleanup checks at the end.
   */
   hdcMem = (HDC)0;
   hpsMem = (HPS)0;
   pbData = (PBYTE)0;         /* just avoiding warnings */
   pbmi   = (PBITMAPINFO2)0;
   /*
   ** Since the cleanup of all resources should be
   ** done very carefully on every error, I implemented
   ** a while loop were we break out on error, avoiding
   ** lots of duplicate code.
   */

   memset(&bmp2,0,sizeof(BITMAPINFOHEADER2));

   bmp2.cbFix = sizeof(BITMAPINFOHEADER2);

   do {

      if (bRetValue) /* something wrong?*/
         break;

      GpiQueryBitmapParameters(hbm,&bmpTemp);

      hdcMem = DevOpenDC( hab,OD_MEMORY,(PSZ)"*",8L,
                 (PDEVOPENDATA)&dcdatablk,NULLHANDLE);

   if (hdcMem == DEV_ERROR)
   {
      hdcMem = 0;
      break;
   }

   sizl.cx = bmpTemp.cx;
   sizl.cy = bmpTemp.cy;

   hpsMem = GpiCreatePS (hab, hdcMem, &sizl,PU_PELS | GPIA_ASSOC) ;

   if (!hpsMem)  break;

   /*
   ** 1 - Fill in the bitmapfileheader.
   ** 2 - Query the bitmap info header using the given bitmap handle.
   ** 3 - Calculate the imagedatasize (scanlinesize * cy).
   ** 4 - Allocate memory for header plus palette.
   ** 5 - Allocate memory for the image data.
   ** 6 - Get the bitmap bits cq imagedata.
   ** 7 - check for palette.
   ** 8 - return img data via pbyte pointer and the size via
   */

   if (!GpiQueryBitmapInfoHeader(hbm, &bmp2))
      break;

   sScanLineSize = ((bmp2.cBitCount * bmp2.cx + 31) / 32) * 4 *
                     bmp2.cPlanes;

   lBmpDataSize  = (LONG) sScanLineSize * bmp2.cy ;

   nColors = 1 << (bmp2.cBitCount * bmp2.cPlanes);

   GpiSetBitmap (hpsMem, hbm);

   /*
   ** Allocate memory for image data.
   */
   pbData = (PBYTE)calloc(lBmpDataSize,sizeof(char));

   if (!pbData) break;

   pbScan = pbData;

   pbmi = (PBITMAPINFO2)calloc((ULONG)(sizeof(BITMAPINFOHEADER2) + (nColors * sizeof(RGB2))),1);

   if (!pbmi)
      break;

   memcpy((void *)pbmi,&bmp2,sizeof(BITMAPINFOHEADER2));

   pbmi->cbFix   = sizeof(BITMAPINFOHEADER2);
   pbmi->cbImage = lBmpDataSize;

   sScans = GpiQueryBitmapBits (hpsMem,0L,bmp2.cy,pbScan,pbmi);

   if(!sScans)
      break;

   bRetValue = TRUE; /* Yes we made it.... */
   break;

   } while (FALSE);

   /*
   ** Cleanup...
   */
   if (hpsMem)
   {
      GpiSetBitmap(hpsMem,(HBITMAP)0L);
      GpiAssociate(hpsMem,(HDC)0L);
      GpiDestroyPS (hpsMem);
   }

   if (hdcMem )
      DevCloseDC (hdcMem);

   if (!bRetValue)
   {
      free(pbData);
      free(pbmi);
      pbData = NULL;
      pbmi   = NULL;
   }

   *pbmp2     = (ULONG )pbmi;
   *pImgData  = (ULONG )pbData;
   return bRetValue;
}
/*---------------------------------------------------[ private ]----------*/
/*  Name: PasteMetaFile.                                                  */
/*                                                                        */
/*  Description : Pasts a metafile from the clipboard.                    */
/*                                                                        */
/*  Parameters : HAB - anchorblockhandle of the calling thread.           */
/*               WINDOWINFO - our info struct.                            */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
static VOID PasteMetaFile(HAB hab, WINDOWINFO *pwi)
{
   HMF hmfNew,hmf;
   POINTL ptl;
   pMetaimg pMeta;
   RECTL rcl;
   ptl.x = 0;
   ptl.y = 0;

   WinOpenClipbrd(hab);
   hmf   = (HMF)WinQueryClipbrdData(hab, CF_METAFILE);

   if(hmf != (HMF)0)
   {
      hmfNew = GpiCopyMetaFile(hmf);
      pMeta = (pMetaimg)OpenMetaSegment(ptl,(char *)0,pwi,hmfNew);
      if (pMeta)
      {
         MetaPictInvArea((POBJECT)pMeta,&rcl,pwi,TRUE);
         WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
      }
   }
   WinCloseClipbrd(hab);
}
/*---------------------------------------------------[ private ]----------*/
/*  Name: PasteBitmap.                                                    */
/*                                                                        */
/*  Description : For now this routine can only  paste images from the    */
/*                clipboard.                                              */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
static VOID PasteBitmap(HAB hab, WINDOWINFO *pwi)
{
   HBITMAP    hbm;
   PRGB2      pRGB2;
   pDrwImage  pImg;  /* The real drawit image struct  which is added to the chain*/
   PBYTE      pData;
   POBJECT    pObj;
   SHORT      nColors;
   ULONG      sScanLineSize;


   ULONG      lhead,ldata;
   RECTL      rcl;

   WinOpenClipbrd(hab);
   hbm   = (HBITMAP)WinQueryClipbrdData(hab, CF_BITMAP);
   pData = (PBYTE)0;

   if(hbm != (HBITMAP)0L)
   {
       if (!Bmp2Filedata(hab,hbm,&ldata,&lhead))
       {
          WinCloseClipbrd(hab);
          return;
       }
       WinCloseClipbrd(hab);

       pImg = (pImage)pObjCreate(pwi,CLS_IMG);
       pObj = (POBJECT)pImg;
       pObj->moveOutline = ImgMoveOutLine;
       pObj->paint       = DrawImgSegment;
       pObj->getInvalidationArea    = ImgInvArea;


       pImg->pbmp2    = (PBITMAPINFOHEADER2)lhead;
       pImg->ImgData  = (PBYTE)ldata;

       nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

       if (nColors <= 256)
       {
          pData  = (PBYTE)pImg->pbmp2;
          pData  += sizeof(BITMAPINFOHEADER2);
          pRGB2  = (RGB2 *)pData;
          pObj->pDrwPal = (drw_palette *)calloc(sizeof(drw_palette),1);
          pObj->pDrwPal->nColors = nColors;
          pObj->pDrwPal->prgb2   = (RGB2 *)calloc(nColors,sizeof(RGB2));
          memcpy(pObj->pDrwPal->prgb2,pRGB2,(nColors*sizeof(RGB2)));
       }
       sScanLineSize = ((pImg->pbmp2->cBitCount * pImg->pbmp2->cx + 31) / 32) * 4 *
                         pImg->pbmp2->cPlanes;

       pImg->ImageDataSize  = (LONG) sScanLineSize * pImg->pbmp2->cy;

       SetupImgSegment(pImg,pwi);

       ImgInvArea((POBJECT)pImg,&rcl,pwi,TRUE);

       WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   }
   else
      WinCloseClipbrd(hab);

}   /* end of PasteRoutine  */
/*------------------------------------------------------------------------*/
/*  Name: DataOnClipBoard                                                 */
/*                                                                        */
/*  Description : Looks if there is data on the clipboard which we can    */
/*                accept. Called during the WM_INITMENU.                  */
/*                                                                        */
/*  Returns     : TRUE on succes.                                         */
/*------------------------------------------------------------------------*/
BOOL DataOnClipBoard(HAB hab)
{
   ULONG   ulFmtInfo;    /* Memory model and usage flags     */
   /*
   ** Look if there is a bitmap or metafile....
   */
   if (!WinQueryClipbrdFmtInfo(hab,CF_BITMAP,&ulFmtInfo) &&
       !WinQueryClipbrdFmtInfo(hab,CF_METAFILE,&ulFmtInfo))
      return FALSE;

   return TRUE;
}
/*-------------------------------------------------[ private ]------------*/
/*                                                                        */
/*  Name       : CopyMetaFile                                             */
/*                                                                        */
/*  Description: copies a selected metafile image to the clipboard.       */
/*                                                                        */
/*                                                                        */
/*  API's      : WinOpenClipbrd                                           */
/*               WinSetClipbrdData                                        */
/*               WinCloseClipbrd                                          */
/*                                                                        */
/*  Parameters : HAB       hanchor block handle of the calling thread     */
/*               pMetaimg  pointer to the selected image.                 */
/*               WINDOWINFO * pointer to our windowinfo structure.        */
/*                                                                        */
/*  Return     : [none]                                                   */
/*------------------------------------------------------------------------*/
static VOID CopyMetaFile(HAB hab,pMetaimg pMeta, WINDOWINFO *pwi)
{
   HMF hmfNew;

   if(pMeta->hmf)
   {
      hmfNew = GpiCopyMetaFile(pMeta->hmf);
      WinOpenClipbrd(hab);
      WinEmptyClipbrd(hab);
      WinSetClipbrdData(hab,(ULONG)hmfNew, CF_METAFILE, CFI_HANDLE);
      WinCloseClipbrd(hab);
   }
   return;
}
/*-------------------------------------------------[ private ]------------*/
/*                                                                        */
/*  Name       : CopyBitmap                                               */
/*                                                                        */
/*  Description: copies a selected image to the clipboard.                */
/*                                                                        */
/*  Concepts   :                                                          */
/*                                                                        */
/*  API's      : WinOpenClipbrd                                           */
/*               WinSetClipbrdData                                        */
/*               WinCloseClipbrd                                          */
/*                                                                        */
/*  Parameters : HAB       hanchor block handle of the calling thread     */
/*               pDrwImage pointer to the selected image.                 */
/*               WINDOWINFO * pointer to our windowinfo structure.        */
/*                                                                        */
/*  Return     : [none]                                                   */
/*------------------------------------------------------------------------*/
static VOID CopyBitmap(HAB hab,pDrwImage pImg, WINDOWINFO *pwi)
{
   HBITMAP hbm;

   GpiErase(pwi->hpsMem);
   hbm = GpiCreateBitmap(pwi->hpsMem,     /* memory-presentation-space handle     */
                         pImg->pbmp2,     /* address of structure for format data */
                         CBM_INIT,        /*options      */
                         pImg->ImgData,   /* bitmap data */
                         (PBITMAPINFO2)pImg->pbmp2); /* address of structure for color and format */

   if(hbm != (HBITMAP)0L)
   {
      WinOpenClipbrd(hab);
      WinEmptyClipbrd(hab);
      WinSetClipbrdData(hab,(ULONG)hbm, CF_BITMAP, 0L);
      WinCloseClipbrd(hab);
   }
   GpiSetBitmap(pwi->hpsMem, (HBITMAP)0);
   GpiDeleteBitmap(hbm);
   return;
}  /*  end of CopyRoutine  */
/*------------------------------------------------------------------------*/
/*                                                                        */
/*  Name       : CutBitmap                                                */
/*                                                                        */
/*  Description: cuts the selected image to the clipboard.                */
/*                                                                        */
/*  Concepts   :                                                          */
/*                                                                        */
/*  API's      : WinOpenClipbrd                                           */
/*               WinSetClipbrdData                                        */
/*               WinCloseClipbrd                                          */
/*                                                                        */
/*  Parameters : HAB       hanchor block handle of the calling thread     */
/*               pDrwImage pointer to the selected image.                 */
/*               WINDOWINFO * pointer to our windowinfo structure.        */
/*                                                                        */
/*  Return     : [none]                                                   */
/*------------------------------------------------------------------------*/
VOID CutBitmap(HAB hab,pDrwImage pImg, WINDOWINFO *pwi)
{
   RECTL rcl;

   /*
   ** First of all, copy the bitmap to the clipboard
   */
   CopyBitmap(hab,pImg,pwi);
   /*
   ** Delete the image from the drawing chain.
   */
   ImgInvArea((POBJECT)pImg,&rcl,pwi,TRUE);
   ObjDelete((POBJECT)pImg);
   WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   return;
}
/*------------------------------------------------------------------------*/
/*  Name: PasteData.                                                      */
/*                                                                        */
/*  Description : Checks what the datafromat is on the clipboard and      */
/*                calls the appropriate routine to load the data.         */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void PasteData(HAB hab,WINDOWINFO *pwi)
{
   ULONG   ulFmtInfo;    /* Memory model and usage flags     */

   if (WinQueryClipbrdFmtInfo(hab,CF_BITMAP,&ulFmtInfo))
      PasteBitmap(hab,pwi);
   if (WinQueryClipbrdFmtInfo(hab,CF_METAFILE,&ulFmtInfo))
      PasteMetaFile(hab,pwi);
}
/*------------------------------------------------------------------------*/
/*  Name: CopyData.                                                       */
/*                                                                        */
/*  Description : Looks at the op_mode flag of the main mod and decides   */
/*                what to copy to the clipboard.                          */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void CopyData(HAB hab,WINDOWINFO *pwi,POBJECT pObj)
{

   switch (pObj->usClass)
   {
      case CLS_IMG:
         CopyBitmap(hab,(pDrwImage)pObj,pwi);
         break;
      case CLS_META:
         CopyMetaFile(hab,(pMetaimg)pObj,pwi);
         break;
      default:
         DosBeep(880,50);
         break;
   }
}
/*------------------------------------------------------[ private ]-------*/
/*  Name: getDropAction                                                   */
/*                                                                        */
/*  Description : Functions checks the extension in the filename to see   */
/*                if we can load it. Only used in dragover to show user   */
/*                if we can accept a drop.                                */
/*                                                                        */
/*  Parameters  : char *szFilename - contains the filename.               */
/*                                                                        */
/*  Returns SHORT 0 on failure. DO_IMAGEDROP | DO_METAFILEDROP            */
/*------------------------------------------------------------------------*/
static SHORT getDropAction(char *szFilename)
{
   char sztmp[CCHMAXPATH];
   char *pDot;
   long i;

   char *szcopy = &sztmp[0];

   szcopy = strupr(szFilename);
//   printf("SuppBmps: szcopy[%s]\n",szcopy);
   pDot = strrchr(szcopy,'.');

   if (!pDot)
      return 0;

   pDot++; /* Jump over dot in filename */

   if (!strncmp("MET",pDot,3))
      return DO_METAFILEDROP;

   if (!strncmp("JSP",pDot,3))
      return DO_DRAWITFILEDROP;

   if (!strncmp("WMF",pDot,3))
      return DO_WINDOWSWMFDROP;

//   printf("SuppBmps: pDot[%s]\n",pDot);
   for (i=0; i < 5; i++)
      if (!strncmp(suppfiles[i],pDot,3))
         return DO_IMAGEDROP;
 

   return 0;
}
/*------------------------------------------------------------------------*
 *
 *  Name       : DragOver(pDraginfo, pCurrentDir)
 *
 *  Description: Provides visual feedback to the user as to whether it
 *               can support the drag operation
 *
 *  Concepts   :  direct manipulation
 *
 *  API's      :  DrgAccessDraginfo
 *                DrgFreeDraginfo
 *                DrgQueryDragitemPtr
 *                DrgQueryStrName
 *                DrgQueryDragitemCount
 *                DrgVerifyRMF
 *                WinGetLastError
 *
 *  Parameters :  pDraginfo  = pointer to drag information structure
 *                pCurrentDir= the name of the current directory
 *
 *  Return     :  DOR_DROP, DO_MOVE for a default operation
 *
 *--------------------------------------------------------------------------*/
MRESULT DragOver(HAB hab, PDRAGINFO pDraginfo, PSZ pCurrentDir)
{
   USHORT        cItems;
   ULONG         ulBytesWritten;
   USHORT        usDropAction;      /* DO_IMAGEDROP | DO_METAFILEDROP | 0 */
   PDRAGITEM     pditem;
   CHAR          SourceDir [CCHMAXPATH];
   char          szSourceFile[CCHMAXPATH];
  /*
   * Get access to the DRAGINFO structure.
   */
   if(!DrgAccessDraginfo(pDraginfo))
   {
      printf("Cannot access draginfo\n");
      return (MRFROM2SHORT (DOR_NODROPOP, 0));
   }
  /*
   * Determine if a drop can be accepted.
   */
   if  (pDraginfo->usOperation != DO_DEFAULT)
      return (MRFROM2SHORT (DOR_NODROPOP, 0));

   /*
    * Determine default operation if current operation is default.
    * The default is move unless either the source or the target is
    * on different removable media.
   */
   pditem = DrgQueryDragitemPtr (pDraginfo, 0);
   ulBytesWritten = DrgQueryStrName(pditem->hstrContainerName,
                                    sizeof(SourceDir),
                                    SourceDir);
   if(ulBytesWritten == 0L)
   {
      ulBytesWritten = DrgQueryStrName(pditem->hstrTargetName,
                                    sizeof(SourceDir),
                                    SourceDir);

//      if(ulBytesWritten)
//         printf("hstrTargetName %s\n",SourceDir);

      ulBytesWritten = DrgQueryStrName(pditem->hstrContainerName,
                                    sizeof(SourceDir),
                                    SourceDir);

//      if(ulBytesWritten)
//         printf("hstrContainerName %s\n",SourceDir);

      return (MRFROM2SHORT (DOR_NODROPOP, 0));
   }
   /*
   ** In order to support the operation, the source must support the
   ** operation this target has decided upon.  The source must also
   ** support a rendering mechanism of <DRM_OS2FILE,DRF_UNKNOWN>.
   ** This target doesn't care about the file type.
   */
   cItems = DrgQueryDragitemCount (pDraginfo);
   /*
   ** We can only accept one item at a time for the moment.
   */
   if (cItems > 1)
      return (MRFROM2SHORT (DOR_NODROPOP, 0));

   /*
   ** inspect item to see if it is acceptable
   */
   pditem = DrgQueryDragitemPtr (pDraginfo, 0);

   /*
   ** Is it the DrawIt colorpalette who is hovering above us...
   ** Give the DO_COLORDROP (DRAWIT CONTANT) so we can catch
   ** it later during the drop().
   */
   if (DrgVerifyTrueType(pditem, CLR_DRAG_DATATYPE))
      return MRFROM2SHORT(DOR_DROP,DO_COLORDROP);
   /*
   ** Check, maybe it is the drawit font palette.
   */
   if (DrgVerifyTrueType(pditem, FNT_DRAG_DATATYPE))
      return MRFROM2SHORT(DOR_DROP,DO_FONTDROP);

   /*
   ** Try to check on the containername source name...
   */
   DrgQueryStrName(pditem->hstrSourceName,sizeof(szSourceFile),
                   szSourceFile);

   printf("DragOver: szSourceFile [%s]\n",szSourceFile);

   usDropAction = getDropAction(szSourceFile);

//   return (MRFROM2SHORT (DOR_NODROPOP,0));

   if(!usDropAction && !DrgFreeDraginfo(pDraginfo))
      if(PMERR_SOURCE_SAME_AS_TARGET != WinGetLastError(hab))
      {
         return (MRFROM2SHORT (DOR_NODROPOP, 0));
      }
      return (MRFROM2SHORT(DOR_DROP,usDropAction));
}  /*  End of DragOver  */
/*------------------------------------------------------------------------*/
/*  Name: Drop                                                            */
/*                                                                        */
/*  Description : Called directly from a drop event. We only accept       */
/*                images for the moment. Color drops and fonts are handled*/
/*                in the windowproc itself.                               */
/*                of objects.                                             */
/*  Parameters  : HAB - Anchor block handle of the calling thread.        */
/*                PDRAGINFO - pointer to the drag info structure.         */
/*                WINDOWINFO *- Our windowinfo structure.                 */
/*                                                                        */
/*  Returns : Message result 0.                                           */
/*------------------------------------------------------------------------*/
MRESULT Drop(HAB hab,PDRAGINFO pDraginfo,WINDOWINFO *pwi)
{
   char  szSourceFile[CCHMAXPATH];
   char  szContain[CCHMAXPATH];
   ULONG ulBytesWritten;
   POBJECT pObj;
   RECTL   rcl;
   POINTL  ptlPos;
   char    *p;
   BOOL    bSelection;
   SHORT   xPos,yPos;
   PDRAGITEM pditem = NULL;
   /*
   ** Get access to the Draginfo structure.
   */

   bSelection   = pwi->bOnArea;
   pwi->bOnArea = TRUE;

   if(!DrgAccessDraginfo (pDraginfo))
   {
      printf("Drop DrgAccessDraginfo ERROR[%d]\n",WinGetLastError(hab));
      pwi->bOnArea = bSelection;
      return (MRESULT)0;
   }

   ptlPos.x = pDraginfo->xDrop;
   ptlPos.y = pDraginfo->yDrop;

   WinMapWindowPoints(HWND_DESKTOP,pwi->hwndClient,&ptlPos,1);
   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlPos);

   pditem = DrgQueryDragitemPtr (pDraginfo, 0);

   if (pDraginfo->usOperation == DO_COLORDROP)
   {
      if (pwi->op_mode == MULTISELECT)
      {
         ObjBoundingRect(pwi,&rcl,TRUE);
         ObjSetMltFillClr(pditem->ulItemID);
         pwi->op_mode   = NOSELECT;
         pwi->pvCurrent = NULL;
         ObjMultiUnSelect();
         WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
      }
      else if ((pObj = (POBJECT)ObjSelect(ptlPos,pwi))!=NULL)
      {
         if (ObjDropFillClr(pwi,pditem->ulItemID,pObj))
            ObjRefresh((POBJECT)pObj,pwi);
      }
      else
      {
         pwi->lBackClr = pditem->ulItemID;
         WinInvalidateRect(pwi->hwndClient,0,TRUE);
      }

      /*
      ** Tidy up Drag Object Memory
      */
      DrgDeleteDraginfoStrHandles(pDraginfo);
      DrgFreeDraginfo(pDraginfo);
      pwi->bOnArea = bSelection;
      return (MRESULT)0;
   }

   if (pDraginfo->usOperation == DO_FONTDROP)
   {

      /*
      ** Pick up the fontname
      */
      ulBytesWritten =DrgQueryStrName(pditem->hstrTargetName,
                                       sizeof(szContain),
                                       szContain);

      if (!ulBytesWritten)
         return (MRESULT)0;

      if (pwi->op_mode == MULTISELECT)
      {
         if (( p = strchr(szContain,'.')) != NULL )
         {
            ObjBoundingRect(pwi,&rcl,TRUE);

            p++; /* jump over dot */
            ObjMultiFontChange(p);

            ObjMultiUnSelect();
            WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
         }
         else
         {
            ObjMultiUnSelect();
            pwi->op_mode = NOSELECT;
            WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
         }
      }
      else if ((pObj = ObjSelect(ptlPos,pwi))!=NULL)
      {
         /*
         ** Font dropped on a single text object?
         */
         if ((pObj->usClass == CLS_TXT || pObj->usClass == CLS_BLOCKTEXT)
             && (p = strchr(szContain,'.')) )
         {
            p++; /* jump over dot */
            ObjFontChange(p,pObj);
            ObjInvArea(pObj,&rcl,pwi,TRUE);
            WinInflateRect((HAB)0,&rcl,50,50);
            WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
         }
      }
      else
      {
         /*
         ** Font is dropped on the drawing area.
         */
         if (( p = strchr(szContain,'.')) != NULL )
         {
            strcpy(pwi->fontname,szContain); /* For status line */
            *p = 0;                     /* overwrite dot   */
            pwi->fxPointsize = MAKEFIXED(atoi(szContain),0);
            p++;                        /* jump over zero  */
            pwi->sizfx.cx = pwi->fxPointsize;
            pwi->sizfx.cy = pwi->fxPointsize;
            strcpy(pwi->fattrs.szFacename,p);

            setFont(pwi,&pwi->fattrs,pwi->sizfx);

            if (pwi->op_mode == TEXTINPUT )
            {
               WinShowCursor(pwi->hwndClient,FALSE);
               WinDestroyCursor(pwi->hwndClient);
               Createcursor(pwi); /*drwtxtin.c*/
            }
            WinPostMsg(pwi->hwndMain, UM_FNTHASCHANGED,
                       (MPARAM)pwi->fontname,(MPARAM)0);
         }
      }
      /*
      ** Tidy up Drag Object Memory
      */
      DrgDeleteDraginfoStrHandles(pDraginfo);
      DrgFreeDraginfo(pDraginfo);
      return (MRESULT)0;
   }

   
   if ( pDraginfo->usOperation == DO_IMAGEDROP      ||
        pDraginfo->usOperation == DO_METAFILEDROP   || 
        pDraginfo->usOperation == DO_DRAWITFILEDROP ||
        pDraginfo->usOperation == DO_WINDOWSWMFDROP )
   {
      /*
      ** Lets get the filename!
      */
      ulBytesWritten =DrgQueryStrName(pditem->hstrSourceName,
                                      sizeof(szSourceFile),
                                      szSourceFile);
      if (!ulBytesWritten)
         return (MRESULT)0;

      ulBytesWritten =DrgQueryStrName(pditem->hstrContainerName,
                                      sizeof(szContain),
                                      szContain);
      if (!ulBytesWritten)
         return (MRESULT)0;

      if (szContain[strlen(szContain)-1]!='\\')
         sprintf(pwi->szFilename,"%s\\%s",szContain,szSourceFile);
      else
         sprintf(pwi->szFilename,"%s%s",szContain,szSourceFile);

   }

   if ( pDraginfo->usOperation == DO_IMAGEDROP )
   {
      pObj  = pObjNew(NULL, CLS_IMG);
      pObj->moveOutline = ImgMoveOutLine;
      pObj->paint       = DrawImgSegment;
      pObj->getInvalidationArea    = ImgInvArea;


      ImgSetDropPosition(pObj,pDraginfo->xDrop,pDraginfo->yDrop,pwi);

      if (pObj)
      {
         pwi->pvCurrent = pObj;
         ThreadFlags = 0;        /* Indicate that the thread is to */
                                 /* be started immediately         */
         StackSize = 45000;      /* Set the size for the new       */
                                 /* thread's stack                 */
         DosCreateThread(&ThreadID,(PFNTHREAD)CreateImgSegment,
                         (ULONG)pwi,ThreadFlags,StackSize);
       }
   }

   if ( pDraginfo->usOperation == DO_METAFILEDROP )
   {
      p = strdup(pwi->szFilename); /* receiver frees memory!! See drwmain.c */
      WinPostMsg(pwi->hwndClient,UM_LOADMETAFILE,(MPARAM)p,(MPARAM)0);
   }
   else if ( pDraginfo->usOperation == DO_WINDOWSWMFDROP )
   {
      p = strdup(pwi->szFilename); /* receiver frees memory!! See drwmain.c */
      xPos = (SHORT)ptlPos.x;
      yPos = (SHORT)ptlPos.y;
      WinPostMsg(pwi->hwndClient,UM_LOADWMFFILE,(MPARAM)p,MPFROM2SHORT(xPos,yPos));
   }
   else if ( pDraginfo->usOperation == DO_DRAWITFILEDROP )
      ReadDrwfile(pwi->szFilename,pwi);

   DrgDeleteDraginfoStrHandles(pDraginfo);
   DrgFreeDraginfo(pDraginfo);

   return (MRESULT)NULL;
}    /*  End of Drop  */
/*-------------------------------------------------------------------------*/
void copyPalette(WINDOWINFO *pwi)
{
   POBJECT        pObj;
   pDrwImage      pImg;  /* The real drawit image struct  which is added to the chain*/
   unsigned char *pData;
   PRGB2          pRGB2,p;
   int            nColors,i;

   pObj = pwi->pvCurrent;
  
   if (!pObj)
      return;

   if (pObj->usClass != CLS_IMG || !pObj->pDrwPal)
      return;

   pImg = (pDrwImage)pObj;
 
   pData = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(pData + sizeof(BITMAPINFOHEADER2));

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (pwi->colorPalette.prgb2)
      free(pwi->colorPalette.prgb2);
   pwi->colorPalette.prgb2   = NULL;
   pwi->colorPalette.nColors = 0;

   if (nColors  &&  nColors <= 256 )
   {
      pwi->colorPalette.nColors = nColors;
      pwi->colorPalette.prgb2   = (PRGB2)calloc(nColors,sizeof(RGB2));
      p = pwi->colorPalette.prgb2;
      for ( i = 0; i < nColors; i++ )
      {
         *p = *pRGB2;
         pRGB2++;
         p++;
      }
   }
}
/*-------------------------------------------------------------------------*/
void pastePalette(WINDOWINFO *pwi)
{
   POBJECT        pObj;
   pDrwImage      pImg;  /* The real drawit image struct  which is added to the chain*/
   unsigned char *pData;
   PRGB2          pRGB2,p;
   int            nColors,i;

   pObj = pwi->pvCurrent;
  
   if (!pObj)
      return;

   if (pObj->usClass != CLS_IMG || !pObj->pDrwPal || !pwi->colorPalette.prgb2)
      return;

   pImg = (pDrwImage)pObj;
 
   pData = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(pData + sizeof(BITMAPINFOHEADER2));

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (nColors <= 256 )
   {
      p = pwi->colorPalette.prgb2;
      for ( i = 0; i < nColors && i < pwi->colorPalette.nColors; i++ )
      {
         *pRGB2 = *p;
         pRGB2++;
         p++;
      }
   }
   ObjRefresh(pObj,pwi);

}
