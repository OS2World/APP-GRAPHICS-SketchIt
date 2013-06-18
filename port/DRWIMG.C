/*------------------------------------------------------------------------*/
/*  Name: DLG_IMG.C                                                       */
/*                                                                        */
/*  Description : Displaying of the images from the images chain.         */
/*                deletion,creation and maintanance of the bitmapImage.   */
/*                                                                        */
/* Functions:                                                             */
/*   imgGrayscale            : Converts color image into grayscale img.   */
/*   imgBlackWhite           : Converts color image into b&w image.       */
/*   OpenImgSegment          : Opens/creates an imagesegment              */
/*   CloseImgSegment         : Closes the segment.                        */
/*   DeleteImage             : Deletes image from the chain               */
/*   DeleteAllImages         : Deletes all images                         */
/*   ExportSel2Bmp           : Exports the selected objects to a BMP!     */
/*   Export2Bmp              : Exports whole drawing to a BMP.            */
/*   DrawImgSegment          : draws the segments into a PS.              */
/*   CreateBitmapHdcHps      : Creates HDC and HPS - OD_MEMORY            */
/*   CloseImgHdcHps          : Deletes HDC and HPS - OD_MEMORY            */
/*   ImageSelect             : Is there an image under my mousepointer?   */
/*   ImgMoveSegment          : Move the image with a given dx,dy.         */
/*   ImageCrop               : Crops the selected image                   */
/*   ImgShowPalette          : Shows the palette in a dialog after DBLCLK */
/*   ImageCircle             : Blits image in circular image              */
/*   ImageStretch            : Stretches the image to the given rectangle */
/*   FlipImgHorz             : Mirror imagedata over X-axis.              */
/*   FlipImgVert             : Mirror imagedata over Y-axis.              */
/*   FilePutImage            : Puts one Imagesegment in a file.           */
/*   FileGetImage            : Loads one imagesegment from a file         */
/*   ImgInvArea              : Calculates the RECTL of the selected img   */
/*   ImgRotateColor          : Rotates colors in the palette (REMAME!!!!) */
/*   ImgRestoreAspect        : Restores the aspect ratio.Called from menu */
/*   ImgRestoreSize          : Restores the original size of the image.   */
/*   ImgSaveOtherFormats     : Save selected image into other formats.    */
/*   SaveBitmap              : Save selected image as OS/2 2.x BMP file.  */
/*   ImgSetCircular          : Sets the circular boolean on true.         */
/*   ImgIsCircular           : Is the image circular?                     */
/*   copyImageObject         : Makes a copy of the given image object.    */
/*   setPalInObj             : Set pal in Obj after reading JSP.          */
/*                                                                        */
/*--ch---date----version-----description----------------------------------*/
/*  1   151198   2.9         Added clippath.                              */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_sqr.h"
#include "dlg_hlp.h"
#include "dlg_fnt.h"
#include "drwutl.h"
#include "drwsplin.h"
#include "resource.h"

#define  DRWIMG   1
#define  BMPMAX_CX   1024
#define  BMPMAX_CY    768
#define  BMPMIN_CX     24
#define  BMPMIN_CY     24
#define  MAX_COLORVAL  128      /* Black & White func */
#include "standard.h"

#include "gbm.h"

#include "gbmerr.h"
#include "gbmht.h"
#include "gbmhist.h"
#include "gbmmcut.h"
#include "gbmmir.h"
#include "gbmtrunc.h"
#include "dlg_img.h"


#define MINIMUMSIZE        20     /* After cropping minimum size in pixels */

#define  ROUNDTODWORD(b)   (((b+31)/32)*4)

HDC     hdcBitmapFile;
BOOL    DlgLoaded = FALSE;
static  Loadinfo li;
/*
** File extesion and format identifier.
*/
static  char  szExt[10];
static  char  szOpt[10];     /* used to distinquish windows bmp from os2 */
static USHORT usIdColor;     /* can be ID_RADTRUE,.._RAD256 etc.         */
static SIZEL  sizlBitmap;    /* Used for defining the bitmap size in pels*/

struct
{
    USHORT usIdColor;        /* Possible option              */
    BOOL   bAllowed;         /* Is this option allowed?      */
    BOOL   bEnabled;         /* Is the button enabled again? */
} colorOptions[]= { {ID_RADTRUE, FALSE, TRUE },
                    {ID_RAD256 , FALSE, TRUE },
                    {ID_RAD64  , FALSE, TRUE },
                    {ID_RAD16  , FALSE, TRUE },
                    {0         , FALSE, TRUE }};

#define OPTIONTAB    5

#define IMAGE        "@IMAGE@"     /* Start off image data in .JSP file        */
#define EOF_IMAGE    "@/IMAGE@"    /* End of image data in .JSP file.          */
#define CLIPPATH     "@CLIPPATH@"  /* Start off image data in .JSP file        */
#define EOF_CLIPPATH "@/CLIPPATH@" /* End of image data in .JSP file.          */
/*
** Structure needed for mutlithreaded saving of images
*/
typedef struct _imgsavestruct
{
   HPS     hpsMem;
   HDC     hdcMem;
   char    szFilename[CCHMAXPATH];
   char    szOpt[10];
   HBITMAP hbm;
   USHORT  usRadioBtn;
   WINDOWINFO  *pwi;
   drw_palette *pDrwPal; /* current palette */
}imgsavestruct, *pimgsavestruct;


typedef  struct _fhelper
   {
       USHORT   ustype;
       USHORT   nColors;
       USHORT   uslayer;
       ULONG    lPattern;      /* Filling pattern         */
       BOOL     ImgCir;
       float    cx;
       float    cy;
       float    cxdest;
       float    cydest;
       float    x;
       float    y;
       unsigned char * ImgData;
       ULONG ImageDataSize;
       ULONG filesize;
//       ULONG ulPoints;         /*@ch1 clippath!!! */
   } IMGFILEHEADER;

CHAR szFullFilePath[CCHMAXPATH] = "";       /* For the filedialog */

static BOOL drawImgInClippath(HBITMAP hbm,HPS hps,pImage pImg,WINDOWINFO *pwi);
static void drawPolyLine(HPS hps, pImage pImg, RECTL rcl);
/*-----------------------------------------------[ public ]---------------*/
/* copyImageObject.                                                       */
/*                                                                        */
/* description  : Makes a copy of the given image object. Used when the   */
/*                user copies the object with the Ctrl plus dragging opp. */
/*                see drwutl.c objectCopy.                                */
/*                                                                        */
/* returns      : A copy of the given object on success, otherwise NULL.  */
/*------------------------------------------------------------------------*/
POBJECT copyImageObject(POBJECT pObj)
{
   USHORT      usColors;
   pImage      pImg,pImgNew;
   ULONG       ulSize;
   POBJECT     pCopy;

   if (!pObj)
      return NULL;
   if (pObj->usClass != CLS_IMG)
      return NULL;

   pImg = (pImage)pObj;

   usColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );
   /*
   ** Calculate the sizeof the BITMAPINFOHEADER2 plus possible palette.
   */
   if (usColors <= 256 )
      ulSize = sizeof(BITMAPINFOHEADER2) + (usColors * sizeof(RGB2));
   else
      ulSize = sizeof(BITMAPINFOHEADER2);

   pCopy = pObjNew(NULL,CLS_IMG);

   if (!pCopy)
      return NULL;

   memcpy(pCopy,pObj,ObjGetSize(CLS_IMG));

   pCopy->moveOutline            = ImgMoveOutLine;
   pCopy->paint                  = DrawImgSegment;
   pCopy->getInvalidationArea    = ImgInvArea;

   pImgNew = (pImage)pCopy;

   pImgNew->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)ulSize,sizeof(char));

   if (!pImgNew->pbmp2)
   {
      free((void *)pCopy);
      return NULL;
   }
   /*
   ** Copy the bitmapinfo header to the destination...
   */
   memcpy(pImgNew->pbmp2,pImg->pbmp2,ulSize);
   /*
   ** Allocate memory for the pixel data.
   */
   pImgNew->ImgData = (char *)calloc((ULONG)pImg->pbmp2->cbImage,sizeof(char));

   if (!pImgNew->ImgData)
   {
      free((void *)pImgNew->pbmp2);
      free((void *)pCopy);
      return NULL;
   }
   memcpy(pImgNew->ImgData,pImg->ImgData,pImg->pbmp2->cbImage);
   /*
   ** Try to copy the original palette from the pObj structure.
   */
   if (pObj->pDrwPal && usColors <= 256 )
   {
      pCopy->pDrwPal = (drw_palette *)calloc(sizeof(drw_palette),1);
      pCopy->pDrwPal->nColors = pObj->pDrwPal->nColors;
      pCopy->pDrwPal->prgb2   = (RGB2 *)calloc(pObj->pDrwPal->nColors,sizeof(RGB2));
      if (!pCopy->pDrwPal->prgb2)
      {
        free((void *)pCopy->pDrwPal);
        pCopy->pDrwPal = NULL;
      }
      else
         memcpy(pCopy->pDrwPal->prgb2,pObj->pDrwPal->prgb2,
                (pObj->pDrwPal->nColors * sizeof(RGB2)));
   }
   else
      pCopy->pDrwPal = NULL;

   return pCopy;
}
/*------------------------------------------------------------------------*/
/* free_drw_palette.                                                      */
/*------------------------------------------------------------------------*/
static void free_drw_palette( drw_palette *pDrwPal)
{
    if (!pDrwPal)
       return;

    if (pDrwPal->prgb2)
       free(pDrwPal->prgb2);
    free(pDrwPal);
    return;
}
/*-----------------------------------------------[ private ]--------------*/
/* free_imgsavestruct                                                     */
/*------------------------------------------------------------------------*/
static void free_imgsavestruct ( imgsavestruct *pSave)
{
   GpiDeleteBitmap(pSave->hbm);
   GpiAssociate(pSave->hpsMem, (HDC)0L);
   GpiDestroyPS(pSave->hpsMem);
   DevCloseDC(pSave->hdcMem);
   free_drw_palette(pSave->pDrwPal);
   free(pSave);
}
/*-----------------------------------------------[ public  ]--------------*/
/* free_imgobject.                                                        */
/*------------------------------------------------------------------------*/
void free_imgobject ( POBJECT pObj)
{
   pImage pImg = (pImage)pObj;

   if (pObj->pDrwPal)
   {
      free_drw_palette( pObj->pDrwPal);
      pObj->pDrwPal = NULL;
   }
   if (pImg->ImgData)
      free(pImg->ImgData);
   if (pImg->pbmp2)
      free(pImg->pbmp2);
   return;
}
/*-----------------------------------------------[ private ]--------------*/
/* readMarker.                                                            */
/*                                                                        */
/* Description  : readImage header from the JSP file. <IMAGE>             */
/*                in a JSP file then we try here to find the end of the   */
/*                image data.                                             */
/*                                                                        */
/* Parameters   : int hFile - handle of the file.                         */
/*                                                                        */
/*------------------------------------------------------------------------*/
static BOOL readMarker(int hFile, char *pszMarker)
{
   char szMark[40];
   char cMark;
   int  i,iLen;
   /*
   ** Error checking goes here
   */
   cMark = 0;
   strcpy(szMark,pszMarker);
   iLen = strlen(szMark);

   for (i = 0; i < iLen; i++)
   {
      read( hFile,(PVOID)&cMark,1);
      if (cMark != szMark[i])
      {
         return FALSE;
      }
   }
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/* spool2ImgEnd                                                           */
/*                                                                        */
/* Description  : When an error occurs during the loading of an image in  */
/*                in a JSP file then we try here to find the end of the   */
/*                image data.                                             */
/*                                                                        */
/* Parameters   : int hFile - handle of the file.                         */
/*                                                                        */
/*------------------------------------------------------------------------*/
static BOOL spool2ImgEnd(int hFile)
{
   char szMark[40];
   char cMark;
   int  iLen,i,iRet;
   BOOL bFound;

   bFound = FALSE;
   cMark  = 0;
   strcpy(szMark,EOF_IMAGE);
   iLen = strlen(szMark);

   do
   {
      iRet = read( hFile, (PVOID)&cMark,1);

      if (iRet < 0)
         return FALSE;

      if (cMark == '<')
      {
         for (i = 0; i < iLen-1 && cMark == szMark[i]; i++)
         {
            iRet = read( hFile,(PVOID)&cMark,1);

            if (iRet < 0)
               return FALSE;
         }
         if (i == iLen - 1)
            bFound = TRUE;
      }
   }while (!bFound);

   return bFound;
}
/*------------------------------------------------------------------------*/
/* FilePutImage.                                                          */
/*------------------------------------------------------------------------*/
int FilePutImageData(pLoadinfo pli, pImage pImg )
{
   int         cbWritten;
   USHORT      usColors;
   PBYTE       data;
   PRGB2       pRGB2;
   ULONG       ulSize;
   PVOID       p;

   if (!pImg)
      return 0;

   write(pli->handle,(PVOID)IMAGE,strlen(IMAGE));

   usColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (usColors <= 256 )
   {
      data = (unsigned char *)pImg->pbmp2;
      pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));
   }
   else
      pRGB2 = (RGB2 *)0;
   /*
   ** Save the bitmap infoheader...
   */
   if (!pImg->pbmp2->cbImage && ( pImg->pbmp2->cx && pImg->pbmp2->cy))
      pImg->pbmp2->cbImage = ROUNDTODWORD(pImg->pbmp2->cx * pImg->pbmp2->cBitCount)*pImg->pbmp2->cy;

   cbWritten = write(pli->handle,(PVOID)pImg->pbmp2,(ULONG)sizeof(BITMAPINFOHEADER2));

   if (pRGB2) /* If there is a palette save it */
   {
      cbWritten = write(pli->handle,(PVOID)pRGB2,
                         (ULONG)(usColors * sizeof(RGB2)) );
   }
   cbWritten = write(pli->handle,(PVOID)pImg->ImgData,pImg->pbmp2->cbImage);
   /*
   ** Write file marker at end of image.
   */
   if (cbWritten > 0)
   {
      write(pli->handle,(PVOID)EOF_IMAGE,strlen(EOF_IMAGE));

      write(pli->handle,(PVOID)CLIPPATH,strlen(CLIPPATH));
      if (pImg->pptl && pImg->nrpts > 0)
      {
         ulSize = pImg->nrpts * sizeof(POINTLF);
         p = (PVOID)pImg->pptl;
         cbWritten = write(pli->handle,(PVOID)p,ulSize);
      }
      write(pli->handle,(PVOID)EOF_CLIPPATH,strlen(EOF_CLIPPATH));
   }
   return cbWritten;
}
/*-----------------------------------------------[ private ]--------------*/
/* setPalInObj.                                                           */
/*                                                                        */
/* Description  : setPalInObj, set the palette in the Object structure if */
/*                the image contains a palette. This function is only     */
/*                when the image is loaded from a JSP file.               */
/*------------------------------------------------------------------------*/
void setPalInObj(POBJECT pObj, USHORT usColors)
{
   RGB2          *pRGB2;
   unsigned char *data;
   pImage         pImg;

   pObj->pDrwPal = (drw_palette *)calloc(sizeof(drw_palette),1);
   pObj->pDrwPal->nColors = usColors;
   pObj->pDrwPal->prgb2   = (RGB2 *)calloc(usColors,sizeof(RGB2));
   if (!pObj->pDrwPal->prgb2)
   {
      free((void *)pObj->pDrwPal);
      pObj->pDrwPal = NULL;
      return;
   }
   pImg = (pImage)pObj;
   data = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));
   memcpy(pObj->pDrwPal->prgb2,pRGB2,(usColors * sizeof(RGB2)));
}
/*------------------------------------------------------------------------*/
/* FileGetImage.                                                          */
/*------------------------------------------------------------------------*/
int FileGetImageData(WINDOWINFO *pwi, pLoadinfo pli, pImage pImg)
{
   ULONG       ulDataSize;
   USHORT      usColors;
   PBYTE       data;
   int         cbRead;    /* Bytes read from file    */
   int         iSize;
   PRGB2       pRGB2;
   BITMAPINFOHEADER2 bmp2;
   char        cMark = 0;
   char        szMark[40];
   int         iLen,i;

   if (pwi->lFileFormat == FILE_DRAWIT28 ||
       pwi->lFileFormat == FILE_DRAWIT29 ||
       pwi->lFileFormat == FILE_DRAWIT30 ||
       pwi->lFileFormat == FILE_DRAWIT32  )
   {
      if (!readMarker(pli->handle,IMAGE))
      {
         pImg->pbmp2   = NULL;
         pImg->ImgData = NULL;
         spool2ImgEnd(pli->handle);
         return 0L;
      }
   }
   memset(&bmp2, 0,sizeof(BITMAPINFOHEADER2));
   /*
   ** Start loading the bitmapheader including the color lookup table
   ** if the number of colors <= 256, else we assume its a truecolor one
   */
   cbRead = read( pli->handle,(PVOID)&bmp2,sizeof(BITMAPINFOHEADER2));

   usColors = 1 << ( bmp2.cBitCount * bmp2.cPlanes );


   ulDataSize = sizeof(BITMAPINFOHEADER2) + (sizeof(RGB2) * usColors);

   pImg->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)ulDataSize,sizeof(char));

   memcpy((void *)pImg->pbmp2,(void *)&bmp2,sizeof(BITMAPINFOHEADER2));

   if (usColors <= 256) /* If there is a palette save it */
   {
       data = (unsigned char *)pImg->pbmp2;
       pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));
       cbRead = read( pli->handle,(PVOID)pRGB2, sizeof(RGB2) * usColors );
   }

   if (cbRead == -1)
   {
      if (pImg->pbmp2)
         free(pImg->pbmp2);
      pImg->pbmp2   = NULL;
      pImg->ImgData = NULL;
      return 0L;
   }

   if (!pImg->pbmp2->cx || !pImg->pbmp2->cy || !pImg->pbmp2->cbImage)
   {
      if (pImg->pbmp2)
         free(pImg->pbmp2);

      pImg->pbmp2   = NULL;
      pImg->ImgData = NULL;

      spool2ImgEnd(pli->handle);
      return 0L;
   }

   if (!pImg->pbmp2->cbImage && ( pImg->pbmp2->cx && pImg->pbmp2->cy))
      pImg->pbmp2->cbImage = ROUNDTODWORD(pImg->pbmp2->cx * pImg->pbmp2->cBitCount)*pImg->pbmp2->cy;

   pImg->ImgData = (char *)calloc((ULONG)pImg->pbmp2->cbImage,sizeof(char));

   cbRead = read( pli->handle, (PVOID)pImg->ImgData,pImg->pbmp2->cbImage);

   if (cbRead== -1)
   {
      if (pImg->ImgData)
         free(pImg->ImgData);

      if (pImg->pbmp2)
         free(pImg->pbmp2);

      pImg->pbmp2   = NULL;
      pImg->ImgData = NULL;

      return 0L;
   }
   /*
   ** Error checking goes here
   */
   if ( pwi->lFileFormat == FILE_DRAWIT28 ||
        pwi->lFileFormat == FILE_DRAWIT29 ||
        pwi->lFileFormat == FILE_DRAWIT30 ||
        pwi->lFileFormat == FILE_DRAWIT32 )
   {
      strcpy(szMark,EOF_IMAGE);
      iLen = strlen(szMark);

      for (i = 0; i < iLen; i++)
      {
         read( pli->handle, (PVOID)&cMark,1);
         if (cMark != szMark[i])
         {
            if (pImg->ImgData)
               free(pImg->ImgData);

            if (pImg->pbmp2)
               free(pImg->pbmp2);

            pImg->pbmp2   = NULL;
            pImg->ImgData = NULL;

            return 0L;
         }

      }
   } /* endif DRAWIT28 */

   if ( pwi->lFileFormat == FILE_DRAWIT29 ||
        pwi->lFileFormat == FILE_DRAWIT30 ||
        pwi->lFileFormat == FILE_DRAWIT32 )
   {
     readMarker(pli->handle,CLIPPATH);
     if (pImg->nrpts)
     {
        iSize      = pImg->nrpts * sizeof(POINTLF);
        pImg->pptl = (PPOINTLF)calloc((ULONG)iSize,sizeof(char));
        cbRead     = read(pli->handle,(PVOID)pImg->pptl,iSize);
     }
     else
        pImg->pptl = NULL;

     readMarker(pli->handle,EOF_CLIPPATH);
   }
   /*
   ** If there is a palette save it in the OBJECT structure so
   ** the user can restore the palette of the image even when it
   ** is loaded from the JSP file.
   */
   if (usColors <= 256)
      setPalInObj((POBJECT)pImg, usColors);
   return cbRead;
}

static int StrideOf(MOD *mod)
   {
   return ( ( mod->gbm.w * mod->gbm.bpp + 31 ) / 32 ) * 4;
   }
/*...e*/
/*...sAllocateData:0:*/

static BOOL AllocateData(MOD *mod)
   {
   int stride = StrideOf(mod);
   if ( (mod->pbData = calloc((stride * mod->gbm.h),1)) == NULL )
      return FALSE;
   return TRUE;
   }

/*...sModCreate:0:*/
MOD_ERR ModCreate(
   int w, int h, int bpp, GBMRGB gbmrgb[],
   MOD *modNew
   )
   {
   modNew->gbm.w   = w;
   modNew->gbm.h   = h;
   modNew->gbm.bpp = bpp;
   if ( gbmrgb != NULL && bpp != 24 )
      memcpy(&(modNew->gbmrgb), gbmrgb, sizeof(GBMRGB) << bpp);
   if ( !AllocateData(modNew) )
      return MOD_ERR_MEM;
   return MOD_ERR_OK;
   }
/*...e*/

/*...sModExpandTo24Bpp:0:*/
MOD_ERR ModExpandTo24Bpp(MOD *mod, MOD *mod24)
   {
   MOD_ERR mrc;
   int stride, stride24, y;

   if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h,
        24, mod->gbmrgb, mod24)) != MOD_ERR_OK )
      return mrc;

   stride   = StrideOf(mod  );
   stride24 = StrideOf(mod24);

   for ( y = 0; y < mod->gbm.h; y++ )
      {
      BYTE *pbSrc  = mod  ->pbData + y * stride  ;
      BYTE *pbDest = mod24->pbData + y * stride24;
      int x;

      switch ( mod->gbm.bpp )
         {
/*...s1:24:*/
case 1:
   {
   BYTE c;

   for ( x = 0; x < mod->gbm.w; x++ )
      {
      if ( (x & 7) == 0 )
         c = *pbSrc++;
      else
         c <<= 1;

      *pbDest++ = mod->gbmrgb[c >> 7].b;
      *pbDest++ = mod->gbmrgb[c >> 7].g;
      *pbDest++ = mod->gbmrgb[c >> 7].r;
      }
   }
   break;
/*...e*/
/*...s4:24:*/
case 4:
   for ( x = 0; x + 1 < mod->gbm.w; x += 2 )
      {
      BYTE c = *pbSrc++;

      *pbDest++ = mod->gbmrgb[c >> 4].b;
      *pbDest++ = mod->gbmrgb[c >> 4].g;
      *pbDest++ = mod->gbmrgb[c >> 4].r;
      *pbDest++ = mod->gbmrgb[c & 15].b;
      *pbDest++ = mod->gbmrgb[c & 15].g;
      *pbDest++ = mod->gbmrgb[c & 15].r;
      }

   if ( x < mod->gbm.w )
      {
      BYTE c = *pbSrc;

      *pbDest++ = mod->gbmrgb[c >> 4].b;
      *pbDest++ = mod->gbmrgb[c >> 4].g;
      *pbDest++ = mod->gbmrgb[c >> 4].r;
      }
   break;
/*...e*/
/*...s8:24:*/
case 8:
   for ( x = 0; x < mod->gbm.w; x++ )
      {
      BYTE c = *pbSrc++;

      *pbDest++ = mod->gbmrgb[c].b;
      *pbDest++ = mod->gbmrgb[c].g;
      *pbDest++ = mod->gbmrgb[c].r;
      }
   break;
/*...e*/
/*...s24:24:*/
case 24:
   memcpy(pbDest, pbSrc, stride);
   break;
/*...e*/
         }
      }

   return MOD_ERR_OK;
   }
/*...e*/
/*
** Write image data to file.
*/
MOD_ERR ModWriteToFile(MOD *mod,CHAR *szFn, CHAR *szOpt)
{
   GBM_ERR grc;
   int fd, ft, flag;
   GBMFT gbmft;

   if ( (grc = gbm_guess_filetype(szFn, &ft)) != GBM_ERR_OK )
      return grc;

   gbm_query_filetype(ft, &gbmft);
   switch ( mod->gbm.bpp )
   {
      case 1:      flag = GBM_FT_W1;   break;
      case 4:      flag = GBM_FT_W4;   break;
      case 8:      flag = GBM_FT_W8;   break;
      case 24:     flag = GBM_FT_W24;  break;
      }

   if ( (gbmft.flags & flag) == 0 )
      return MOD_ERR_SUPPORT;

   if ( (fd = open(szFn, O_CREAT|O_TRUNC|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE)) == -1 )
      return MOD_ERR_CREATE;

   if ( (grc = gbm_write(szFn, fd, ft, &(mod->gbm), mod->gbmrgb, mod->pbData, szOpt)) != GBM_ERR_OK )
      {
      close(fd);
      unlink(szFn);
      return MOD_ERR_GBM(grc);
      }

   close(fd);

   return MOD_ERR_OK;
   }
/*---------------------------------------------------------------------------*/
int ModCreateFromFile(CHAR *szFn, CHAR *szOpt,MOD *modNew)
{

GBM_ERR grc;
int fd, ft;

   if ( (grc = gbm_guess_filetype(szFn, &ft)) != GBM_ERR_OK )
      return MOD_ERR_GBM(grc);

   if ( (fd = open(szFn, O_RDONLY|O_BINARY)) == -1 )
      return MOD_ERR_OPEN;

   if ( (grc = gbm_read_header(szFn, fd, ft, &(modNew->gbm), szOpt)) != GBM_ERR_OK )
      {
      close(fd);
      return MOD_ERR_GBM(grc);
      }

   if ( (grc = gbm_read_palette(fd, ft, &(modNew->gbm), modNew->gbmrgb)) != GBM_ERR_OK )
      {
      close(fd);
      return MOD_ERR_GBM(grc);
      }

   if ( !AllocateData(modNew) )
      {
      close(fd);
      return MOD_ERR_MEM;
      }

   if ( (grc = gbm_read_data(fd, ft, &(modNew->gbm), modNew->pbData)) != GBM_ERR_OK )
      {
      free(modNew->pbData);
      close(fd);
      return MOD_ERR_GBM(grc);
      }

   close(fd);

   return MOD_ERR_OK;
}
/*---------------------------------------------------------------------------*/
static int ModMakeHBITMAP(MOD *mod, POBJECT pObj)
{
   USHORT usColors;
   ULONG  bytesperline,newdatasize;
   pImage pImg = (pImage)pObj;


   if (mod->gbm.bpp == 24)
      usColors = 0;
   else
      usColors = (1<<mod->gbm.bpp);


   pImg->pbmp2= (BITMAPINFOHEADER2 *)calloc(((ULONG)sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*usColors),
                                                        sizeof(char));

   pImg->pbmp2->cbFix    = sizeof(BITMAPINFOHEADER2);
   pImg->pbmp2->cx       = mod->gbm.w;
   pImg->pbmp2->cy       = mod->gbm.h;
   pImg->pbmp2->cBitCount= mod->gbm.bpp;
   pImg->pbmp2->cPlanes  = 1;
   pImg->pbmp2->cxResolution  = 0;
   pImg->pbmp2->cyResolution  = 0;
   pImg->pbmp2->usUnits       = BRU_METRIC;
   pImg->pbmp2->usRecording   = 0;
   pImg->pbmp2->usRendering   = 0;
   pImg->pbmp2->ulColorEncoding = BCE_RGB;
   pImg->pbmp2->cclrUsed      = usColors;
   pImg->pbmp2->cclrImportant = 0;


   if ( mod->gbm.bpp != 24 )
   {
      int i;
      RGB2  *prgb2;
      RGB2  *pRGB2;
      unsigned char *data;

      data = (unsigned char *)pImg->pbmp2;
      pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

      pObj->pDrwPal = (drw_palette *)calloc(sizeof(drw_palette),1);

      pObj->pDrwPal->nColors = usColors;
      pObj->pDrwPal->prgb2   = (RGB2 *)calloc(usColors,sizeof(RGB2));

      prgb2 = pObj->pDrwPal->prgb2;

      for ( i = 0; i < usColors; i++ )
      {
         prgb2->bRed   = pRGB2->bRed   = mod->gbmrgb[i].r;
         prgb2->bGreen = pRGB2->bGreen = mod->gbmrgb[i].g;
         prgb2->bBlue  = pRGB2->bBlue  = mod->gbmrgb[i].b;
         prgb2->fcOptions = pRGB2->fcOptions = 0;
         pRGB2++;
         prgb2++;
      }
   }
   else
   {
      pObj->pDrwPal = NULL;
   }
   pImg->ImgData = mod->pbData;
   bytesperline = ((pImg->pbmp2->cx * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
   newdatasize = bytesperline * pImg->pbmp2->cy;
   pImg->pbmp2->cbImage = newdatasize;
   pImg->ImageDataSize  = newdatasize;
   return 0;
}

static int Hbitmap2Mod(MOD *mod, pImage pImg)
{
   USHORT usColors;

   mod->gbm.w   = pImg->pbmp2->cx;
   mod->gbm.h   = pImg->pbmp2->cy;
   mod->gbm.bpp = pImg->pbmp2->cBitCount;

   if (mod->gbm.bpp == 24)
      usColors = 0;
   else
      usColors = (1<<mod->gbm.bpp);

   if ( mod->gbm.bpp != 24 )
   {
      int i;
      RGB2  *pRGB2;
      unsigned char *data;

      data = (unsigned char *)pImg->pbmp2;
      pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

      for ( i = 0; i < usColors; i++ )
      {
         mod->gbmrgb[i].r = pRGB2->bRed;
         mod->gbmrgb[i].g = pRGB2->bGreen;
         mod->gbmrgb[i].b = pRGB2->bBlue;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
   }
   mod->pbData = pImg->ImgData;
   return 0;
}

MOD_ERR ModCopy(MOD *mod, MOD *modNew)
   {
   modNew->gbm.w   = mod->gbm.w;
   modNew->gbm.h   = mod->gbm.h;
   modNew->gbm.bpp = mod->gbm.bpp;
   if ( mod->gbm.bpp != 24 )
      memcpy(modNew->gbmrgb, mod->gbmrgb, sizeof(GBMRGB) << mod->gbm.bpp);

   if ( !AllocateData(modNew) )
      return MOD_ERR_MEM;
   memcpy(modNew->pbData, mod->pbData, StrideOf(mod) * mod->gbm.h);
   return MOD_ERR_OK;
   }

/*...sModReflectHorz:0:*/
MOD_ERR ModReflectHorz(MOD *mod, MOD *modNew)
   {
   MOD_ERR mrc;
   if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
      return mrc;
   if ( !gbm_ref_horz(&(modNew->gbm), modNew->pbData) )
      {
      free(modNew->pbData);
      return MOD_ERR_MEM;
      }
   return MOD_ERR_OK;
   }

MOD_ERR ModReflectVert(MOD *mod, MOD *modNew)
   {
   MOD_ERR mrc;
   if ( (mrc = ModCopy(mod, modNew)) != MOD_ERR_OK )
      return mrc;
   if ( !gbm_ref_vert(&(modNew->gbm), modNew->pbData) )
      {
      free(modNew->pbData);
      return MOD_ERR_MEM;
      }
   return MOD_ERR_OK;
   }
/*...e*/
/*...sModDelete:0:*/
MOD_ERR ModDelete(MOD *mod)
   {
   free(mod->pbData);
   return MOD_ERR_OK;
   }
/*...e*/
/*...sModTranspose:0:*/
MOD_ERR ModTranspose(MOD *mod, MOD *modNew)
   {
   modNew->gbm.w   = mod->gbm.h;
   modNew->gbm.h   = mod->gbm.w;
   modNew->gbm.bpp = mod->gbm.bpp;
   if ( mod->gbm.bpp != 24 )
      memcpy(modNew->gbmrgb, mod->gbmrgb, sizeof(GBMRGB) << mod->gbm.bpp);
   if ( !AllocateData(modNew) )
      return MOD_ERR_MEM;
   gbm_transpose(&(mod->gbm), mod->pbData, modNew->pbData);
   return MOD_ERR_OK;
   }
/*---------------------------------------------------------------------------*/
static VOID ToGreyPal(GBMRGB *gbmrgb)
   {
   int   i;

   for ( i = 0; i < 0x100; i++ )
      gbmrgb [i].r =
      gbmrgb [i].g =
      gbmrgb [i].b = (byte) i;
   }
/*...e*/
/*...sToGrey:0:*/
static VOID ToGrey(GBM *gbm, byte *src_data, byte *dest_data)
   {
   int   src_stride  = ((gbm -> w * 3 + 3) & ~3);
   int   dest_stride = ((gbm -> w     + 3) & ~3);
   int   y;

   for ( y = 0; y < gbm -> h; y++ )
      {
      byte   *src  = src_data;
      byte   *dest = dest_data;
      int   x;

      for ( x = 0; x < gbm -> w; x++ )
         {
         byte   b = *src++;
         byte   g = *src++;
         byte   r = *src++;

         *dest++ = (byte) (((word) r * 77 + (word) g * 151 + (word) b * 28) >> 8);
         }

      src_data  += src_stride;
      dest_data += dest_stride;
      }
   gbm -> bpp = 8;
   }

/*...e*/
/*...sTripelPal:0:*/
static VOID TripelPal(GBMRGB *gbmrgb)
   {
   int   i;

   memset(gbmrgb, 0, 0x100 * sizeof(GBMRGB));

   for ( i = 0; i < 0x40; i++ )
      {
      gbmrgb [i       ].r = (byte) (i << 2);
      gbmrgb [i + 0x40].g = (byte) (i << 2);
      gbmrgb [i + 0x80].b = (byte) (i << 2);
      }
   }
/*...e*/
/*...sTripel:0:*/
static VOID Tripel(GBM *gbm, byte *src_data, byte *dest_data)
   {
   int   src_stride  = ((gbm -> w * 3 + 3) & ~3);
   int   dest_stride = ((gbm -> w     + 3) & ~3);
   int   y;

   for ( y = 0; y < gbm -> h; y++ )
      {
      byte   *src  = src_data;
      byte   *dest = dest_data;
      int   x;

      for ( x = 0; x < gbm -> w; x++ )
         {
         byte   b = *src++;
         byte   g = *src++;
         byte   r = *src++;

         switch ( (x+y)%3 )
            {
            case 0:   *dest++ = (byte)         (r >> 2) ;   break;
            case 1:   *dest++ = (byte) (0x40 + (g >> 2));   break;
            case 2:   *dest++ = (byte) (0x80 + (b >> 2));   break;
            }
         }

      src_data  += src_stride;
      dest_data += dest_stride;
      }
   gbm -> bpp = 8;
   }
/*...e*/

static BOOL BppMap(MOD *mod24,int iPal, int iAlg,int iKeepRed, int iKeepGreen, int iKeepBlue, int nCols,
   MOD *modNew)
{
   BYTE rm = (BYTE) (0xff00 >> iKeepRed  );
   BYTE gm = (BYTE) (0xff00 >> iKeepGreen);
   BYTE bm = (BYTE) (0xff00 >> iKeepBlue );
   BOOL ok = TRUE;

#define   SW2(a,b) (((a)<<8)|(b))

   switch ( SW2(iPal,iAlg) )
      {
      case SW2(CVT_BW,CVT_NEAREST):
         gbm_trunc_pal_BW(modNew->gbmrgb);
         gbm_trunc_BW(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_4G,CVT_NEAREST):
         gbm_trunc_pal_4G(modNew->gbmrgb);
         gbm_trunc_4G(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_8,CVT_NEAREST):
         gbm_trunc_pal_8(modNew->gbmrgb);
         gbm_trunc_8(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_VGA,CVT_NEAREST):
         gbm_trunc_pal_VGA(modNew->gbmrgb);
         gbm_trunc_VGA(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_784,CVT_NEAREST):
         gbm_trunc_pal_7R8G4B(modNew->gbmrgb);
         gbm_trunc_7R8G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_666,CVT_NEAREST):
         gbm_trunc_pal_6R6G6B(modNew->gbmrgb);
         gbm_trunc_6R6G6B(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_8G,CVT_NEAREST):
         ToGreyPal(modNew->gbmrgb);
         ToGrey(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_TRIPEL,CVT_NEAREST):
         TripelPal(modNew->gbmrgb);
         Tripel(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_FREQ,CVT_NEAREST):
         memset(modNew->gbmrgb, 0, sizeof(modNew->gbmrgb));
         gbm_hist(&(modNew->gbm), mod24->pbData, modNew->gbmrgb, modNew->pbData, nCols, rm, gm, bm);
         break;
      case SW2(CVT_MCUT,CVT_NEAREST):
         memset(modNew->gbmrgb, 0, sizeof(modNew->gbmrgb));
         gbm_mcut(&(modNew->gbm), mod24->pbData, modNew->gbmrgb, modNew->pbData, nCols);
         break;
      case SW2(CVT_RGB,CVT_NEAREST):
         gbm_trunc_24(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
         break;
      case SW2(CVT_BW,CVT_ERRDIFF):
         gbm_errdiff_pal_BW(modNew->gbmrgb);
         ok = gbm_errdiff_BW(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_4G,CVT_ERRDIFF):
         gbm_errdiff_pal_4G(modNew->gbmrgb);
         ok = gbm_errdiff_4G(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_8,CVT_ERRDIFF):
         gbm_errdiff_pal_8(modNew->gbmrgb);
         ok = gbm_errdiff_8(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_VGA,CVT_ERRDIFF):
         gbm_errdiff_pal_VGA(modNew->gbmrgb);
         ok = gbm_errdiff_VGA(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_784,CVT_ERRDIFF):
         gbm_errdiff_pal_7R8G4B(modNew->gbmrgb);
         ok = gbm_errdiff_7R8G4B(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_666,CVT_ERRDIFF):
         gbm_errdiff_pal_6R6G6B(modNew->gbmrgb);
         ok = gbm_errdiff_6R6G6B(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_RGB,CVT_ERRDIFF):
         ok = gbm_errdiff_24(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
         break;
      case SW2(CVT_784,CVT_HALFTONE):
         gbm_ht_pal_7R8G4B(modNew->gbmrgb);
         gbm_ht_7R8G4B_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_666,CVT_HALFTONE):
         gbm_ht_pal_6R6G6B(modNew->gbmrgb);
         gbm_ht_6R6G6B_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_8,CVT_HALFTONE):
         gbm_ht_pal_8(modNew->gbmrgb);
         gbm_ht_8_3x3(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_VGA,CVT_HALFTONE):
         gbm_ht_pal_VGA(modNew->gbmrgb);
         gbm_ht_VGA_3x3(&(modNew->gbm), mod24->pbData, modNew->pbData);
         break;
      case SW2(CVT_RGB,CVT_HALFTONE):
         gbm_ht_24_2x2(&(modNew->gbm), mod24->pbData, modNew->pbData, rm, gm, bm);
         break;
   }
   return ok;
}
/*...e*/


MOD_ERR ModBppMap(MOD *mod,int iPal, int iAlg,int iKeepRed, int iKeepGreen, int iKeepBlue, int nCols,
   MOD *modNew)
{
   MOD_ERR mrc;
   int newbpp;
   BOOL fOk;

   switch ( iPal )
      {
      case CVT_BW:
         newbpp = 1;
         break;
      case CVT_4G:
      case CVT_8:
      case CVT_VGA:
         newbpp = 4;
         break;
      case CVT_784:
      case CVT_666:
      case CVT_8G:
      case CVT_TRIPEL:
      case CVT_FREQ:
      case CVT_MCUT:
         newbpp = 8;
         break;
      case CVT_RGB:
         newbpp = 24;
         break;
      }

   /* The following breakdown into cases can result in significant
      savings in memory by eliminating costly intermediate bitmaps. */

   if ( mod->gbm.bpp == 24 )
      /* 24bpp -> anything, map direct from mod */
      {
      if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h, newbpp, NULL, modNew)) != MOD_ERR_OK )
         return mrc;
      fOk = BppMap(mod, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
      }
   else if ( newbpp == 24 )
      /* !24 -> 24bpp, expand mod to modNew, map modNew inline */
      {
      if ( (mrc = ModExpandTo24Bpp(mod, modNew)) != MOD_ERR_OK )
         return mrc;
      fOk = BppMap(modNew, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
      }
   else
      /* !24bpp -> 24bpp, expand mod to mod24, and map mod24 to modNew */
      {
      MOD mod24;
      if ( (mrc = ModCreate(mod->gbm.w, mod->gbm.h, newbpp, NULL, modNew)) != MOD_ERR_OK )
         return mrc;
      if ( (mrc = ModExpandTo24Bpp(mod, &mod24)) != MOD_ERR_OK )
         {
         ModDelete(modNew);
         return mrc;
         }
      fOk = BppMap(&mod24, iPal, iAlg, iKeepRed, iKeepGreen, iKeepBlue, nCols, modNew);
      ModDelete(&mod24);
      }

   return fOk ? MOD_ERR_OK : MOD_ERR_MEM;
}

MOD_ERR ModRotate270(MOD *mod, MOD *modNew)
   {
   MOD_ERR mrc;
   MOD modTmp;

   if ( (mrc = ModReflectHorz(mod, &modTmp)) != MOD_ERR_OK )
      return mrc;
   mrc = ModTranspose(&modTmp, modNew);
   ModDelete(&modTmp);
   return mrc;
   }

MOD_ERR ModRotate90(MOD *mod, MOD *modNew)
{
   MOD_ERR mrc;
   MOD modTmp;

   if ( (mrc = ModReflectVert(mod, &modTmp)) != MOD_ERR_OK )
      return mrc;
   mrc = ModTranspose(&modTmp, modNew);
   ModDelete(&modTmp);
   return mrc;
}
/*------------------------------------------------------------------------*/
/* ImgRotate90.                                                           */
/*------------------------------------------------------------------------*/
BOOL ImgRotate(WINDOWINFO *pwi,POBJECT pObject, int degrees)
{
  MOD    modNew,mod;
  int    mrc;
  float  fAspect;
  float  xPos,yPos,xCenter,yCenter,xWidth,yHeight;
  RECTL  rcl;
  pImage pImg = (pImage)pObject;

  if (!pObject)
     return FALSE;

  if (degrees != 90 && degrees != 270)  return FALSE;

  Hbitmap2Mod(&mod,pImg);

  if (degrees == 90)
     mrc = ModRotate90(&mod, &modNew);
  else
     mrc = ModRotate270(&mod, &modNew);

  if (mrc == MOD_ERR_OK)
  {
      pImg->pbmp2->cx = modNew.gbm.w;
      pImg->pbmp2->cy = modNew.gbm.h;
      pImg->ImgData   = modNew.pbData;
      /*
      ** Remember upperleft corner of the displayed image.
      */
      yPos   = (pImg->y      * pwi->usHeight);
      yHeight= (pImg->cydest * pwi->usHeight);
      yCenter= yPos + (yHeight/2);

      xPos   = (pImg->x      * pwi->usWidth);
      xWidth = (pImg->cxdest * pwi->usWidth);
      xCenter= xPos + (xWidth/2);

      yPos   = yCenter - xWidth/2;
      xPos   = xCenter - yHeight/2;

      pImg->y = yPos/pwi->usHeight;
      pImg->x = xPos/pwi->usWidth;
      pImg->cydest = xWidth/pwi->usHeight;
      pImg->cxdest = yHeight/pwi->usWidth;
      return TRUE;
  }
  return FALSE;
}
/*------------------------------------------------------------------------*/
/* SetupImageSegment.                                                     */
/*------------------------------------------------------------------------*/
pImage SetupImgSegment(pImage pImg,WINDOWINFO *pwi)
{
   POINTL ptl;
   RECTL  rcl;
   float  fAspect;
   long   lMenu;

   ptl.x = 0;
   ptl.y = 0;

   if (!pImg)
      return (pImage)0;

   lMenu = (LONG)WinQuerySysValue(HWND_DESKTOP,SV_CYMENU) * 2;

   WinQueryWindowRect(pwi->hwndClient, &rcl);

   ptl.y = rcl.yTop - pImg->pbmp2->cy;

   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);

   if (pImg)
   {
      /*
      ** Avoid the image to disappear under the menu.
      */
      if (pImg->ptf.x == 0.0 || pImg->ptf.y == 0.0)
      {
         ptl.y -= lMenu;
         pImg->y = (float)ptl.y;
         pImg->x = (float)ptl.x;

         pImg->y /= (float)pwi->usHeight;
         pImg->x /= (float)pwi->usWidth;
      }
      else
      {
         /*
         ** When an image is dropped it gets a drop position.
         */
         pImg->y = pImg->ptf.y;
         pImg->x = pImg->ptf.x;
      }
      pImg->ImgCir     = FALSE;
      pImg->ustype     = CLS_IMG;
      pImg->uslayer    = pwi->uslayer;
      pImg->lPattern   = PATSYM_DEFAULT;
      pImg->lRopCode   = ROP_SRCCOPY;
      /*
      ** Set the initial display parameters.
      */
      ptl.x = pImg->pbmp2->cx;
      ptl.y = pImg->pbmp2->cy;
      GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);
      pImg->cydest = pImg->cy  = (float)ptl.y;
      pImg->cxdest = pImg->cx  = (float)ptl.x;

      /*
      ** Initial width may not exceed 80% of the form width
      */
      if ( pImg->cxdest > (float)( pwi->usWidth * 0.8) )
      {
        fAspect =  (float)pImg->cydest;
        fAspect /= (float)pImg->cxdest;
        pImg->cxdest = (float)pwi->usWidth;
        pImg->cxdest *= (float)0.8;
        pImg->cydest = pImg->cxdest * fAspect;
        pImg->cx     = pImg->cxdest;
        pImg->cy     = pImg->cydest;

        pImg->y = (float)pwi->usHeight;
        pImg->y -= (pImg->cydest + lMenu); // menu.
        pImg->x = (float)50.0;
        pImg->y /= (float)pwi->usHeight;
        pImg->x /= (float)pwi->usWidth;
      }

      pImg->cydest /= (float)pwi->usHeight;
      pImg->cxdest /= (float)pwi->usWidth;
      pImg->cy     /= (float)pwi->usHeight;
      pImg->cx     /= (float)pwi->usWidth;
   }
   return pImg;
}
/*------------------------------------------------------------------------*/
/* ImgSetup.                                                              */
/*                                                                        */
/*------------------------------------------------------------------------*/
POBJECT ImgSetup(WINDOWINFO *pwi, char *pszFilename)
{
  POBJECT pObj;
  char   buf[350];
  char   *pDot;

  if (!pszFilename)
  {
     li.dlgflags = FDS_OPEN_DIALOG;

     strcpy(li.szExtension,".*");        /* Set default extension */

     if (!FileGetName(&li,pwi))
        return NULL;

      pDot = strchr(li.szFileName,'.');

      if (!pDot)
         return NULL;

      pDot++; /* Jump over dot in filename. */
      if (!stricmp("JSP",pDot))
      {
         sprintf(buf,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
                 "This is a not a bitmap file like",
                 "OS/2 2.x Bitmap    (*.BMP)",
                 "Windows 3.x Bitmap (*.BMP)",
                 "Compuserve GIF     (*.GIF)",
                 "PaintBrush PCX     (*.PCX)",
                 "HP TIFF            (*.TIF)",
                 "\nThis is a DRAWIT file (*.JSP)",
                 "Use Ctrl+O or Open in the file menu",
                 "to load this file.");

         WinMessageBox(HWND_DESKTOP,hwndClient,buf,
                       "Not a bitmap file",
                       0,
                       MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                       MB_ICONEXCLAMATION);
         return NULL;
      }
      strcpy(pwi->szFilename,li.szFileName);
   }
   else
      strcpy(pwi->szFilename,pszFilename);

   pObj =  pObjNew(NULL, CLS_IMG);
   pObj->moveOutline = ImgMoveOutLine;
   pObj->paint       = DrawImgSegment;
   pObj->getInvalidationArea    = ImgInvArea;
   return pObj;

}
/*------------------------------------------------------------------------*/
/* Creates an image and posts a message to the main thread after          */
/* completion. (This routine runs in a separate thread.)                  */
/*------------------------------------------------------------------------*/
void _System CreateImgSegment(WINDOWINFO *pwi)
{
   pImage pImg = (pImage)pwi->pvCurrent;
   POBJECT pObj= pwi->pvCurrent;
   MOD   modNew;
   int   mrc;
   char  *p;
   BOOL  bJpeg=FALSE;

   /*
   ** First check which library we need to access.
   ** When the filename ends on JPG we simply enter the
   ** JPEG lib else we go on with the GBMSRC of IBM.
   */
   p = strchr(strupr(pwi->szFilename),'.');
   if (p)
   {
      p++;
      if (*p == 'J' && *(p+1) == 'P' && *(p+2) == 'G')
      {
         if (!readjpeg(pwi,pImg))
         {
            free(pImg);
            WinPostMsg(pwi->hwndClient,UM_IMGLOADED,(MPARAM)0,(MPARAM)0);
            return;
         }
         bJpeg = TRUE;
         mrc   = MOD_ERR_OK;
      }
   }

   if (!bJpeg)
      mrc = ModCreateFromFile(pwi->szFilename, (char *)"",&modNew);

   if (mrc == MOD_ERR_OK)
   {
      if (!bJpeg)
         ModMakeHBITMAP(&modNew,pObj);
      pImg = SetupImgSegment(pImg,pwi);
      pObjAppend((POBJECT)pImg);
      WinPostMsg(pwi->hwndClient,UM_IMGLOADED,(MPARAM)pImg,(MPARAM)0);
   }
   else
   {
      free(pImg);
      WinPostMsg(pwi->hwndClient,UM_IMGLOADED,(MPARAM)0,(MPARAM)0);
   }
   return;
}
/*------------------------------------------------------------------------*/
/* void imgLoad                                                           */
/*------------------------------------------------------------------------*/
void ImgLoad(WINDOWINFO *pwi, char *pszFilename)
{
   POBJECT  pObj;
   APIRET   rc;
   TID      ThreadID;       /* New thread ID (returned)                   */
   ULONG    ThreadFlags;    /* When to start thread,how to allocate stack */
   ULONG    StackSize;      /* Size in bytes of new thread's stack        */

   pObj = ImgSetup(pwi,pszFilename);

   if (pObj)
   {
      pwi->pvCurrent = pObj;
      ThreadFlags = 0;        /* Indicate that the thread is to */
                              /* be started immediately         */
      StackSize = 45000;      /* Set the size for the new       */
                              /* thread's stack                 */
      rc = DosCreateThread(&ThreadID,(PFNTHREAD)CreateImgSegment,
                           (ULONG)pwi,ThreadFlags,StackSize);
      /*
      ** Tell status bar that we are loading an image.
      */
      if (!rc)
         WinPostMsg(pwi->hwndMain,UM_READWRITE,
                    (MPARAM)IDS_LOADING,(MPARAM)0);
   }
   return;
}
/*------------------------------------------------------------------------*/
/* Flip image Vertical.....                                               */
/*------------------------------------------------------------------------*/
void FlipImgVert(POBJECT pObj)
{
   PBYTE ImgDat;
   BYTE  data;
   ULONG i,p,ScanLine;
   ULONG bytesperline,half;
   pImage pImg = (pImage)pObj;

   if (!pImg || !pImg->pbmp2)
      return;

   bytesperline = ((pImg->pbmp2->cx * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;

   ImgDat   = pImg->ImgData;


   half = bytesperline /2;

   p = 0;
   for (ScanLine = 0; ScanLine < pImg->pbmp2->cy; ScanLine++)
   {
      for ( i = 0; i < half; i++)
      {
         data = *(ImgDat+i+p);
         *(ImgDat+i+p) = *(ImgDat+(bytesperline - i)+p);
         *(ImgDat+(bytesperline - i)+p) = data;
      }
      p += bytesperline;
   }
}
/*------------------------------------------------------------------------*/
/* Flip image horizontal.....                                             */
/*------------------------------------------------------------------------*/
void FlipImgHorz(POBJECT pObj)
{
   PBYTE ImgDat;
   BYTE  data;
   ULONG i,datasize;
   pImage pImg = (pImage)pObj;

  if (!pImg || !pImg->pbmp2)
     return;

  datasize = pImg->ImageDataSize;
  ImgDat   = pImg->ImgData;

  for ( i = 0; i < ( datasize / 2); i ++)
  {
     data = *(ImgDat+i);
     *(ImgDat+i) = *(ImgDat+(datasize - i));
     *(ImgDat+(datasize - i)) = data;
  }
}
/*------------------------------------------------------------------------*/
/*  Name: ImgRotateColor                                                  */
/*                                                                        */
/*  Description : Rotates color of a colored or grayscaled image if the   */
/*                usmode parameter contains a '+' or '-'.                 */
/*                Increases the brightness of a color or grayscaled image */
/*                if the usmode parameter contains a 'b' or 'B'.          */
/*                Inverts the image if the usmode contains a 'I' or 'i'.  */
/*                The function is called from the windowproc during a     */
/*                WM_CHAR messages when a image is selected.              */
/*                                                                        */
/*  Parameters  : pImage - a pointer to the image structure containing the*/
/*                bitmap data to work on.                                 */
/*                USHORT usmode - containing the char defining the        */
/*                operation within the function.                          */
/*  Returns     : BOOL - TRUE on success, FALSE if no action has taken    */
/*                place.                                                  */
/*------------------------------------------------------------------------*/
BOOL ImgRotateColor(pImage pImg, SHORT usmode)
{
   PBYTE  data;
   ULONG i,nColors;
   PRGB2 pRGB2;
   BOOL  bRet = FALSE;

   if (!pImg || !pImg->pbmp2)
      return bRet;

   data = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );


   if (nColors <= 256 && (CHAR)usmode == '+')
   {
      for ( i = 0; i < nColors; i++ )
      {
         pRGB2->bRed   += 0x10;
         pRGB2->bGreen += 0x10;
         pRGB2->bBlue  += 0x10;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
      bRet = TRUE;
   }
   else if (nColors <= 256 && (CHAR)usmode == '-' )
   {
      for ( i = 0; i < nColors; i++ )
      {
         pRGB2->bRed   -= 0x10;
         pRGB2->bGreen -= 0x10;
         pRGB2->bBlue  -= 0x10;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
      bRet = TRUE;
   }
   else if (nColors <= 256 && ( (CHAR)usmode == 'b' || (CHAR)usmode == 'B' ) )
   {
      for ( i = 0; i < nColors; i++ )
      {
         if (pRGB2->bRed < 0xF0 )
            pRGB2->bRed   += 0x05;
         if (pRGB2->bGreen < 0xF0 )
            pRGB2->bGreen += 0x05;
         if (pRGB2->bBlue < 0xF0 )
            pRGB2->bBlue  += 0x05;

         pRGB2->fcOptions = 0;
         pRGB2++;
      }
      bRet = TRUE;
   }
   else if (nColors <= 256 && ( (CHAR)usmode == 'i' || (CHAR)usmode == 'I' ) )
   {
      for ( i = 0; i < nColors; i++ )
      {
         pRGB2->bRed   = ~pRGB2->bRed;
         pRGB2->bGreen = ~pRGB2->bGreen;
         pRGB2->bBlue  = ~pRGB2->bBlue;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
      bRet = TRUE;
   }
   else if (pImg->pbmp2->cBitCount == 24 && (CHAR)usmode == '+')
   {
     /*
     ** True color image colorrotation...
     */
     data = (PBYTE )pImg->ImgData;
     for ( i = 0; i < pImg->pbmp2->cbImage; i += 3 )
     {
        *data  += 0x10;
        data++;
        *data  += 0x10;
        data++;
        *data  += 0x10;
        data++;
      }
      bRet = TRUE;
  }
  else if (pImg->pbmp2->cBitCount == 24 && (CHAR)usmode == '-')
  {
     /*
     ** True color image colorrotation...
     */
     data = (PBYTE )pImg->ImgData;
     for ( i = 0; i < pImg->pbmp2->cbImage; i += 3 )
     {
        *data  -= 0x10;
        data++;
        *data  -= 0x10;
        data++;
        *data  -= 0x10;
        data++;
      }
      bRet = TRUE;
  }
  else if (pImg->pbmp2->cBitCount == 24 && ( (CHAR)usmode == 'b'|| (CHAR)usmode == 'B') )
  {
     /*
     ** True color image brightness.
     */
     data = (PBYTE )pImg->ImgData;
     for ( i = 0; i < pImg->pbmp2->cbImage; i += 3 )
     {
        if (*data < 0xF0 )
           *data  += 0x05;
        data++;
        if (*data < 0xF0 )
           *data  += 0x05;
        data++;
        if (*data < 0xF0 )
           *data  += 0x05;
        data++;
      }
      bRet = TRUE;
  }
  else if (pImg->pbmp2->cBitCount == 24 && ( (CHAR)usmode == 'i'|| (CHAR)usmode == 'I') )
  {
     /*
     ** True color image inverse
     */
     data = (PBYTE )pImg->ImgData;
     for ( i = 0; i < pImg->pbmp2->cbImage; i += 3 )
     {
        *data  = ~*data;
        data++;
        *data  = ~*data;
        data++;
        *data  = ~*data;
        data++;
      }
      bRet = TRUE;
  }
  return bRet;
}
/*------------------------------------------------------------------------*/
/*  Name: ImgRestoreAspect.                                               */
/*                                                                        */
/*  Description : Restores the aspect ratio of an image using the biggest */
/*                size as the reference.                                  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ImgRestoreAspect(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi)
{
   float fAspect;
   POINTL ptl;
   LONG   cx,cy;

   pImage pImg = (pImage)pObj;

   if (!pObj)
      return FALSE;

  if (pObj->usClass != CLS_IMG) return FALSE;

  /*
  ** Make 0.1 mm size and calc the aspect ratio
  ** device independance!
  */
  ptl.x = pImg->pbmp2->cx;
  ptl.y = pImg->pbmp2->cy;

  fAspect =  (float)ptl.x;
  fAspect /= (float)ptl.y;
  cy =  (LONG)(pImg->cydest * pwi->usFormHeight);
  cx = (fAspect * cy);

  pImg->cxdest = (float)cx;
  pImg->cxdest /= pwi->usFormWidth;


  /*
  ** As a service we deliver to our caller the new inv area...
  */
  ImgInvArea((POBJECT)pImg,rcl,pwi,TRUE);

  return TRUE;
}
/*------------------------------------------------------------------------*/
/*  Name: ImgRestoreSize.                                                 */
/*                                                                        */
/*  Description : Restores the original size of the selected image.       */
/*                Called from the menu. (drwmain.c).                      */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ImgRestoreSize(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi)
{
   POINTL ptl;
   pImage pImg = (pImage)pObj;
   float  fAspect;

   if (!pObj)
      return FALSE;

  if (pObj->usClass != CLS_IMG) return FALSE;

  ptl.x = pImg->pbmp2->cx;
  ptl.y = pImg->pbmp2->cy;
  GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);
  pImg->cxdest = (float)ptl.x;
  pImg->cydest = (float)ptl.y;

  /*
  ** Initial width may not exceed 80% of the form width
  */
  if ( pImg->cxdest > (float)( pwi->usWidth * 0.8) )
  {
        fAspect =  (float)pImg->cydest;
        fAspect /= (float)pImg->cxdest;
        pImg->cxdest = ( pwi->usWidth * 0.8 );
        pImg->cydest = pImg->cxdest * fAspect;

        pImg->y = (float)pwi->usHeight;
        pImg->y -= (pImg->cydest + 10); // 10 for menu.
        pImg->x = (float)50.0;
        pImg->y /= (float)pwi->usHeight;
        pImg->x /= (float)pwi->usWidth;

  }

  pImg->cxdest /= (float)pwi->usWidth;
  pImg->cydest /= (float)pwi->usHeight;
  /*
  ** As a service we deliver to our caller the new inv area...
  */
  ImgInvArea((POBJECT)pImg,rcl,pwi,TRUE);
  return TRUE;
}
/*-----------------------------------------------[ public ]---------------*/
/*  Name: imgUndoPalette.                                                 */
/*                                                                        */
/*  Description : Restores the original palette. Can be used when the     */
/*                palette is changed via the popupmenu options.           */
/*                                                                        */
/*  PreCondition: No action is taken when image does not contain a pal.   */
/*                                                                        */
/*  Parameters  : POBJECT pObj - pointer to an image object.              */
/*                                                                        */
/*  Returns     : BOOL - TRUE on succes.                                  */
/*------------------------------------------------------------------------*/
BOOL imgUndoPalette( POBJECT pObj)
{
   pImage pImg = (pImage)pObj;
   drw_palette *pDrwPal;
   RGB2 *pRGB2,*prgb2;
   BYTE *data;
   ULONG nColors;
   int   i;

   pDrwPal = pObj->pDrwPal;

   if (pObj->usClass != CLS_IMG || !pDrwPal)
      return FALSE;

   data  = (BYTE *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

   prgb2 = pObj->pDrwPal->prgb2;

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (nColors <= 256)
   {
      for (i = 0; i < nColors; i++ )
      {
         pRGB2->bRed   = prgb2->bRed;
         pRGB2->bGreen = prgb2->bGreen;
         pRGB2->bBlue  = prgb2->bBlue;
         pRGB2->fcOptions = 0;
         prgb2++;
         pRGB2++;
      }
      return TRUE;
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/*  Name: imgGrayScale                                                    */
/*                                                                        */
/*  Description : Converts an color bitmap image into an grayscale img    */
/*                image.                                                  */
/*                                                                        */
/*  Parameters : pImage pImg: Pointer to the selected image.              */
/*                                                                        */
/*  Returns:  BOOL : TRUE on success.                                     */
/*------------------------------------------------------------------------*/
BOOL imgGrayScale(POBJECT pObj)
{
   PBYTE data,p;
   ULONG i,nColors,ulDataSize;
   PRGB2 pRGB2;
   BYTE  MaxColor;
   pImage pImg = (pImage)pObj;

   if (!pImg || !pImg->pbmp2)
      return FALSE;

   data = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (nColors <= 256)
   {
      for ( i = 0; i < nColors; i++ )
      {
         MaxColor = pRGB2->bRed;
         if ( pRGB2->bGreen > MaxColor)
            MaxColor = pRGB2->bGreen;
         if ( pRGB2->bBlue > MaxColor)
            MaxColor = pRGB2->bBlue;

         pRGB2->bRed   = MaxColor;
         pRGB2->bGreen = MaxColor;
         pRGB2->bBlue  = MaxColor;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
   }
   else if (pImg->pbmp2->cBitCount == 24)
   {
      /* Wow! a true color image? */
      ulDataSize = pImg->pbmp2->cbImage;
      printf("ulDataSize=%ld\n",ulDataSize);
      p = (PBYTE )pImg->ImgData;
      data = p;
      for ( i = 0; i < ulDataSize; i += 3 )
      {
         MaxColor = *p++;
         if ( *p > MaxColor)
            MaxColor = *p;
         p++;
         if ( *p > MaxColor)
            MaxColor = *p;
         *p++;
         *data++ = MaxColor;
         *data++ = MaxColor;
         *data++ = MaxColor;
      }
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
/*  Name: imgBlackWhite                                                   */
/*------------------------------------------------------------------------*/
BOOL imgBlackWhite(POBJECT pObj)
{
   PBYTE data,p;
   ULONG i,nColors,ulDataSize;
   PRGB2 pRGB2;
   BYTE  MaxColor;
   pImage pImg = (pImage)pObj;

   if (!pImg || !pImg->pbmp2)
      return FALSE;

   data = (unsigned char *)pImg->pbmp2;
   pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (nColors <= 256)
   {
      for ( i = 0; i < nColors; i++ )
      {
         MaxColor = pRGB2->bRed;
         if ( pRGB2->bGreen > MaxColor)
            MaxColor = pRGB2->bGreen;
         if ( pRGB2->bBlue > MaxColor)
            MaxColor = pRGB2->bBlue;
         if (MaxColor > MAX_COLORVAL )
         {
            pRGB2->bRed   = 0xFF;
            pRGB2->bGreen = 0xFF;
            pRGB2->bBlue  = 0xFF;
         }
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
   }
   else if (pImg->pbmp2->cBitCount == 24)
   {
      /* Wow! a true color image? */
      ulDataSize = pImg->pbmp2->cbImage;
      p = (PBYTE )pImg->ImgData;
      data = p;
      for ( i = 0; i < ulDataSize; i += 3 )
      {
         MaxColor = *p++;
         if ( *p > MaxColor)
            MaxColor = *p;
         p++;
         if ( *p > MaxColor)
            MaxColor = *p;
         *p++;
         if (MaxColor > MAX_COLORVAL)
             MaxColor = 0xFF;
         *data++ = MaxColor;
         *data++ = MaxColor;
         *data++ = MaxColor;
      }
   }
   return TRUE;
}
/*------------------------------------------------------------------------*/
/* Image outline is the exact outline in window coords.                   */
/*------------------------------------------------------------------------*/
void ImgOutLine(POBJECT pObj, RECTL *rcl, WINDOWINFO *pwi)
{
   pImage pImg = (pImage)pObj;
   BOOL   bCalc;
   RECTL  r;


   if (!pImg)
      return;

   r.xLeft    = pImg->x * pwi->usFormWidth;
   r.xRight   = r.xLeft + (pImg->cxdest * pwi->usFormWidth );
   r.yBottom  = pImg->y * pwi->usFormHeight;
   r.yTop     = r.yBottom + (pImg->cydest * pwi->usFormHeight);

   if (pImg->nrpts)
   {
      GpiResetBoundaryData(pwi->hps);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_ON);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_OFF);
      drawPolyLine(pwi->hps,pImg,r);
      GpiSetDrawControl(pwi->hps, DCTL_BOUNDARY,DCTL_OFF);
      GpiSetDrawControl(pwi->hps, DCTL_DISPLAY, DCTL_ON);
      GpiQueryBoundaryData(pwi->hps, rcl);
   }
   else
   {
      *rcl = r;
   }
}
/*------------------------------------------------------------------------*/
/* ImgInvArea()                                                           */
/*------------------------------------------------------------------------*/
void ImgInvArea(POBJECT pObj, RECTL *rcl, WINDOWINFO *pwi, BOOL bInc)
{
   pImage pImg = (pImage)pObj;

   if (!pImg)
      return;

   ImgOutLine((POBJECT)pImg,rcl,pwi);
   /*
   ** Add extra space for selection handles.
   */
   if (bInc)
   {
      rcl->xLeft    -= 20;
      rcl->xRight   += 20;
      rcl->yBottom  -= 20;
      rcl->yTop     += 20;
   }
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,2,(POINTL *)rcl);
}
/*------------------------------------------------------------------------*/
/*  Name: DrawImgSegment                                                  */
/*                                                                        */
/*  Description : Draws the given imagesegment into the given presentation*/
/*                space.                                                  */
/*                                                                        */
/*                                                                        */
/*  Parameters : HPS hps - presentation space, can printer,metafile,      */
/*                         bitmap or just the screen.                     */
/*               WINDOWINFO - pointer to the printer/window/meta info     */
/*               pImage  - pointer to the imagesegment.                   */
/*               RECTL * - pointer to the invalidated rectangle, only when*/
/*                         called from a WM_PAINT ! else this points to   */
/*                         NULL.                                          */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
VOID DrawImgSegment(HPS hps,WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl)
{
   HBITMAP hbm;
   POINTL  aBitmap[4];
   RECTL   rcl,rclDest;
   pImage  pImg;

   pImg = (pImage)pObj;

   if (pwi->usdrawlayer == pImg->uslayer)
   {
      if (prcl)
      {
         ImgOutLine((POBJECT)pImg,&rcl,pwi);
         if (!WinIntersectRect(hab,&rclDest,prcl,&rcl))
            return;
      }

      hbm = GpiCreateBitmap(pwi->hpsMem,      /* presentation-space handle */
                            pImg->pbmp2,      /* address of structure for format data */
                            CBM_INIT,         /* options     */
                            pImg->ImgData,    /* bitmap data */
                            (PBITMAPINFO2)pImg->pbmp2); /* address of structure for color and format */

      if (pImg->ImgCir)
      {
         ImageCircle(hbm,hps,pImg,pwi);
      }
      else if (pImg->pptl)
         drawImgInClippath(hbm,hps,pImg,pwi);
      else
      {
         aBitmap[0].x = (LONG)( pImg->x * pwi->usFormWidth ); // Dest Lower LH Corner
         aBitmap[0].y = (LONG)( pImg->y * pwi->usFormHeight);
         aBitmap[1].x = aBitmap[0].x + (long)(pImg->cxdest * pwi->usFormWidth );
         aBitmap[1].y = aBitmap[0].y + (long)(pImg->cydest * pwi->usFormHeight);
         aBitmap[2].x = 0;                  // Source Lower LH Corner
         aBitmap[2].y = 0;
         aBitmap[3].x = pImg->pbmp2->cx-1;   // Source Upper RH Corner
         aBitmap[3].y = pImg->pbmp2->cy-1;
//      printf("Size cx=%f cy=%f \n",aBitmap[1].x,aBitmap[1].y);
         if (pImg->lPattern == PATSYM_DEFAULT ||
             pImg->lPattern == PATSYM_SOLID   ||
             pImg->lPattern == PATSYM_GRADIENTFILL )
            GpiWCBitBlt(hps,  /* presentation space                      */
            hbm,              /* bit-map handle                          */
            4L,               /* four points needed to compress          */
            aBitmap,          /* points for source and target rectangles */
            pImg->lRopCode,   /* copy source replacing target            */
            BBO_IGNORE);      /* discard extra rows and columns          */
         else
         {
            GpiSetPattern(hps,pImg->lPattern);
            GpiWCBitBlt(hps,  /* presentation space                      */
            hbm,              /* bit-map handle                          */
            4L,               /* four points needed to compress          */
            aBitmap,          /* points for source and target rectangles */
            ROP_MERGECOPY,    /* copy source replacing target            */
            BBO_IGNORE);      /* discard extra rows and columns          */
         }
//@ch1   GpiSetBitmap(hps,(HBITMAP)0); /*Deselect any bitmap from ps */
         GpiDeleteBitmap(hbm);
      }
   } /*drawlayer*/
}
/*****************************************************************************/
/*                                                                           */
/* Create a memory DC and an associated PS.                                  */
/*                                                                           */
/*****************************************************************************/
BOOL CreateBitmapHdcHps( PHDC phdc, PHPS phps)
{
  SIZEL    sizl;
  HDC      hdc;
  HPS      hps;

  hdc = DevOpenDC(hab,OD_MEMORY,"*",0L,NULL,NULLHANDLE);
  if( !hdc)
    return( FALSE);

  sizl.cx = sizl.cy = 0L;
  hps = GpiCreatePS( hab , hdc, &sizl, PU_PELS | GPIA_ASSOC | GPIT_MICRO );

  if( !hps)
    return( FALSE);

  *phdc = hdc;
  *phps = hps;
  return( TRUE);
}
/*------------------------------------------------------------------------*/
VOID CloseImgHdcHps(WINDOWINFO *pwi)
{
   if (pwi->hpsMem)
   {
      GpiDestroyPS(pwi->hpsMem);
      pwi->hpsMem = 0;
   }
   if (hdcBitmapFile)
   {
      DevCloseDC(hdcBitmapFile);
      hdcBitmapFile = 0;
   }
}
/*------------------------------------------------------------------------*/
/* Here we show the details of the selected image.
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY ClrPalDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static pImage pI;
   SWP    swp;                  /* Screen Window Position Holder         */
   ULONG  nColors;
   ULONG  ulStorage[2];    /* To get the vals out of the spins  */
   PVOID  pStorage;        /* idem spinbutton.                  */
   CHAR   szBuffer[10] ;

   switch (msg)
   {
      case WM_INITDLG:
                          /* Centre dialog on the screen */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);

         pI = (pImage)mp2;

         nColors = 1 << ( pI->pbmp2->cBitCount * pI->pbmp2->cPlanes );

         if (nColors)
            WinSetDlgItemText(hwnd,ID_IMGCOLORS,itoa(nColors, szBuffer, 10));
         else
            WinSetDlgItemText(hwnd,ID_IMGCOLORS,"TRUE COLOR");

         WinSetDlgItemText(hwnd,ID_IMGWIDTH,itoa(pI->pbmp2->cx, szBuffer, 10));
         WinSetDlgItemText(hwnd,ID_IMGHEIGHT,itoa(pI->pbmp2->cy, szBuffer, 10));

         /* setup the layer spinbutton */

         WinSendDlgItemMsg( hwnd, ID_SPNIMLAYER, SPBM_SETLIMITS,
                            MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

         WinSendDlgItemMsg( hwnd, ID_SPNIMLAYER, SPBM_SETCURRENTVALUE,
                            MPFROMLONG((LONG)pI->uslayer), NULL);
         return 0;
     case WM_COMMAND:
        switch(LOUSHORT(mp1))
   {
           case DID_OK:
              /*-- get our layer info out of the spin --*/

              pStorage = (PVOID)ulStorage;

              WinSendDlgItemMsg(hwnd,ID_SPNIMLAYER,
                                SPBM_QUERYVALUE,(MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= 1 && ulStorage[0] <= 10 )
                 pI->uslayer = (USHORT)ulStorage[0];
              DlgLoaded = FALSE;
              break;
           case DID_CANCEL:
              DlgLoaded = FALSE;
              WinDismissDlg(hwnd,FALSE);
              return 0;
        }
        WinDismissDlg(hwnd,TRUE);
        return 0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/* ImgShowPalette.                                                        */
/*                                                                        */
/* Description : Shows the details  if exists of the selected image.      */
/*               This function is called after a WM_BUTTON1DBLCLK on the  */
/*               selected image.                                          */
/*------------------------------------------------------------------------*/
void ImgShowPalette(HWND hwnd, pImage pImg )
{
   if (pImg && !DlgLoaded)
   {
      if (WinLoadDlg(HWND_DESKTOP,hwnd,(PFNWP)ClrPalDlgProc,(HMODULE)0,
            ID_IMGDETAIL,(PVOID)pImg))
         DlgLoaded = TRUE;
   }
   else
      WinAlarm(HWND_DESKTOP, WA_WARNING);
   return;
}
/*----------------------------------------------------------------------*/
VOID * ImageSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   POINTLF ptlf;
   pImage  pImg = (pImage)pObj;

   ptlf.x = (float)ptl.x;
   ptlf.x /= pwi->usFormWidth;
   ptlf.y = (float)ptl.y;
   ptlf.y /= pwi->usFormHeight;

   if (pImg->uslayer == pwi->uslayer || pwi->bSelAll)
   {
      if (ptlf.x >= pImg->x && ptlf.x <= ( pImg->x + pImg->cxdest))
      {
         if (ptlf.y >= pImg->y && ptlf.y <= ( pImg->y + pImg->cydest))
         {
            return (void *)pImg;
         }
      }
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/* ImageStretch: stretch the image destination to the new given rectangle */
/*------------------------------------------------------------------------*/
void ImageStretch(POBJECT pObj,PRECTL prclNew,WINDOWINFO *pwi,ULONG ulMsg)
{
   POINTL ptl;
   static RECTL rclOld,r,rTmp;
   pImage pImg = (pImage)pObj;

   switch (ulMsg)
   {
      case WM_BUTTON1DOWN:
         rclOld = *prclNew; /* Get initial size */
         r.xLeft    = pImg->x * pwi->usFormWidth;
         r.xRight   = r.xLeft + (pImg->cxdest * pwi->usFormWidth );
         r.yBottom  = pImg->y * pwi->usFormHeight;
         r.yTop     = r.yBottom + (pImg->cydest * pwi->usFormHeight);
         break;
      case WM_BUTTON1UP:
         if (!pImg->nrpts)
         {
            pImg->x  = (float)prclNew->xLeft;     // Dest Lower LH Corner
            pImg->x /= pwi->usFormWidth;
            pImg->y  = prclNew->yBottom;
            pImg->y /= pwi->usFormHeight;
            pImg->cxdest  = (float)(prclNew->xRight - prclNew->xLeft);
            pImg->cxdest /= pwi->usFormWidth;
            pImg->cydest  = (float)(prclNew->yTop - prclNew->yBottom);
            pImg->cydest /= pwi->usFormHeight;
         }
         else
         {
            rTmp.xLeft  = prclNew->xLeft  - rclOld.xLeft;
            rTmp.xRight = prclNew->xRight - rclOld.xRight;
            rTmp.yTop   = prclNew->yTop   - rclOld.yTop;
            rTmp.yBottom= prclNew->yBottom- rclOld.yBottom;

            rTmp.xLeft  = r.xLeft   + rTmp.xLeft;
            rTmp.xRight = r.xRight  + rTmp.xRight;
            rTmp.yTop   = r.yTop    + rTmp.yTop;
            rTmp.yBottom= r.yBottom + rTmp.yBottom;

            pImg->x  = (float)rTmp.xLeft;     // Dest Lower LH Corner
            pImg->x /= pwi->usFormWidth;
            pImg->y  = rTmp.yBottom;
            pImg->y /= pwi->usFormHeight;
            pImg->cxdest  = (float)(rTmp.xRight - rTmp.xLeft);
            pImg->cxdest /= pwi->usFormWidth;
            pImg->cydest  = (float)(rTmp.yTop - rTmp.yBottom);
            pImg->cydest /= pwi->usFormHeight;
         }
         break;
      case WM_MOUSEMOVE:
         if (!pImg->nrpts)
         {
            GpiSetLineType(pwi->hps,LINETYPE_DOT);
            ptl.x = prclNew->xLeft;
            ptl.y = prclNew->yBottom;
            GpiMove(pwi->hps, &ptl);
            ptl.x = prclNew->xRight;
            ptl.y = prclNew->yTop;
            GpiBox(pwi->hps,DRO_OUTLINE,&ptl,0L,0L);
         }
         else
         {
            rTmp.xLeft  = prclNew->xLeft  - rclOld.xLeft;
            rTmp.xRight = prclNew->xRight - rclOld.xRight;
            rTmp.yTop   = prclNew->yTop   - rclOld.yTop;
            rTmp.yBottom= prclNew->yBottom- rclOld.yBottom;

            rTmp.xLeft  = r.xLeft   + rTmp.xLeft;
            rTmp.xRight = r.xRight  + rTmp.xRight;
            rTmp.yTop   = r.yTop    + rTmp.yTop;
            rTmp.yBottom= r.yBottom + rTmp.yBottom;

            drawPolyLine(pwi->hps,pImg,rTmp); /* Show clippath */
         }
         break;
      default:
         break;
   }
   return;
}
/*------------------------------------------------------------------------*/
/* Move an Image segment.                                                 */
/*------------------------------------------------------------------------*/
void ImgMoveSegment(POBJECT pObject, SHORT dx, SHORT dy, WINDOWINFO *pwi)
{
   float fdx,fdy,fcx,fcy;
   pImage pImg = (pImage)pObject;

   if (!pObject)
      return;

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fdx = (float)dx;
   fdy = (float)dy;

   pImg->x    += (fdx /fcx);
   pImg->y    += (fdy /fcy);
   return;
}
/*------------------------------------------------------------------------*/
void ImgMoveOutLine(POBJECT pObj,WINDOWINFO *pwi, SHORT dx, SHORT dy)
{
   pImage pImg = (pImage)pObj;
   POINTL ptl1,ptl2;
   RECTL  rcl;

   if (!pObj)
      return;
   ptl1.y = pImg->y * pwi->usFormHeight;
   ptl1.x = pImg->x * pwi->usFormWidth;

   ptl1.y += (LONG)dy;
   ptl1.x += (LONG)dx;

   rcl.xLeft   = ptl1.x;
   rcl.yBottom = ptl1.y;

   ptl2.x = ptl1.x + (pImg->cxdest * pwi->usFormWidth );
   ptl2.y = ptl1.y + (pImg->cydest * pwi->usFormHeight);

   rcl.xRight = ptl2.x;
   rcl.yTop   = ptl2.y;

   if (!pImg->nrpts)
   {
      GpiMove(pwi->hps,&ptl1);
      GpiBox(pwi->hps,DRO_OUTLINE,&ptl2,0,0);
   }
   else
   {
      drawPolyLine(pwi->hps,pImg,rcl); /* Show clippath */
   }
}
/*------------------------------------------------------------------------*/
/* ImgSetDropPosition.   (Used when a file is dropped on surface).        */
/*                                                                        */
/* Parameters   :        POBJECT - pointer to image object.               */
/*                       SHORT x - X-Position in pixels!                  */
/*                       SHORT y - Y-Position in pixels!                  */
/*                       WINDOWINFO * - Pointer to the application data.  */
/*------------------------------------------------------------------------*/
void ImgSetDropPosition(POBJECT pObject, SHORT x, SHORT y, WINDOWINFO *pwi)
{
   float fx,fy,fcx,fcy;
   POINTL ptl;

   pImage pImg = (pImage)pObject;

   if (!pObject)
      return;
   /*
   ** Important: The drop position is in desktop coordinate.
   */
   ptl.x = (LONG)x;
   ptl.y = (LONG)y;

   WinMapWindowPoints(HWND_DESKTOP,pwi->hwndClient,&ptl,1);
   /*
   ** Convert from pixels to 0.1 mm ...
   */
   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);

   fcx = (float)pwi->usFormWidth;
   fcy = (float)pwi->usFormHeight;
   fx = (float)ptl.x;
   fy = (float)ptl.y;

   pImg->ptf.x    = (fx /fcx);
   pImg->ptf.y    = (fy /fcy);
   return;
}
/*------------------------------------------------------------------------*/
void ImageCrop(POINTL ptlStart, POINTL ptlEnd, pImage pImg, WINDOWINFO *pwi)
{

   POINTL LowerLeft,UpperRight,ptlTmp;
   ULONG  bytesperline,newdatasize;
   ULONG  cxBytes;
   ULONG  xBytes;
   ULONG  xSource,ySource,cxSource,cySource;
   ULONG  cxdest,cydest; /* We need integraltypes for cropping */
   ULONG  x,y,n,p;
   PBYTE  pNewImageData,ImgDat;
   RECTL  rcl;
   /*
   ** Both points should be in the image, so check it.
   */
   ImgOutLine((POBJECT)pImg, &rcl, pwi);

   if (!WinPtInRect((HAB)0,&rcl,&ptlStart) ||
       !WinPtInRect((HAB)0,&rcl,&ptlEnd))
      return; /* Sorry one of the points outside the image */

   /*
   ** Convert the points to pixels
   */
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlEnd);
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlStart);

    /* First let's define the upper left corner*/

   if (ptlEnd.y > ptlStart.y )
   {
      LowerLeft.y  = ptlStart.y;
      UpperRight.y = ptlEnd.y;
   }
   else
   {
      LowerLeft.y  = ptlEnd.y;
      UpperRight.y = ptlStart.y;
   }

   if (ptlEnd.x < ptlStart.x)
   {
      LowerLeft.x  = ptlEnd.x;
      UpperRight.x = ptlStart.x;
   }
   else
   {
      LowerLeft.x  = ptlStart.x;
      UpperRight.x = ptlEnd.x;
   }

   /*
   ** Check if we are not too small!
   */
   if ((UpperRight.x - LowerLeft.x ) < MINIMUMSIZE)
      UpperRight.x  =   LowerLeft.x + MINIMUMSIZE;

   if ((UpperRight.y - LowerLeft.y ) < MINIMUMSIZE)
      UpperRight.y  =   LowerLeft.y + MINIMUMSIZE;

   ptlTmp.x = (ULONG)(pImg->x * pwi->usFormWidth );
   ptlTmp.y = (ULONG)(pImg->y * pwi->usFormHeight);
   /*
   ** Convert from units to pixels....
   */
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlTmp);

   xSource =  LowerLeft.x - ptlTmp.x;
   ySource =  LowerLeft.y - ptlTmp.y;

   cxSource = (UpperRight.x - LowerLeft.x);
   cySource = (UpperRight.y - LowerLeft.y);

   /*
   ** Get our new coords. First get it into UNITS!!!
   */
   ptlTmp.x = LowerLeft.x;
   ptlTmp.y = LowerLeft.y;
   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlTmp);
   pImg->x = (float)ptlTmp.x;
   pImg->y = (float)ptlTmp.y;
   pImg->y /= pwi->usFormHeight;
   pImg->x /= pwi->usFormWidth;

   /*
   ** Get the destination sizes in pixels.
   */
   ptlTmp.x = (ULONG)(pImg->cxdest * pwi->usFormWidth );
   ptlTmp.y = (ULONG)(pImg->cydest * pwi->usFormHeight);
   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptlTmp);
   cxdest = ptlTmp.x;
   cydest = ptlTmp.y;

   /*
   ** Here we go, after we defined were the bitblit should start within
   ** the imagedata via the xsource,ysource,cxsource and cysource we
   ** start now the real work, namely picking out the imagedata of the
   ** selected part. So finally cxsource etc will be put in the bitmap
   ** header to specify the new imagesize.
   */

   /* if this is a stretched image take it into account */
   if ( pImg->pbmp2->cx != cxdest)
   {
      cxSource = (cxSource * pImg->pbmp2->cx) / cxdest;
      xSource  = (xSource * pImg->pbmp2->cx) / cxdest;
   }

   if (pImg->pbmp2->cy != cydest )
   {
      cySource = (cySource * pImg->pbmp2->cy) / cydest;
      ySource  = (ySource * pImg->pbmp2->cy) / cydest;
   }
   bytesperline = ((cxSource * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
   newdatasize = bytesperline * cySource;

   cxBytes = ((pImg->pbmp2->cx * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
   xBytes = ((xSource  * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;

   ImgDat   = pImg->ImgData;
   pNewImageData = (unsigned char *)calloc(newdatasize,sizeof(char));

   n = 0;
   p = (ySource * cxBytes ) + xBytes;

   for (y=0; y < cySource; y++)
   {
     for (x=0; x < bytesperline; x++)
     {
        *(pNewImageData + n) = *(ImgDat+p+x);
        n++;
     }
     p += (cxBytes);
   }
   free(pImg->ImgData);
   pImg->ImgData  = pNewImageData;
   pImg->pbmp2->cbImage = newdatasize;
   pImg->ImageDataSize  = newdatasize;
   pImg->pbmp2->cx = cxSource;
   pImg->pbmp2->cy = cySource;
   /*
   ** Finally convertion back into UNITS...
   */
   ptlTmp.x = (UpperRight.x - LowerLeft.x);
   ptlTmp.y = (UpperRight.y - LowerLeft.y);
   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlTmp);

   pImg->cxdest  = (float)ptlTmp.x;
   pImg->cydest  = (float)ptlTmp.y;
   pImg->cxdest /= (float)pwi->usFormWidth;
   pImg->cydest /= (float)pwi->usFormHeight;
}
/*========================================================================*/
BOOL ImageCircle(HBITMAP hbm,HPS hps,pImage pImg , WINDOWINFO *pwi)
{
    POINTL ptlCentre;
    ARCPARAMS arcpParms;
    ULONG  Multiplier;
    ULONG  StartAngle;
    ULONG  SweepAngle;
    POINTL  aBitmap[4];
    RECTL  rcl;

    aBitmap[0].x = (LONG)(pImg->x * pwi->usFormWidth ); // Dest Lower LH Corner
    aBitmap[0].y = (LONG)(pImg->y * pwi->usFormHeight);
    aBitmap[1].x = aBitmap[0].x + (long)(pImg->cxdest * pwi->usFormWidth );
    aBitmap[1].y = aBitmap[0].y + (long)(pImg->cydest * pwi->usFormHeight);
    aBitmap[2].x = 0;                  // Source Lower LH Corner
    aBitmap[2].y = 0;
    aBitmap[3].x = pImg->pbmp2->cx-1;   // Source Upper RH Corner
    aBitmap[3].y = pImg->pbmp2->cy-1;


    rcl.xLeft    = (LONG)(pImg->x * pwi->usFormWidth);
    rcl.xRight   = rcl.xLeft  + (LONG)(pImg->cxdest * pwi->usFormWidth);
    rcl.yBottom  = (LONG)(pImg->y * pwi->usFormHeight);
    rcl.yTop     = rcl.yBottom + (LONG)(pImg->cydest * pwi->usFormHeight);

    /*
    ** Calculate the center of the cicular clipping area.
    */

    ptlCentre.x = rcl.xLeft   + (rcl.xRight - rcl.xLeft)/2;
    ptlCentre.y = rcl.yBottom + (rcl.yTop   - rcl.yBottom)/2;

    arcpParms.lP = (rcl.xRight - rcl.xLeft)/ 2; /* radius in x direction*/
    arcpParms.lQ = (rcl.yTop   - rcl.yBottom)/2; /* radius in y direction*/
    arcpParms.lR = 0L;
    arcpParms.lS = 0L;


    Multiplier = 0x0000ff00L;
    StartAngle = 0x00000000L;
    SweepAngle = MAKEFIXED(360,0);
    /*
    ** Make the clipping path for the image
    */

    GpiBeginPath( hps, 1L);  /* define a clip path    */
    GpiSetCurrentPosition(hps,&ptlCentre);
    GpiSetArcParams(hps,&arcpParms);
    GpiPartialArc(hps,&ptlCentre,Multiplier,StartAngle,SweepAngle);
    GpiEndPath(hps);
    GpiSetClipPath(hps,1L,SCP_AND);

         GpiWCBitBlt(hps,  /* presentation space                      */
         hbm,              /* bit-map handle                          */
         4L,               /* four points needed to compress          */
         aBitmap,          /* points for source and target rectangles */
         pImg->lRopCode,
         BBO_IGNORE);      /* discard extra rows and columns          */

   GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */

   return TRUE;
}
/*------------------------------------------------------------------------*/
/*  SaveOtherFormats                                                      */
/*                                                                        */
/*  Description : Tries to get the bitmap data out of the bitmap handle   */
/*                which we get via HBITMAP hbm. We set it into the        */
/*                memory device context via gpisetbitmap (this is         */
/*                probably already done but if so gpi just gives an busy  */
/*                ps).                                                    */
/*                                                                        */
/*                                                                        */
/*  Parameters : HBITMAP hbm - bitmap handle.                             */
/*               HPS hpsMem  - Memory PS.                                 */
/*                                                                        */
/*  Returns:  BOOL : TRUE on success.                                     */
/*------------------------------------------------------------------------*/
static void _System SaveOtherFormats(pimgsavestruct pSaveImg)
{
   BITMAPFILEHEADER2 bfh;   /* includes the bitmapinfoheader2 at the end*/
   PBITMAPINFO2      pbmi;  /* is infoheader2 + pointer to rgb table    */
   PBYTE             pbScan;
   RGB2              *pRGB2;
   ULONG             lBmpDataSize;
   USHORT            sScanLineSize;
   USHORT            nColors;
   BOOL              bRet;
   ULONG             sScans;
   ULONG             i;
   MOD               mod,modNew;
   WINDOWINFO        *pwi = pSaveImg->pwi;
   char *p;

                              // Get bitmap information

   memset(&bfh,0,sizeof(BITMAPFILEHEADER2));

   bfh.usType   = BFT_BMAP;   /* 'BM' */
   bfh.xHotspot = 0;
   bfh.yHotspot = 0;
   bfh.offBits  = 0;
   bfh.bmp2.cbFix = sizeof (BITMAPINFOHEADER2);

   bRet = GpiQueryBitmapInfoHeader(pSaveImg->hbm, &bfh.bmp2);

   if(!bRet)
   {
      free_imgsavestruct (pSaveImg);
      return;
   }
   sScanLineSize = ((bfh.bmp2.cBitCount * bfh.bmp2.cx + 31) / 32) * 4 *
                     bfh.bmp2.cPlanes;

   lBmpDataSize  = (LONG) sScanLineSize * bfh.bmp2.cy ;

   mod.gbm.w    = bfh.bmp2.cx;
   mod.gbm.h    = bfh.bmp2.cy;
   mod.gbm.bpp  = bfh.bmp2.cBitCount;

   nColors = 1 << (bfh.bmp2.cBitCount * bfh.bmp2.cPlanes);

   bfh.usType = BFT_BMAP;   /* 'BM' */
   bfh.cbSize = sizeof(BITMAPFILEHEADER2);
   bfh.xHotspot = 0;
   bfh.yHotspot = 0;
   bfh.offBits  = 0;

   // Create memory DC and PS, and set bitmap in it
   // Allocate memory for BITMAPINFO table & scans

   pbmi = calloc( sizeof(BITMAPINFOHEADER2) + (nColors * sizeof(RGB2)),1);

   memcpy((void *)pbmi,&bfh.bmp2,sizeof(BITMAPINFOHEADER2));

   pbScan = calloc(lBmpDataSize,sizeof(char));

   sScans = GpiQueryBitmapBits (pSaveImg->hpsMem,
                                0L, bfh.bmp2.cy, pbScan, pbmi);

   if(!sScans)
   {
      free_imgsavestruct(pSaveImg);
      free(pbScan);
      free(pbmi);
      WinPostMsg(pwi->hwndMain, UM_READWRITE,(MPARAM)0,(MPARAM)0);
      return;
   }

   pRGB2 = (RGB2 *)0;
   /*
   ** The big JPEG hack!!!
   */
   p = strchr(strupr(pSaveImg->szFilename),'.');
   if (p)
   {
      p++;
      if (*p == 'J' && *(p+1) == 'P' && *(p+2) == 'G')
      {
         writejpegdata(pSaveImg->szFilename,
                       (unsigned char *)pbScan,
                       (BITMAPINFOHEADER2 *)pbmi);

        free_imgsavestruct(pSaveImg);
        free (pbScan);
        free (pbmi);
        WinPostMsg(pwi->hwndMain, UM_READWRITE,(MPARAM)0,(MPARAM)0);
        return;
      }
   } /* EOF JPG !! */

   if (nColors && nColors <=256)
   {
      /*
      ** We do not trust the OS/2 color palette by default.
      ** Here we, if the number of colors in the pbmi struct is
      ** the same as DrawIt has record during image load, we simply
      ** use the palette as was found in the pImg struct.
      */
      if (pSaveImg->pDrwPal && pSaveImg->pDrwPal->nColors == nColors)
      {
         pRGB2 = pSaveImg->pDrwPal->prgb2;
      }
      else
         pRGB2 = &pbmi->argbColor[0];     /* Color definition record */
   }


   if (pRGB2) /* If there is a palette save it */
   {
      for (i=0; i<=nColors; i++)
      {
         mod.gbmrgb[i].r = pRGB2->bRed;
         mod.gbmrgb[i].g = pRGB2->bGreen;
         mod.gbmrgb[i].b = pRGB2->bBlue;
         pRGB2++;

      }
   }
   mod.pbData = pbScan;

   switch (pSaveImg->usRadioBtn)
   {
      case ID_RAD256:
         ModBppMap(&mod,(int)4,(int)0,(int)8,(int)8,(int)8,(int)256,&modNew);
         ModWriteToFile(&modNew,pSaveImg->szFilename, pSaveImg->szOpt);
         break;
      case ID_RAD64:
         ModBppMap(&mod,(int)4,(int)0,(int)8,(int)8,(int)8,(int)64,&modNew);
         ModWriteToFile(&modNew,pSaveImg->szFilename,pSaveImg->szOpt);
         break;
      case ID_RAD16:
         ModBppMap(&mod,(int)1,(int)0,(int)8,(int)8,(int)8,(int)16,&modNew);
         ModWriteToFile(&modNew,pSaveImg->szFilename,pSaveImg->szOpt);
         break;
      default:
         ModWriteToFile(&mod,pSaveImg->szFilename,pSaveImg->szOpt);
         break;
   }
   free_imgsavestruct(pSaveImg);
   free (pbScan);
   free (pbmi);
   WinPostMsg(pwi->hwndMain, UM_READWRITE,(MPARAM)0,(MPARAM)0);
   return;
}

/*------------------------------------------------------------------------*/
/*  SaveBitmap.                                                           */
/*                                                                        */
/*  Description : Tries to get the bitmap data out of the bitmap handle   */
/*                which we get via HBITMAP hbm. We set it into the        */
/*                memory device context via gpisetbitmap (this is         */
/*                probably already done but if so gpi just gives an busy  */
/*                ps).                                                    */
/*                                                                        */
/*                                                                        */
/*  Parameters : HBITMAP hbm - bitmap handle.                             */
/*               HPS hpsMem  - Memory PS.                                 */
/*                                                                        */
/*  Returns:  BOOL : TRUE on success.                                     */
/*------------------------------------------------------------------------*/
static BOOL SaveBitmap (HDC hdcMem, HBITMAP hbm, HPS hpsMem,
                        char * extension, USHORT usRadioBtn,
                        WINDOWINFO *pwi, drw_palette *pDrwPal)
{
   Loadinfo          linf;
   APIRET            rc;
   TID         ThreadID;       /* New thread ID (returned)                   */
   ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
   ULONG       StackSize;      /* Size in bytes of new thread's stack        */
   pimgsavestruct    pSave;
   /*
   ** Try to get a filename...
   */
   linf.dlgflags = FDS_SAVEAS_DIALOG;
   linf.szFileName[0]=0;
   strcpy(linf.szExtension,extension);
   if (!FileGetName(&linf,pwi))
   {
      GpiDeleteBitmap(hbm);
      GpiAssociate(hpsMem, (HDC)0L);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      return FALSE;
   }
   pSave = (pimgsavestruct)calloc(sizeof(imgsavestruct),1);
   pSave->usRadioBtn = usRadioBtn;
   pSave->hbm        = hbm;
   pSave->hpsMem     = hpsMem;
   pSave->hdcMem     = hdcMem;
   pSave->pwi        = pwi;
   pSave->pDrwPal    = pDrwPal;

   strcpy (pSave->szFilename,linf.szFileName);
   strcpy (pSave->szOpt,szOpt);

   WinPostMsg(pwi->hwndMain, UM_READWRITE,(MPARAM)IDS_SAVING,(MPARAM)0);


   ThreadFlags = 0;        /* Indicate that the thread is to */
                           /* be started immediately         */
   StackSize = 14096;      /* Set the size for the new       */
                           /* thread's stack                 */
   rc = DosCreateThread(&ThreadID,(PFNTHREAD)SaveOtherFormats,
                               (ULONG)pSave,
                               ThreadFlags,
                               StackSize);
   if (rc)
   {
      /*
      ** Something went wrong so we should do the cleanup here.
      */
      free(pSave);
      GpiDeleteBitmap(hbm);
      GpiAssociate(hpsMem, (HDC)0L);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      WinPostMsg(pwi->hwndMain,UM_READWRITE,(MPARAM)0,(MPARAM)0);
      return FALSE;
   }
   return TRUE;
}
/*-------------------------------------------------------------------------*/
static BOOL initImageInfo(WINDOWINFO *pwi, WINDOWINFO *pWiImg)
{
   HDC      hdcMem;
   HPS      hpsMem;
   POINTL   ptl;
   char     errbf[150];
   ULONG    ulNr;
   /*
   ** Calc factor between PU_LOMETRIC (DrawIt standard) and the PU_PELS
   ** (pixels) as used in bitmaps
   */
   memset(pWiImg,0,sizeof(WINDOWINFO));

   ptl.x = 10000;
   ptl.y = 10000;
   GpiConvert(pwi->hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);

   pWiImg->uYfactor     = (float)ptl.y;
   pWiImg->uXfactor     = (float)ptl.x;
   pWiImg->uYfactor     /= (float)10000;
   pWiImg->uXfactor     /= (float)10000;
   /*
   ** Extract the info from the windowinfo main struct which
   ** we need to draw the objects.
   */
   pWiImg->usdrawlayer  = pwi->uslayer;
   pWiImg->uslayer      = pwi->uslayer;
   pWiImg->lBackClr     = pwi->lBackClr;
   pWiImg->ulUnits      = PU_PELS;
   pWiImg->yPixels      = pwi->yPixels;
   pWiImg->xPixels      = pwi->xPixels;

   if (!CreateBitmapHdcHps(&hdcMem,&hpsMem))
   {
      ulNr = WinGetLastError(hab);
      sprintf(errbf,"ExporSel2Bmp:\nCould not create PS or DC [%X]",ulNr);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,errbf,
                    "System Error",0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                    MB_ICONEXCLAMATION);
      return FALSE;
   }
   /*
   ** Remember the hps and hdc. This is the presentation space
   ** where in the final drawing is done.
   */
   pWiImg->hps        = hpsMem;
   pWiImg->hdcClient  = hdcMem;
   pWiImg->lcid       = getFontSetID(pWiImg->hps);
   /*
   ** We need an extra memory PS. When an image must be drawn into the
   ** drawing ps bla bla...
   */
   if (!CreateBitmapHdcHps(&hdcMem,&hpsMem))
   {
      GpiAssociate(pWiImg->hps, (HDC)0L);
      GpiDestroyPS(pWiImg->hps);
      DevCloseDC(pWiImg->hdcClient);
      pWiImg->hps       = (HPS)0;
      pWiImg->hdcClient = (HDC)0;
      return FALSE;
   }
   pWiImg->hpsMem    = pwi->hpsMem;
   pWiImg->hdcMem    = hdcMem;
   GpiCreateLogColorTable(pWiImg->hps,LCOL_RESET,LCOLF_RGB, 0, 0, NULL );

   return TRUE;
}
/*-------------------------------------------------------------------------*/
/* Frees the structure. Is called in case of an error during the export    */
/* of a drawing.                                                           */
/*-------------------------------------------------------------------------*/
static void free_imgInfoStruct(WINDOWINFO *pWiImg)
{
   GpiAssociate(pWiImg->hps, (HDC)0L);
   GpiDestroyPS(pWiImg->hps);
   DevCloseDC  (pWiImg->hdcClient);

   GpiAssociate(pWiImg->hpsMem, (HDC)0L);
   GpiDestroyPS(pWiImg->hpsMem);
   DevCloseDC(pWiImg->hdcMem);
   pWiImg->hpsMem    = (HPS)0;
   pWiImg->hdcMem    = (HDC)0;
   pWiImg->hps       = (HPS)0;
   pWiImg->hdcClient = (HDC)0;
}
/*-------------------------------------------------------------------------*/
/*  Name       : ExportSel2Bmp                                             */
/*                                                                         */
/*  Description: If a bounding rectangle has been defined, create a screen */
/*               compatible device context and a screen compatible bit map.*/
/*               Select the bit map into the DC, create a                  */
/*               suitably sized GPI PS and associate it with the DC.       */
/*               BitBlt that part of the                                   */
/*               screen defined by the rectangle into the new bit map.     */
/*               Destroy all other resources but return the bit map handle */
/*                                                                         */
/*  Parameters : WINDOWINFO * - pointer to windowinfo struct.              */
/*               RECTL *        pointer to the selection rectangle.        */
/*                              rectangle in PIXELS!! (objboundingrect)    */
/*               char  *        pointer to a null terminated string which  */
/*                              contains the file extension. (".PCX")      */
/*                              If ".NONE" use local statics szExt and     */
/*                              usIdColors instead of the parameters ext   */
/*                              and usId.                                  */
/*               USHORT       - Contains the #color conversion.            */
/*                                                                         */
/*  Return     : TRUE on success.                                          */
/*-------------------------------------------------------------------------*/
BOOL ExportSel2Bmp(WINDOWINFO *pwi,RECTL *prcl,char *ext, USHORT usId)
{
   HBITMAP  bmap;
   ULONG    ulNr;
   BITMAPINFOHEADER2 bmpMemory;
   SIZEL    sizlWork;
   BOOL     bRet = TRUE;
   static   WINDOWINFO wiImg; /* Used on export to bitmap format..*/
   POINTL   ptl;
   char     errbf[150];

   if (!initImageInfo(pwi,&wiImg))
      return FALSE;
   /*
   ** Extract the info from the windowinfo main struct which
   ** we need to draw the objects.
   */
   wiImg.usFormHeight = (pwi->usHeight * pwi->yPixels)/10000;
   wiImg.usFormWidth  = (pwi->usWidth  * pwi->xPixels)/10000;
   wiImg.usHeight     = wiImg.usFormHeight;
   wiImg.usWidth      = wiImg.usFormWidth;
   /*
   ** It is a bounding rectangle, so we only want what is inside.
   ** Make it an unzoomed size if zooming is done.
   */
   if (pwi->usHeight != pwi->usFormHeight)
   {
      prcl->xRight = (prcl->xRight * pwi->usWidth)  / pwi->usFormWidth;
      prcl->xLeft  = (prcl->xLeft  * pwi->usWidth)  / pwi->usFormWidth;
      prcl->yTop   = (prcl->yTop   * pwi->usHeight) / pwi->usFormHeight;
      prcl->yBottom= (prcl->yBottom* pwi->usHeight) / pwi->usFormHeight;
   }
   sizlWork.cx = (prcl->xRight - prcl->xLeft);
   sizlWork.cy = (prcl->yTop - prcl->yBottom);

   memset (&bmpMemory,0, sizeof(BITMAPINFOHEADER2));

   bmpMemory.cbFix           = sizeof(BITMAPINFOHEADER2);
   bmpMemory.cx              = (USHORT)sizlWork.cx;
   bmpMemory.cy              = (USHORT)sizlWork.cy;
   bmpMemory.cPlanes         = 1L;
   bmpMemory.cBitCount       = 24L;
   bmpMemory.ulCompression   = BCA_UNCOMP;
   bmpMemory.cxResolution    = 0;
   bmpMemory.cyResolution    = 0;
   bmpMemory.cclrUsed        = 0;
   bmpMemory.cclrImportant   = 0;
   bmpMemory.usUnits         = BRU_METRIC;
   bmpMemory.usRecording     = BRA_BOTTOMUP;
   bmpMemory.usRendering     = BRH_NOTHALFTONED;
   bmpMemory.cSize1          = 0;
   bmpMemory.cSize2          = 0;
   bmpMemory.ulColorEncoding = BCE_RGB;
   bmpMemory.ulIdentifier    = 0;

   bmap = GpiCreateBitmap(wiImg.hps,(PBITMAPINFOHEADER2)&bmpMemory,
                          0L,(PBYTE)NULL,(PBITMAPINFO2)NULL);
   if (!bmap)
   {
      ulNr = WinGetLastError(hab);
      sprintf(errbf,"ExporSel2Bmp:\nCan't create bitmap Error [%X]",ulNr);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,errbf,
                    "System Error",0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                    MB_ICONEXCLAMATION);
      free_imgInfoStruct(&wiImg);
      return FALSE;
   }

   GpiSetBitmap(wiImg.hps,bmap);
   /*
   ** First off all draw a square and fill this with the
   ** background color of the drawing area.
   */
   ptl.x = 0;
   ptl.y = 0;
   GpiMove(wiImg.hps,&ptl);
   ptl.x = sizlWork.cx;
   ptl.y = sizlWork.cy;

   GpiSetPattern(wiImg.hps,PATSYM_SOLID);
   GpiSetMix    (wiImg.hps,FM_OVERPAINT);
   GpiSetColor  (wiImg.hps,pwi->lBackClr);
   GpiBox       (wiImg.hps,DRO_FILL,&ptl,0L,0L);
   /*
   ** Start drawing the selected elements in our
   ** memoryPS..
   */
   wiImg.ulUnits = PU_PELS; /* Pixels so convert if needed */
   wiImg.yPixels = pwi->yPixels;
   wiImg.xPixels = pwi->xPixels;

   GpiQueryModelTransformMatrix(wiImg.hps,9L,&wiImg.matOrg);

   for ( wiImg.usdrawlayer = MINLAYER; wiImg.usdrawlayer <= MAXLAYER; wiImg.usdrawlayer++)
      ObjDrawSelected((POBJECT)0,wiImg.hps,&wiImg,prcl,TRUE);
   /*
   ** Save the bitmap to disk.
   */
   if (!stricmp(ext,".NON"))
   {
      /*
      ** Use string and #colors filled in by the dialog procedure
      ** Export2ImgDlgProc.
      ** SaveBitmap destroys the hps and hdc, so we leave it as it is.
      */
      bRet = SaveBitmap (wiImg.hdcClient,bmap,wiImg.hps,szExt,usIdColor,pwi,NULL);
   }
   else
      bRet = SaveBitmap (wiImg.hdcClient,bmap,wiImg.hps,ext,usId,pwi,NULL);

   GpiAssociate(wiImg.hpsMem, (HDC)0L);
   GpiDestroyPS(wiImg.hpsMem);
   DevCloseDC(wiImg.hdcMem);
   return bRet;
}  /*  end of GetBitmap  */
/*-------------------------------------------------------------------------*/
/*  Name       : Export2Bmp                                                */
/*                                                                         */
/*  Description: Exports the whole drawing to a bitmap file.               */
/*                                                                         */
/*  Parameters : WINDOWINFO * - pointer to windowinfo struct.              */
/*                                                                         */
/*  Pre cond     : char *szExt - contains the file extension.              */
/*                 USHORT usIdColor - contains the #colors for destination */
/*                                   image.                                */
/*  Return     : TRUE on success.                                          */
/*-------------------------------------------------------------------------*/
BOOL Export2Bmp(WINDOWINFO *pwi )
{
   HBITMAP  bmap;
   ULONG    ulNr;
   BITMAPINFOHEADER2 bmpMemory;
   SIZEL    sizlWork;
   POINTL   ptl;
   BOOL     bRet = TRUE;
   char     errbf[125];
   static   WINDOWINFO wiImg;         /* Used on export to bitmap format.. */

   if (!initImageInfo(pwi,&wiImg))
      return FALSE;
   /*
   ** Extract the info from the windowinfo main struct which
   ** we need to draw the objects.
   */
   wiImg.usFormHeight = sizlBitmap.cy;
   wiImg.usFormWidth  = sizlBitmap.cx;

   sizlWork.cx = sizlBitmap.cx;
   sizlWork.cy = sizlBitmap.cy;

   memset (&bmpMemory,0, sizeof(BITMAPINFOHEADER2));
   bmpMemory.cbFix           = sizeof(bmpMemory);
   bmpMemory.cx              = (USHORT)sizlWork.cx;
   bmpMemory.cy              = (USHORT)sizlWork.cy;
   bmpMemory.cPlanes         = 1L;
   bmpMemory.cBitCount       = 24L;
   bmpMemory.ulCompression   = BCA_UNCOMP;
   bmpMemory.cxResolution    = 0;
   bmpMemory.cyResolution    = 0;
   bmpMemory.cclrUsed        = 0;
   bmpMemory.cclrImportant   = 0;
   bmpMemory.usUnits         = BRU_METRIC;
   bmpMemory.usRecording     = BRA_BOTTOMUP;
   bmpMemory.usRendering     = BRH_NOTHALFTONED;
   bmpMemory.cSize1          = 0;
   bmpMemory.cSize2          = 0;
   bmpMemory.ulColorEncoding = BCE_RGB;
   bmpMemory.ulIdentifier    = 0;

   bmap = GpiCreateBitmap(wiImg.hps,(PBITMAPINFOHEADER2)&bmpMemory,
                          0L,(PBYTE)NULL,(PBITMAPINFO2)NULL);
   if (!bmap)
   {
      ulNr = WinGetLastError(hab);
      sprintf(errbf,"Export2Bmp: Can't create bitmap Error [%X]",ulNr);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,errbf,
                    "System Error",0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                    MB_ICONEXCLAMATION);
      free_imgInfoStruct(&wiImg);
      return FALSE;
   }

   GpiSetBitmap(wiImg.hps,bmap);
    /*
   ** First off all draw a square and fill this with the
   ** background color of the drawing area.
   */
   ptl.x = 0;
   ptl.y = 0;
   GpiMove(wiImg.hps,&ptl);
   ptl.x = sizlWork.cx;
   ptl.y = sizlWork.cy;
   GpiSetPattern(wiImg.hps,PATSYM_SOLID);
   GpiSetMix    (wiImg.hps, FM_OVERPAINT);
   GpiSetColor  (wiImg.hps,pwi->lBackClr);
   GpiBox       (wiImg.hps,DRO_FILL,&ptl,0L,0L);
   /*
   ** Start drawing the ALL elements in our
   ** memoryPS..
   */
   for ( wiImg.usdrawlayer = MINLAYER; wiImg.usdrawlayer <= MAXLAYER; wiImg.usdrawlayer++)
      ObjDrawSegment(wiImg.hps,&wiImg,(POBJECT)0,(RECTL *)0);
   /*
   ** Save the bitmap to disk.
   ** Use string and #colors filled in by the dialog procedure
   ** Export2ImgDlgProc.
   */
   bRet = SaveBitmap (wiImg.hdcClient,bmap,wiImg.hps,szExt,usIdColor,pwi,NULL);
   /*
   ** Cleanup and go....
   */
   GpiAssociate(wiImg.hpsMem, (HDC)0L);
   GpiDestroyPS(wiImg.hpsMem);
   DevCloseDC  (wiImg.hdcMem);
   return bRet;
}  /*  end of GetBitmap  */
/*-----------------------------------------------[ private ]--------------*/
/*  onControl.                                                            */
/*                                                                        */
/*  Description : Handles the control message for the image dialog.       */
/*------------------------------------------------------------------------*/
static MPARAM onControl(HWND hwnd, MPARAM mp1, MPARAM mp2, BOOL bTrueColor)
{
   int i;

   for (i = 0; i < OPTIONTAB; i++)
   {
       if (colorOptions[i].bAllowed && !colorOptions[i].bEnabled)
       {
          WinEnableWindow(WinWindowFromID(hwnd,colorOptions[i].usIdColor),TRUE);
          colorOptions[i].bEnabled = TRUE;
       }
   }

   switch (LOUSHORT(mp1))
   {
      case ID_RADTRUE:
      case ID_RAD256:
      case ID_RAD64:
      case ID_RAD16:
         usIdColor = LOUSHORT(mp1);
         break;
      case ID_RADPCX:
         if (!bTrueColor)
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),TRUE );
         strcpy(szExt,".PCX");
         strcpy(szOpt," ");
         break;
      case ID_RADOS2:
         if (!bTrueColor)
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),TRUE );
         strcpy(szExt,".BMP");
         strcpy(szOpt,"2.0");
         break;
      case ID_RADWIN:
         if (!bTrueColor)
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),TRUE );
         strcpy(szExt,".BMP");
         strcpy(szOpt,"win");
         break;
      case ID_RADGIF:
         strcpy(szExt,".GIF");
         strcpy(szOpt," ");
         /*
         ** No true color support so disable the true color
         ** radio button. Uncheck it and check the 256 color
         ** button. Default setting for GIF.
         */
         if (usIdColor == ID_RADTRUE)
         {
            WinSendDlgItemMsg(hwnd,ID_RADTRUE,BM_SETCHECK,(MPARAM)0,(MPARAM)0);
            WinSendDlgItemMsg(hwnd,ID_RAD256,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
            usIdColor = ID_RAD256;
         }
         colorOptions[0].bEnabled = FALSE;
         WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),FALSE );
         break;
      case ID_RADTIF:
         if (!bTrueColor)
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),TRUE );
         strcpy(szExt,".TIF");
         strcpy(szOpt," ");
         break;
      case ID_RADTGA:
         if (!bTrueColor)
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),TRUE );
         strcpy(szExt,".TGA");
         strcpy(szOpt," ");
         break;
      case ID_RADJPG:
         strcpy(szExt,".JPG");
         strcpy(szOpt," ");
         for ( i = 0; i < OPTIONTAB; i++)
         {
            WinEnableWindow(WinWindowFromID(hwnd,colorOptions[i].usIdColor),FALSE);
            colorOptions[i].bEnabled = FALSE;
         }
         break;
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Export2ImgDlgProc.                                                    */
/*                                                                        */
/*  Description : Dialog procedure for the image export function.         */
/*                Gives the user the possibility to save the selected area*/
/*                or the total drawing in one of the given formats.       */
/*                This procedure sets the global var's which values are   */
/*                finally used in the Export2Selbmp or Eport2Bmp.         */
/*                                                                        */
/*  Notice      : When the mp2 parameter has is zero at initdlg than the  */
/*                spinbuttons will be disabled. The spinbuttons are use   */
/*                to define the picture size when the total drawing is    */
/*                exported.                                               */
/*                                                                        */
/*  Parameters  : hwnd   - window handle of the dialog.                   */
/*                ulMsg  - Message id.                                    */
/*                mp1    - Message parameter 1.                           */
/*                mp2    - Message parameter 2.                           */
/*                                                                        */
/* Post         : char *szExt - contains the file extension.              */
/*                USHORT usIdColor - contains the #colors for destination */
/*                                   image.                               */
/*                SIZEL sizlBitmap will be filled by the spinbuttons.     */
/*                                                                        */
/* Returns      : MRESULT - Message result.                               */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY Export2ImgDlgProc(HWND hwnd,ULONG ulmsg,MPARAM mp1,MPARAM mp2)
{
   SWP    swp;                  /* Screen Window Position Holder         */
   static WINDOWINFO *pwi;      /* Used for setting the spinbuttons.     */
   ULONG ulStorage[2];          /* To get the vals out of the spins      */
   PVOID pStorage;
   int   i;

   switch (ulmsg)
   {
      case WM_INITDLG:  /* Center dialog on the screen */
         pwi       = (WINDOWINFO *)mp2;
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CXSCREEN) - swp.cx) / 2),
             ((WinQuerySysValue(HWND_DESKTOP,   SV_CYSCREEN) - swp.cy) / 2),
             0, 0, SWP_MOVE);
         /*
         ** Set default radiobutton for bitmap format to PCX.
         ** And true color.
         */
         WinSendDlgItemMsg(hwnd,ID_RADTRUE,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
         strcpy(szOpt,"2.0");
         strcpy(szExt,".BMP");  /* fill in the local global..OS/2  default   */
         usIdColor = ID_RADTRUE; /* True color is the default.               */
         for (i = 0; i < OPTIONTAB; i++)
         {
            colorOptions[i].bAllowed = TRUE;
            colorOptions[i].bEnabled = TRUE;
         }
         /*
         ** Initialize our spinbuttons, with the maxvals etc
         */
         if (pwi)
         {
            /*
            ** Export total drawing.
            */
            WinSendDlgItemMsg(hwnd,ID_SPINBMPCX,SPBM_SETLIMITS,
                              MPFROMLONG(BMPMAX_CX), MPFROMLONG(BMPMIN_CX));

            WinSendDlgItemMsg(hwnd,ID_SPINBMPCY,SPBM_SETLIMITS,
                              MPFROMLONG(BMPMAX_CY), MPFROMLONG(BMPMIN_CY));

            WinSendDlgItemMsg(hwnd,ID_SPINBMPCX,SPBM_SETCURRENTVALUE,
                              MPFROMLONG((LONG)(pwi->usWidth/4)),NULL);

            WinSendDlgItemMsg(hwnd,ID_SPINBMPCY,SPBM_SETCURRENTVALUE,
                              MPFROMLONG((LONG)(pwi->usHeight/4)),NULL);
         }
         else
         {
            WinEnableWindow(WinWindowFromID(hwnd,ID_SPINBMPCY),FALSE);
            WinEnableWindow(WinWindowFromID(hwnd,ID_SPINBMPCX),FALSE);
         }
         return 0;

     case WM_COMMAND:
        switch(LOUSHORT(mp1))
   {
           case DID_OK:
              if (pwi)
              {
                 /*
                 ** Get the values from the spinbuttons
                 */
                 pStorage = (PVOID)ulStorage;

                 WinSendDlgItemMsg(hwnd,ID_SPINBMPCX,SPBM_QUERYVALUE,
                                   (MPARAM)(pStorage),MPFROM2SHORT(0,0));

                 sizlBitmap.cx  = ulStorage[0];

                 WinSendDlgItemMsg(hwnd,ID_SPINBMPCY,SPBM_QUERYVALUE,
                                   (MPARAM)(pStorage),MPFROM2SHORT(0,0));

                 sizlBitmap.cy  = ulStorage[0];
              }
              WinDismissDlg(hwnd,DID_OK);
              break;
           case DID_CANCEL:
              WinDismissDlg(hwnd,DID_CANCEL);
              break;
           case DID_HELP:
              ShowDlgHelp(hwnd);
              return 0;
        }
        return (MRESULT)0;

     case WM_CONTROL:
        return onControl(hwnd,mp1,mp2,TRUE);
   }
   return(WinDefDlgProc(hwnd, ulmsg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  SaveSelImgDlgProc.                                                    */
/*                                                                        */
/*  Description : This dialog procedure uses the same dialog as the       */
/*                Export2ImgDlgProc. But this procedure is used for saving*/
/*                a selected image and will not use the global locals     */
/*                Further the possibilities of choosing the number of     */
/*                colors in the image to save will be limited by the      */
/*                # colors in the selected image.                         */
/*                                                                        */
/*  Parameters  : hwnd   - window handle of the dialog.                   */
/*                ulMsg  - Message id.                                    */
/*                mp1    - Message parameter 1.                           */
/*                mp2    - Message parameter 2.                           */
/*                                                                        */
/* Returns      : MRESULT - Message result.                               */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY SaveSelImgDlgProc(HWND hwnd,ULONG ulmsg,MPARAM mp1,MPARAM mp2)
{
   SWP    swp;                 /* Screen Window Position Holder         */
   static USHORT usColors;     /* Number of colors in the selected img  */
   char   szWindowTitle[MAXNAMEL];
   static pImage pImg;         /* Pointer to the image info.            */
   int    i;

   switch (ulmsg)
   {
      case WM_INITDLG:  /* Center dialog on the screen */
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
               ((WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN) - swp.cx) / 2),
               ((WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - swp.cy) / 2),
                         0, 0, SWP_MOVE);

         for (i = 0; i < OPTIONTAB; i++)
         {
            colorOptions[i].bAllowed = FALSE;
            colorOptions[i].bEnabled = FALSE;
         }
         /*
         ** Set default radiobutton for bitmap format to IBM OS/2
         ** And true color.
         */
         if (WinLoadString(hab, (HMODULE)0,IDS_SAVESELIMGAS,
                          MAXNAMEL,(PSZ)szWindowTitle))
         WinSetWindowText(hwnd,(PSZ)szWindowTitle);

         strcpy(szExt,".BMP");   /* fill in the local global.... os2 default */
         usIdColor = ID_RADTRUE; /* True color is the default.               */
         strcpy(szOpt,"2.0");    /* OS/2 2.0 bitmap format used as def.      */

         pImg = (pImage)mp2;     /* Load image info...       */
         usIdColor = 0;          /* No conversion by default */

         if (pImg->pbmp2->cBitCount != 24)
         {
            usColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes);
            /*
            ** Disable true color radio button.
            */
            WinEnableWindow(WinWindowFromID(hwnd,ID_RADTRUE),FALSE);
            switch (usColors)
            {
              case 256:
                 WinSendDlgItemMsg(hwnd,ID_RAD256,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
                 colorOptions[1].bAllowed = TRUE;
                 colorOptions[2].bAllowed = TRUE;
                 colorOptions[3].bAllowed = TRUE;
                 usIdColor = ID_RAD256;
                 break;
              case 64:
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD256),FALSE);
                 WinSendDlgItemMsg(hwnd,ID_RAD64,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
                 usIdColor = ID_RAD64;
                 colorOptions[2].bAllowed = TRUE;
                 colorOptions[3].bAllowed = TRUE;
                 break;
              case 16:
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD256),FALSE);
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD64),FALSE);
                 WinSendDlgItemMsg(hwnd,ID_RAD16,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
                 usIdColor = ID_RAD16;
                 colorOptions[3].bAllowed = TRUE;
                 break;
              default:
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD256),FALSE);
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD64),FALSE);
                 WinEnableWindow(WinWindowFromID(hwnd,ID_RAD16),FALSE);
                 usIdColor = ID_RADTRUE;
                 break;
           }
        }
        else
        {
           usColors  = 0; /* true color stuff selected */
           usIdColor = ID_RADTRUE;
           WinSendDlgItemMsg(hwnd,ID_RADTRUE,BM_SETCHECK,(MPARAM)1,(MPARAM)0);

           for (i = 0; i < OPTIONTAB; i++)
           {
              colorOptions[i].bAllowed = TRUE;
              colorOptions[i].bEnabled = TRUE;
           }

        }
        return (MRESULT)0;

     case WM_COMMAND:
        switch(LOUSHORT(mp1))
        {
           case DID_OK:
              WinDismissDlg(hwnd,DID_OK);
              break;
           case DID_CANCEL:
              WinDismissDlg(hwnd,DID_CANCEL);
              break;
           case DID_HELP:
              ShowDlgHelp(hwnd);
              return 0;
        }
        return (MRESULT)0;

     case WM_CONTROL:
        return onControl(hwnd,mp1,mp2,(BOOL)usColors);
   }
   return(WinDefDlgProc(hwnd, ulmsg, mp1, mp2));
}
/*-----------------------------------------------[ private ]--------------*/
/*  ImgSaveOtherFormats                                                   */
/*                                                                        */
/*  Description : Used to save the selected as is seen on screen.         */
/*                Including stretching.                                   */
/*                Activated from image menu "Save Image"                  */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ImgSaveOtherFormats(HWND hwnd, POBJECT pObj,WINDOWINFO *pwi)
{
   HBITMAP   bmap;
   HBITMAP   hbm;
   HDC       hdcMem;
   HPS       hpsMem;
   POINTL    aBitmap[4];
   BITMAPINFOHEADER2 bmpMemory;
   SIZEL     sizlWork;
   POINTL    ptl;
   BOOL      bRet = TRUE;
   char      errbf[125];
   BYTE      *pByte;
   PRGB2     pRGB2,pPal;
   pImage    pImg = (pImage)pObj;
   int       nColors;
   drw_palette *pDrwPal;

   if (!pObj)
      return FALSE;

   if ( WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)SaveSelImgDlgProc,
                  (HMODULE)0,IDB_SAVESELIMG,(VOID *)pImg) != DID_OK)
      return FALSE;

   pRGB2 = NULL;
   pPal  = NULL;
   pDrwPal= NULL;

   CreateBitmapHdcHps(&hdcMem,&hpsMem);

   GpiCreateLogColorTable ( hpsMem,LCOL_RESET,LCOLF_RGB, 0, 0, NULL );

   ptl.x = (LONG)(pImg->cxdest  * pwi->usWidth);
   ptl.y = (LONG)(pImg->cydest  * pwi->usHeight);

   GpiConvert(pwi->hps,CVTC_DEFAULTPAGE,CVTC_DEVICE,1,&ptl);
   sizlWork.cx = (LONG)ptl.x;
   sizlWork.cy = (LONG)ptl.y;

   memset (&bmpMemory,0, sizeof(BITMAPINFOHEADER2));


   if (pImg->pbmp2->cBitCount == 24)
   {
      bmpMemory.cbFix           = sizeof(bmpMemory);
      bmpMemory.cx              = (USHORT)sizlWork.cx;
      bmpMemory.cy              = (USHORT)sizlWork.cy;
      bmpMemory.cPlanes         = 1L;
      bmpMemory.cBitCount       = 24L;
      bmpMemory.ulCompression   = BCA_UNCOMP;
      bmpMemory.cxResolution    = 0;
      bmpMemory.cyResolution    = 0;
      bmpMemory.cclrUsed        = 0;
      bmpMemory.cclrImportant   = 0;
      bmpMemory.usUnits         = BRU_METRIC;
      bmpMemory.usRecording     = BRA_BOTTOMUP;
      bmpMemory.usRendering     = BRH_NOTHALFTONED;
      bmpMemory.cSize1          = 0;
      bmpMemory.cSize2          = 0;
      bmpMemory.ulColorEncoding = BCE_RGB;
      bmpMemory.ulIdentifier    = 0;
   }
   else
   {
      memcpy(&bmpMemory,pImg->pbmp2,sizeof(BITMAPINFOHEADER2));

      bmpMemory.cx              = (USHORT)sizlWork.cx;
      bmpMemory.cy              = (USHORT)sizlWork.cy;

      pByte = (BYTE *)pImg->pbmp2;
      pRGB2 = (RGB2 *)(pByte + sizeof(BITMAPINFOHEADER2));

      nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );
/*
      if (nColors <= 256)
      {
         pPal   = (RGB2 *)calloc(nColors,sizeof(RGB2));
         pDrwPal = (drw_palette *)calloc(sizeof(drw_palette),sizeof(char));
         memcpy(pPal,pRGB2,(nColors * sizeof(RGB2)) );
         pDrwPal->nColors = nColors;
         pDrwPal->prgb2   = pPal;
      }
*/
   }

   bmap = GpiCreateBitmap(hpsMem,(PBITMAPINFOHEADER2)&bmpMemory,
                          0L,(PBYTE)NULL,(PBITMAPINFO2)NULL);

   if (!bmap)
   {
      ULONG ulNr;

      ulNr = WinGetLastError(hab);
      sprintf(errbf,"MakeBmpDataFromPimg: Can't create bitmap Error [%X]",ulNr);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,errbf,
                    "System Error",0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                    MB_ICONEXCLAMATION);
      GpiAssociate(hpsMem, (HDC)0L);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      return FALSE;
   }

   GpiSetBitmap(hpsMem,bmap);

   aBitmap[0].x = (LONG)0;
   aBitmap[0].y = (LONG)0;
   aBitmap[1].x = (long)sizlWork.cx;
   aBitmap[1].y = (long)sizlWork.cy;
   aBitmap[2].x = 0;                  // Source Lower LH Corner
   aBitmap[2].y = 0;
   aBitmap[3].x = pImg->pbmp2->cx-1;   // Source Upper RH Corner
   aBitmap[3].y = pImg->pbmp2->cy-1;

   hbm = GpiCreateBitmap(pwi->hpsMem,     /* presentation-space handle */
                         pImg->pbmp2,     /* address of structure for format data */
                         CBM_INIT,        /*options      */
                         pImg->ImgData,   /* bitmap data */
                         (PBITMAPINFO2)pImg->pbmp2); /* address of structure for color and format */


   GpiWCBitBlt(hpsMem,hbm,4L,aBitmap,pImg->lRopCode,BBO_IGNORE);
   /*
   ** Save the bitmap to disk.
   ** Use string and #colors filled in by the dialog procedure
   ** Export2ImgDlgProc.
   */
   bRet = SaveBitmap (hdcMem,bmap,hpsMem,szExt,usIdColor,pwi,pDrwPal);

   return bRet;
}  /*  end of GetBitmap  */
/*-----------------------------------------------[ public ]---------------*/
/*  setCircular.                                                          */
/*                                                                        */
/*------------------------------------------------------------------------*/
void ImgSetCircular(POBJECT pObj, BOOL bCirc)
{
   pImage    pImg = (pImage)pObj;
   pImg->ImgCir = bCirc;
}
/*-----------------------------------------------[ public ]---------------*/
/*  ImgIsCircular.                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL ImgIsCircular(POBJECT pObj)
{
   pImage    pImg = (pImage)pObj;
   return pImg->ImgCir;
}
/*-----------------------------------------------[ public ]---------------*/
/*  imgAddClipPath.                                                       */
/*                                                                        */
/*------------------------------------------------------------------------*/
void imgAddClipPath(POBJECT pObjImg,POBJECT pObjSpline,WINDOWINFO *pwi)
{
   RECTL    rcl;
   ULONG    ulPoints;
   ULONG    ulType;
   void    *pvPoints;
   pImage   pImg = (pImage)pObjImg;

   if (!pObjImg || !pObjSpline)
      return;

   ImgOutLine(pObjImg,&rcl,pwi);

   if ( (pvPoints = splMakeClipPath(pObjSpline,pwi,rcl,&ulPoints,&ulType)) != NULL)
   {
       pImg->nrpts  = ulPoints;             /* Number of points of clipping path */
       pImg->pptl   = (POINTLF *)pvPoints;  /* Points to the chain of points...  */
       pImg->ulClipPathType = ulType;
   }
}
/*-----------------------------------------------[ public ]---------------*/
BOOL imgDelClipPath(POBJECT pObj)
{
   pImage   pImg = (pImage)pObj;

   if (!pObj)
      return FALSE;

   if (pObj->usClass != CLS_IMG)
      return FALSE;

   if (!pImg->nrpts)
      return FALSE;

   pImg->nrpts  = 0L;
   if (pImg->pptl)
      free((void *)pImg->pptl);
   pImg->pptl = NULL;
   return TRUE;
}
/*-----------------------------------------------[ public ]---------------*/
BOOL imgHasClipPath(POBJECT pObj)
{
   pImage pImg = (pImage)pObj;

   if (!pObj)
      return FALSE;

   if (pImg->nrpts && pImg->pptl)
      return TRUE;

   return FALSE;
}
/*-----------------------------------------------[ private ]----------------*/
static void drawPolyLine(HPS hps, pImage pImg, RECTL rcl)
{
   PPOINTLF pptl;
   ULONG    lWidth;
   ULONG    lHeight;
   int      i;
   POINTL   ptl[500],ptlStart;

   if (!pImg->nrpts)
      return;

   lWidth  = rcl.xRight - rcl.xLeft;
   lHeight = rcl.yTop   - rcl.yBottom;

   pptl = pImg->pptl;

   ptl[0].x = (LONG)(pptl->x * lWidth );
   ptl[0].y = (LONG)(pptl->y * lHeight);
   ptl[0].x +=(LONG)rcl.xLeft;
   ptl[0].y +=(LONG)rcl.yBottom;

   ptlStart = ptl[0];

   GpiMove(hps, &ptl[0]);
   /*
   ** Use our points buffer defined at the top of
   ** this file to put in the calculated points...
   */
   for ( i = 0; i < pImg->nrpts-1; i++)
   {
      pptl++;
      ptl[i].x =(LONG)(pptl->x * lWidth );
      ptl[i].y =(LONG)(pptl->y * lHeight);
      ptl[i].x += rcl.xLeft;
      ptl[i].y += rcl.yBottom;
   }
   ptl[i++]= ptlStart;

   if (pImg->ulClipPathType == SPL_SPLINE)
      GpiPolyFillet(hps,(LONG)i,ptl);
   else
      GpiPolyLine(hps,(LONG)i,ptl);

   GpiCloseFigure(hps);
}
/*--------------------------------------------------------------------------*/
static BOOL drawImgInClippath(HBITMAP hbm,HPS hps,pImage pImg,WINDOWINFO *pwi)
{
    POINTL  aBitmap[4];
    RECTL   rcl;

    aBitmap[0].x = (LONG)(pImg->x * pwi->usFormWidth ); // Dest Lower LH Corner
    aBitmap[0].y = (LONG)(pImg->y * pwi->usFormHeight);
    aBitmap[1].x = aBitmap[0].x + (long)(pImg->cxdest * pwi->usFormWidth );
    aBitmap[1].y = aBitmap[0].y + (long)(pImg->cydest * pwi->usFormHeight);
    aBitmap[2].x = 0;                  // Source Lower LH Corner
    aBitmap[2].y = 0;
    aBitmap[3].x = pImg->pbmp2->cx-1;   // Source Upper RH Corner
    aBitmap[3].y = pImg->pbmp2->cy-1;


    rcl.xLeft    = (LONG)(pImg->x * pwi->usFormWidth);
    rcl.xRight   = rcl.xLeft  + (LONG)(pImg->cxdest * pwi->usFormWidth);
    rcl.yBottom  = (LONG)(pImg->y * pwi->usFormHeight);
    rcl.yTop     = rcl.yBottom + (LONG)(pImg->cydest * pwi->usFormHeight);
    /*
    ** Make the clipping path for the image
    */
    GpiBeginPath( hps, 1L);  /* define a clip path    */
    drawPolyLine(hps,pImg,rcl);
    GpiEndPath(hps);
    GpiSetClipPath(hps,1L,SCP_AND);

         GpiWCBitBlt(hps,  /* presentation space                      */
         hbm,              /* bit-map handle                          */
         4L,               /* four points needed to compress          */
         aBitmap,          /* points for source and target rectangles */
         pImg->lRopCode,
         BBO_IGNORE);      /* discard extra rows and columns          */

   GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
   return TRUE;
}
/*-----------------------------------------------[ public ]----------------*/
void imgChangeRopCode(POBJECT pObj,LONG lRop)
{
   pImage pImg = (pImage)pObj;

   pImg->lRopCode = lRop;

}
/*-----------------------------------------------[ public ]----------------*/
void imgSetMenuCheckMarks(HWND hwndMenu,POBJECT pObj)
{
   USHORT usMenuId;
   pImage pImg = (pImage)pObj;

   for (usMenuId = IDM_SRCCOPY; usMenuId <= IDM_PATONE; usMenuId++)
      WinSendMsg(hwndMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));

   switch(pImg->lRopCode)
   {
      case ROP_SRCPAINT:    usMenuId = IDM_SRCPAINT;     break;
      case ROP_SRCAND:      usMenuId = IDM_SRCAND;       break;
      case ROP_SRCINVERT:   usMenuId = IDM_SRCINVERT;    break;
      case ROP_SRCERASE:    usMenuId = IDM_SRCERACE;     break;
      case ROP_NOTSRCCOPY:  usMenuId = IDM_NOTSRCCOPY;   break;
      case ROP_NOTSRCERASE: usMenuId = IDM_NOTSRCERASE;  break;
      case ROP_MERGECOPY:   usMenuId = IDM_MERGECOPY;    break;
      case ROP_PATCOPY:     usMenuId = IDM_PATCOPY;      break;
      case ROP_PATPAINT:    usMenuId = IDM_PATPAINT;     break;
      case ROP_PATINVERT:   usMenuId = IDM_PATINVERT;    break;
      case ROP_DSTINVERT:   usMenuId = IDM_PATDSTINVERT; break;
      case ROP_ZERO:        usMenuId = IDM_PATZERO;      break;
      case ROP_ONE:         usMenuId = IDM_PATONE;       break;
      default:              usMenuId = IDM_SRCCOPY;      break;
   }
   WinSendMsg(hwndMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
