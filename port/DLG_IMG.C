/*------------------------------------------------------------------------*/
/*  Name: DLG_IMG.C                                                       */
/*                                                                        */
/*  Description : Displaying of the images from the images chain.         */
/*                deletion,creation and maintanance of the bitmapImage.   */
/*                                                                        */
/* Functions:                                                             */
/*   BlackWhite              : Converts color image into B&W image.       */
/*   OpenImgSegment          : Opens/creates an imagesegment              */
/*   CloseImgSegment         : Closes the segment.                        */
/*   DeleteImage             : Deletes image from the chain               */
/*   DeleteAllImages         : Deletes all images                         */
/*   ExportSel2Bmp           : Exports the selected objects to a BMP!     */
/*   ReadBitmap              : Reads the file and finds out the format    */
/*   DrawImgSegment          : draws the segments into a PS.              */
/*   CreateBitmapHdcHps      : Creates HDC and HPS - OD_MEMORY            */
/*   CloseImgHdcHps          : Deletes HDC and HPS - OD_MEMORY            */
/*   IsOnImageCorner         : Is mouse pointer on one of the corners     */
/*   ImageSelect             : Is there an image under my mousepointer?   */
/*   ImageCrop               : Crops the selected image                   */
/*   ImgShowPalette          : Shows the palette in a dialog after DBLCLK */
/*   ImageCircle             : Blits image in circular image              */
/*   ImageStretch            : Stretches the image to the given rectangle */
/*   FlipImgHorz             : Mirror imagedata over X-axis.              */
/*   FlipImgVert             : Mirror imagedata over Y-axis.              */
/*   ReadBmpFile             : Reads Windows,OS/2 BMP-files.              */
/*   ReadPcxFile             : Reads Z-soft pcx formatted files.          */
/*   pcx_getb                : Gets byte out of PCX-file.                 */
/*   FilePutImage            : Puts one Imagesegment in a file.           */
/*   FileGetImage            : Loads one imagesegment from a file         */
/*   ImgInvArea              : Calculates the RECTL of the selected img   */
/*   ImgRotateColor          : Rotates colors in the palette (REMAME!!!!) */
/*   SaveImage               : Save selected image as OS/2 2.x BMP file.  */
/*   ImgFilterDlgProc        : Colorfiltering dlgproc for selected bmp.   */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_img.h"
#include "drwutl.h"
#include "drwtif.h"
#include "resource.h"

#include "\gbmsrc\standard.h"

#include "\gbmsrc\gbm\gbm.h"

#include "\gbmsrc\gbm\gbmerr.h"
#include "\gbmsrc\gbm\gbmht.h"
#include "\gbmsrc\gbm\gbmhist.h"
#include "\gbmsrc\gbm\gbmmcut.h"



typedef int MOD_ERR;
#define	MOD_ERR_OK	((MOD_ERR) 0)
#define	MOD_ERR_MEM	((MOD_ERR) 1)
#define	MOD_ERR_OPEN	((MOD_ERR) 2)
#define	MOD_ERR_CREATE	((MOD_ERR) 3)
#define	MOD_ERR_SUPPORT	((MOD_ERR) 4)
#define	MOD_ERR_HDC	((MOD_ERR) 5)
#define	MOD_ERR_HPS	((MOD_ERR) 6)
#define	MOD_ERR_HBITMAP	((MOD_ERR) 7)
#define	MOD_ERR_HMF	((MOD_ERR) 8)
#define	MOD_ERR_CLIP	((MOD_ERR) 9)
#define	MOD_ERR_GBM(rc)	((MOD_ERR) 0x8000+(rc))

typedef struct
	{
	GBM gbm;
	GBMRGB gbmrgb[0x100];
	BYTE *pbData;
	} MOD;



#define DEFAULT_DIR    "\\OS2\\BITMAP"
#define MAXREADWRITEBUF    500
#define MINIMUMSIZE        20     /* After cropping minimum size in pixels */

#define  ROUNDTODWORD(b)   (((b+31)/32)*4)
extern void ErrorBox(char *Str1, char *Str2);          /*Dialog.c*/
pImage ImgBase,ImgHead;

HDC     hdcBitmapFile;
DEVOPENSTRUC       dop;
BOOL    DlgLoaded = FALSE;
static  Loadinfo li;

typedef  struct _fhelper
   {
       USHORT   ustype;
       USHORT   usHalftone;
       USHORT   nColors;
       USHORT   uslayer;
       BOOL     ImgDeleted;
       BOOL     ImgMultiSel;
       BOOL     ImgCir;
       ULONG      cx;      
       ULONG      cy;      
       ULONG      cxdest;      
       ULONG      cydest;      
       ULONG      x;
       ULONG      y;
       ULONG      selcx;      
       ULONG      selcy;      
       ULONG      selx;
       ULONG      sely;
       unsigned char * ImgData;
       ULONG ImageDataSize;
       ULONG filesize;
   } IMGFILEHEADER;


CHAR szFullFilePath[CCHMAXPATH] = "";       /* For the filedialog */

/*------------------------------------------------------------------------*/
/* FilePutImage.                                                          */
/*------------------------------------------------------------------------*/
int FilePutImageData(pLoadinfo pli, pImage pImg )
{
   int         cbWritten;
   USHORT      usColors;
   PBYTE       data;
   PRGB2       pRGB2;
   ULONG       ulWritten = 0;
   ULONG       ulRestBytes;

   PBYTE       pImgData;

   if (!pImg)
      return 0;

   if (!pImg->cx || !pImg->cy)
      return 0;


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

   pImgData = pImg->ImgData;

   if (pImg->pbmp2->cbImage > MAXREADWRITEBUF)
   {
      ulRestBytes = pImg->pbmp2->cbImage % MAXREADWRITEBUF;

      do {

         cbWritten = write(pli->handle,(PVOID)pImgData,MAXREADWRITEBUF);
         ulWritten += cbWritten;
         pImgData  += cbWritten;
         pImg->pbmp2->cbImage -= cbWritten;
         if (cbWritten <= 0)
            return -1;

      } while (pImg->pbmp2->cbImage > MAXREADWRITEBUF);

      cbWritten = write(pli->handle,(PVOID)pImgData,ulRestBytes);

      if (cbWritten <= 0)
         return -1;

   }
   else
   {
      cbWritten = write(pli->handle,(PVOID)pImg->ImgData,pImg->pbmp2->cbImage);
      if (cbWritten <= 0)
         return -1;
   }
   return cbWritten;
}
/*------------------------------------------------------------------------*/
/* FileGetImage.                                                          */
/*------------------------------------------------------------------------*/
int FileGetImageData(pLoadinfo pli, pImage pImg)
{
   ULONG       ulDataSize;
   USHORT      usColors;
   PBYTE       data;
   int         cbRead;    /* Bytes read from file    */
   PRGB2       pRGB2;
   BITMAPINFOHEADER2 bmp2;

   /*
   ** Start loading the bitmapheader including the color lookup table
   ** if the number of colors <= 256, else we assume its a truecolor one
   */
   if (!pImg->cx || !pImg->cy)
      return 0;

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
      return 0L;
   }
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

int ModMakeHBITMAP(MOD *mod, pImage pImg)
{
   USHORT usColors;
   ULONG  bytesperline,newdatasize;


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
      RGB2  *pRGB2;
      unsigned char *data;

      data = (unsigned char *)pImg->pbmp2;
      pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));

      for ( i = 0; i < usColors; i++ )
      {
         pRGB2->bRed      = mod->gbmrgb[i].r;
         pRGB2->bGreen    = mod->gbmrgb[i].g;
         pRGB2->bBlue     = mod->gbmrgb[i].b;
         pRGB2->fcOptions = 0;
         pRGB2++;
      }

   }
   pImg->ImgData = mod->pbData;

   bytesperline = ((pImg->pbmp2->cx * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
   newdatasize = bytesperline * pImg->pbmp2->cy;
   pImg->pbmp2->cbImage = newdatasize;
   pImg->ImageDataSize  = newdatasize;
}
/*------------------------------------------------------------------------*/
/* OpenImageSegment.                                                      */
/*                                                                        */
/*------------------------------------------------------------------------*/
pImage OpenImgSegment()
{
  IMG    Img;
  PIMAGE Pmg;
  char   buf[350];
  char   *pDot;
  pImage pImg;
  MOD   modNew;
  int   mrc;

  Pmg = &Img;

  li.dlgflags = FDS_OPEN_DIALOG;

  strcpy(li.szExtension,"*.*");        /* Set default extension */


  if (FileGetName(&li))
  {
    pDot = strchr(li.szFileName,'.');
    if (pDot)
    {
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

          WinMessageBox(HWND_DESKTOP,
                        hwndClient,
                        buf, 
                        "Not a bitmap file", 
                        0,
                        MB_OK | MB_APPLMODAL | MB_MOVEABLE | 
                        MB_ICONQUESTION);
          pImg = (pImage)0;
          return pImg;
       }
    
    
       mrc = ModCreateFromFile(li.szFileName, (char *)"",&modNew);
    }
    if (mrc == MOD_ERR_OK)
    {
      pImg = (pImage)pObjCreate(CLS_IMG);
      ModMakeHBITMAP(&modNew,pImg);
    }
    else
      pImg = (pImage)0;
  }
  else
      pImg = (pImage)0;

  return pImg;
}
/*------------------------------------------------------------------------*/
/* CloseImageSegment.                                                     */
/*------------------------------------------------------------------------*/
pImage CloseImgSegment(pImage pImg,WINDOWINFO *pwi)
{
   POINTL ptldummy;
   RECTL  rcl;

   ptldummy.x = 0;
   ptldummy.y = 0;

   WinQueryWindowRect(pwi->hwndClient, &rcl);
   
   ptldummy.y = rcl.yTop - pImg->pbmp2->cy;

   if (pImg)
   {
      pImg->y = PYVALUE(ptldummy.y);
      pImg->x = PXVALUE(ptldummy.x); 
      pImg->sely = -1;
      pImg->selx = -1;
      pImg->ImgDeleted = FALSE;
      pImg->ImgCir     = FALSE;
      pImg->ustype = CLS_IMG;
      pImg->uslayer = pwi->uslayer;
      pImg->usHalftone = 0;
      /*
      ** Set the initial display parameters.
      */
      pImg->cydest = pImg->cy  = pImg->pbmp2->cy;
      pImg->cxdest = pImg->cx  = pImg->pbmp2->cx;
      pImg->selcx = pImg->cx+1;
      pImg->selcy = pImg->cy+1;
//      pImg->ulFilter = 0x00808080;
   }
   return pImg;
}
/*------------------------------------------------------------------------*/
/* Flip image Vertical.....                                               */
/*------------------------------------------------------------------------*/
void FlipImgVert(pImage pImg)
{
   PBYTE ImgDat;
   BYTE  data;
   ULONG i,p,ScanLine;
   ULONG bytesperline,half;

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
void FlipImgHorz(pImage pImg)
{
   PBYTE ImgDat;
   BYTE  data;
   ULONG i,datasize;

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
/*  Name: BlackWhite.                                                     */
/*                                                                        */
/*  Description : Converts an color bitmap image into an Black & White    */
/*                image.                                                  */
/*                                                                        */
/*  Parameters : pImage pImg: Pointer to the selected image.              */
/*                                                                        */
/*  Returns:  BOOL : TRUE on success.                                     */
/*------------------------------------------------------------------------*/
BOOL BlackWhite(pImage pImg)
{
   PBYTE data,p;
   ULONG i,nColors,ulDataSize;
   PRGB2 pRGB2;
   BYTE  MaxColor;
   
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
/* Image outline is the exact outline in window coords.                   */
/*------------------------------------------------------------------------*/
void ImgOutLine(pImage pImg, RECTL *rcl, WINDOWINFO *pwi)
{
   if (!pImg)
      return;

   rcl->xLeft    = WXVALUE(pImg->x);
   rcl->xRight   = rcl->xLeft + pImg->cxdest;
   rcl->yBottom  = WYVALUE(pImg->y);
   rcl->yTop     = rcl->yBottom + pImg->cydest;
}
/*------------------------------------------------------------------------*/
/* ImgInvArea()                                                           */
/*------------------------------------------------------------------------*/
void ImgInvArea(pImage pImg, RECTL *rcl, WINDOWINFO *pwi)
{
   if (!pImg)
      return;

   ImgOutLine(pImg,rcl,pwi);
   /*
   ** Add extra space for selection handles.
   */
   rcl->xLeft    -= 10;
   rcl->xRight   += 10;
   rcl->yBottom  -= 10;
   rcl->yTop     += 10;

}
/******************************************************************************
*
*  Name        : ReadBitmap
*
*  Description :
*
* Get the bitmap from disk.
*
******************************************************************************/

BOOL ReadBitmap(pLoadinfo pli, PIMAGE pImg)
{
   APIRET     rc;                        /* API return code */
   LONG   curpos,length;
   char buf[40];
   PBYTE  pFileData = NULL;             /* beginning of bitmap file data */
   USHORT *ftype;
   PIMAGE pimage;
   unsigned char *pBits;               /* For getting the first byte     */
   BOOL   DONE=FALSE;

   do
   {
      curpos = ftell(pli->fp);
      fseek(pli->fp,0L,SEEK_END);
      length = ftell(pli->fp);
      fseek(pli->fp,curpos,SEEK_SET);
       
      rc =
      DosAllocMem((PPVOID) &pFileData,
                  (ULONG)  length,
                  PAG_READ | PAG_WRITE | PAG_COMMIT);
      if (rc)
      {
         sprintf(buf,"DosAllocMem rc[%d]",rc);
         DispError(hab,hwndClient);
         break;
      }

      pImg->pFileData = pFileData;
      pImg->filesize = length;

      fread(pImg->pFileData,length,1,pli->fp);
      fclose( pli->fp);

      pBits = (unsigned char *)pImg->pFileData;

      if (*pBits == 10)                  /* PCX ??? */
      {
         pimage = (PIMAGE)ReadPcxFile(pImg);
         DONE=TRUE;
      }
      else                             /* BMP ??? */
      {
         ftype = (USHORT *)pImg->pFileData;
         switch (*ftype)
         {
            case BFT_BITMAPARRAY:
            case BFT_BMAP:
               pimage = (PIMAGE)ReadBmpFile(pImg);
               pimage->status = 0;
               DONE=TRUE;
               break;
         }
      }
     
      if (!DONE)
      {
         pimage= (PIMAGE)ReadGifFile(pImg);
         if (!pimage->status)
            DONE = TRUE;
      }

      if (!DONE)
      {
         /*
         ** Let's try the TIFF.
         */
         pBits = strchr(pli->szFileName,'.');
         pimage->status = 0;
         if (pBits)
         {

            pBits++; /* Jump over dot in filename. */
            if (stricmp("TIF",pBits))
               pimage->status = IMG_WRONGFORMAT;
         }
         else
            pimage->status = IMG_WRONGFORMAT;

         if (!pimage->status && (TIFLoad(pImg,pli->szFileName)) == 0)
         {
             pimage = pImg;
             pimage->status = 0;
             DONE = TRUE;
          }
          else
             pimage->status = IMG_WRONGFORMAT;
      }

      if (pimage->status)
      {
         switch (pimage->status)
         {
            case IMG_WRONGFORMAT:
               sprintf(buf,"Image wrong format\nNot a TIF,GIF,BMP or PCX file");
               break;
            case IMG_OUTOFMEMORY:
               sprintf(buf,"Image out of memory");
               break;
            case IMG_NOTIMPLEMENTED:
               sprintf(buf,"Imagetype not implemented");
               break;
            default:
               sprintf(buf,"Unknown error during load");
               break;
         }
         WinMessageBox(HWND_DESKTOP,
                       hwndClient,
                       buf, 
                       "Image load error", 
                       0,
                       MB_OK | MB_APPLMODAL | MB_MOVEABLE | 
                       MB_ICONQUESTION);

         if (pimage)
            DosFreeMem(&pImg->pFileData);
         pimage = NULL;
      }
      if (pimage)
         DosFreeMem(&pImg->pFileData);
      else
         return FALSE;

      return TRUE;            /* function successful */
   } while (FALSE);           /* fall through loop first time */
   fclose( pli->fp);
   return FALSE;              /* function failed */
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

VOID DrawImgSegment(HPS hps,WINDOWINFO *pwi, pImage pImg, RECTL *prcl)
{
   HBITMAP hbm;
   ULONG   cScansRet;          /* number of scan lines in bitmap (cy) */
   POINTL  aBitmap[4];
   RECTL   rcl,rclDest;
   LONG    oldPattern;         /* Remember the pattern settings.  */

   oldPattern = GpiQueryPattern(pwi->hps);
   GpiSetPattern(pwi->hps,PATSYM_SOLID);

   if (pwi->usdrawlayer == pImg->uslayer)
   {
      if (prcl)
      {
         ImgOutLine(pImg,&rcl,pwi);
         if (!WinIntersectRect(hab,&rclDest,prcl,&rcl) || rcl.yTop < 0)
         return;
      }

      /*
      ** Since the printer and the layer dialog
      ** etc may not change the selection square
      ** position we check on the cyclient size.
      ** Only the main window is using this,
      ** and may change this value.
      */

      if (pwi->cyClient)
      {
         pImg->selx  = WXVALUE(pImg->x);
         pImg->sely  = WYVALUE(pImg->y);
      }
      rcl.xLeft    = WXVALUE(pImg->x);
      rcl.xRight   = WXVALUE(pImg->x) + pImg->cxdest;
      rcl.yBottom  = WYVALUE(pImg->y);
      rcl.yTop     = rcl.yBottom + pImg->cydest;

      hbm = GpiCreateBitmap(pwi->hpsMem,   /* presentation-space handle */
                            pImg->pbmp2,     /* address of structure for format data */
                            CBM_INIT,        /*options      */
                            pImg->ImgData,   /* bitmap data */
                            (PBITMAPINFO2)pImg->pbmp2); /* address of structure for color and format */


      if (pImg->ImgCir)
      {
         ImageCircle(hbm,hps,pImg,pwi);
      }
      else
      {
         printf("Draw image x = [%d] y = [%d] \n",pImg->x,pImg->y);
         printf("Draw image cx= [%d] cy= [%d] \n",pImg->cxdest,pImg->cydest);

         aBitmap[0].x = WXVALUE(pImg->x);    // Dest Lower LH Corner
         aBitmap[0].y = WYVALUE(pImg->y);
         aBitmap[1].x = WXVALUE(pImg->x) + pImg->cxdest;   // Dest Upper RH Corner
         aBitmap[1].y = pImg->cydest + WYVALUE(pImg->y);
         aBitmap[2].x = 0;                  // Source Lower LH Corner
         aBitmap[2].y = 0;
         aBitmap[3].x = pImg->pbmp2->cx-1;   // Source Upper RH Corner
         aBitmap[3].y = pImg->pbmp2->cy-1;   


         aBitmap[0].x *= pwi->uXfactor;
         aBitmap[0].y *= pwi->uYfactor;
         aBitmap[1].x *= pwi->uXfactor;
         aBitmap[1].y *= pwi->uYfactor;

//         GpiBitBlt(hps,pwi->hpsMem,4, aBitmap, ROP_SRCCOPY,BBO_IGNORE); 
//         GpiSetBitmap(pwi->hpsMem, (HBITMAP)0);

         GpiWCBitBlt(hps,  /* presentation space                      */
         hbm,              /* bit-map handle                          */
         4L,               /* four points needed to compress          */
         aBitmap,          /* points for source and target rectangles */
         ROP_SRCCOPY,      /* copy source replacing target            */
         BBO_IGNORE);      /* discard extra rows and columns          */


   
         GpiDeleteBitmap(hbm);
      }
   } /*drawlayer*/
   GpiSetPattern(pwi->hps,oldPattern);
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

  hdc = DevOpenDC( hab, OD_MEMORY, "*", 3L, (PDEVOPENDATA)&dop, NULLHANDLE);
  if( !hdc)
    return( FALSE);

  sizl.cx = sizl.cy = 1L;
  hps = GpiCreatePS( hab , hdc, &sizl, PU_PELS | GPIA_ASSOC | GPIT_NORMAL );
    
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
/*  Name: IsOnImageCorner                                                 */
/*                                                                        */
/*  Description : Tels wether the mouspointer resides on one of the       */
/*                corners of the selected images.                         */
/*                                                                        */
/*  Concepts : Sets the right pointer. There are two different ponters,   */
/*             SPTR_SIZENESW = Upward sloping double-headed arrow,        */
/*             SPTR_SIZENWSE = Downward sloping double-headed arrow.      */
/*                                                                        */
/*  API's : WinSetPointer                                                 */
/*          WinQuerySysPointer                                            */
/*                                                                        */
/*  Parameters : USHORT X,Y (mouse position)                              */
/*               Pimage : pointer to the current selected image.          */
/*               wi     : pointer to the windowstruct used all over the   */
/*                        place.                                          */
/*                                                                        */
/*  Returns:  TRUE : if the pointer is on a corner.                       */
/*            FALSE: if the pointer is not on a corner.                   */
/*            USHORT: Corner number.                                      */
/*                                                                        */
/*           1ÚÄÄÄÄÄÄÄ¿2                                                  */
/*            ³       ³                                                   */
/*           4ÀÄÄÄÄÄÄÄÙ3                                                  */
/*------------------------------------------------------------------------*/
BOOL IsOnImageCorner( LONG x, LONG y, pImage pImg,WINDOWINFO *pwi, USHORT *cnr)
{

   POINTL mousptl;  /* in the future a parameter! */

   mousptl.x = PXVALUE(x);
   mousptl.y = PYVALUE(y);

   /*
   ** right bottom
   */
   if (mousptl.x >= ((pImg->x + pImg->cxdest)-10) && 
       mousptl.x <= ( pImg->x + pImg->cxdest)     &&
       mousptl.y <= pImg->y +10 && 
       mousptl.y >= pImg->y )
   {
      /*
      ** If the element is set to deleted it may not be
      ** selected!
      */
      if (!pImg->ImgDeleted)
      {
         *cnr = 3;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                          SPTR_SIZENWSE,FALSE));
         return TRUE;
      }
   }

   /*
   ** right top
   */

   else   if (mousptl.x >= ((pImg->x + pImg->cxdest)-10) && 
       mousptl.x <= ( pImg->x + pImg->cxdest)     &&
       mousptl.y <= (pImg->y + pImg->cydest) && 
       mousptl.y >= (pImg->y + pImg->cydest-10))
   {
      if (!pImg->ImgDeleted)
      {
         *cnr = 2;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                          SPTR_SIZENESW,FALSE));
         return TRUE;
      }
   }
   else if (mousptl.x >= pImg->x && 
            mousptl.x <= ( pImg->x + 10) &&
            mousptl.y <= pImg->y +10 && 
            mousptl.y >= pImg->y )
   {
      /*
      ** If the element is set to deleted it may not be
      ** selected!
      */
      if (!pImg->ImgDeleted)
      {
         *cnr = 4;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                          SPTR_SIZENESW,FALSE));
         return TRUE;
      }
   }
   else if (mousptl.x >= pImg->x && 
            mousptl.x <= ( pImg->x + 10) &&
            mousptl.y <= pImg->y + pImg->cydest && 
            mousptl.y >= pImg->y + pImg->cydest-10 )
   {
      /*
      ** If the element is set to deleted it may not be
      ** selected!
      */
      if (!pImg->ImgDeleted)
      {
         *cnr = 1;
         WinSetPointer(HWND_DESKTOP,
                       WinQuerySysPointer(HWND_DESKTOP,
                                          SPTR_SIZENWSE,FALSE));
         return TRUE;
      }
   }
   return FALSE;
}
/*------------------------------------------------------------------------*/
/* ImgShowPalette.                                                        */
/*                                                                        */
/* Description : Shows the palette  if exists of the selected image.      */
/*               This function is called after a WM_BUTTON1DBLCLK on the  */
/*               selected image.                                          */
/*               Starts up the appropriate dialog, eg with 16 or 256      */
/*               entries in the valueset, or gives a warning if the       */
/*               image does not contain any palette.                      */
/*------------------------------------------------------------------------*/
void ImgShowPalette(HWND hwnd, pImage pImg )
{
   ULONG  nColors;

   nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

   if (nColors == 256 )
      WinLoadDlg(HWND_DESKTOP,
                 hwnd,
                 (PFNWP)ClrPalDlgProc,(HMODULE)0,
	         1400,
                 (PVOID)pImg);

   else if (nColors == 16 )
      WinLoadDlg(HWND_DESKTOP,
                 hwnd,
                 (PFNWP)ClrPalDlgProc,(HMODULE)0,
	         1500,
                 (PVOID)pImg);
   else if (nColors > 256)
      WinLoadDlg(HWND_DESKTOP,
                 hwnd,
                 (PFNWP)ClrPalDlgProc,(HMODULE)0,
	         1600,
                 (PVOID)pImg);
   return;
} 
/*------------------------------------------------------------------------*/
/* Here we show the colorpalette of the image in a valueset.              */
/*------------------------------------------------------------------------*/

MRESULT EXPENTRY ClrPalDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   static pImage pI;
   static ULONG  ulOffset=0;
   SWP    swp;                  /* Screen Window Position Holder         */
   USHORT idxCol;
   USHORT idxRow;
   USHORT x,y;
   PBYTE  data;
   ULONG  nColors,ulClr;
   ULONG  idItem;
   ULONG ulStorage[2];    /* To get the vals out of the spins  */
   PVOID pStorage;        /* idem spinbutton.                  */
   PRGB2  pRGB2;

   switch (msg)
   {
      case WM_INITDLG:
		       /* Centre dialog	on the screen			*/
         WinQueryWindowPos(hwnd, (PSWP)&swp);
         WinSetWindowPos(hwnd, HWND_TOP,
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CXSCREEN) - swp.cx) / 2),
		       ((WinQuerySysValue(HWND_DESKTOP,	SV_CYSCREEN) - swp.cy) / 2),
		       0, 0, SWP_MOVE);

         pI = (pImage)mp2;

         data = (unsigned char *)pI->pbmp2;
         pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));
         nColors = 1 << ( pI->pbmp2->cBitCount * pI->pbmp2->cPlanes );

         if (nColors == 256 )
         {
            x = 16;
            y = 16;
            ulOffset = 0;
         }
         else if (nColors == 16)
         {
            x = 4;
            y = 4;
            ulOffset=100;
         }
         else 
         {
            x = 0;
            y = 0;
            ulOffset=200;
         }
         if (x && y)
         {
            for (idxCol=1; idxCol<= x; idxCol++)
               for (idxRow=1; idxRow<= y; idxRow++)
               {
                  ulClr = (ULONG) pRGB2->bRed    << 16 |  
                          (ULONG) pRGB2->bGreen  << 8  |
                          (ULONG) pRGB2->bBlue; 
                  WinSendDlgItemMsg(hwnd,(1401 + ulOffset),
                                    VM_SETITEM,
                                    MPFROM2SHORT(idxRow,idxCol),
                                    MPFROMLONG(ulClr));
                  pRGB2++;
               }
         }
           /* setup the layer spinbutton */

        WinSendDlgItemMsg( hwnd, (1407 + ulOffset), SPBM_SETLIMITS,
                           MPFROMLONG(MAXLAYER), MPFROMLONG(MINLAYER));

        WinSendDlgItemMsg( hwnd, (1407 + ulOffset), SPBM_SETCURRENTVALUE,
                           MPFROMLONG((LONG)pI->uslayer), NULL);

        WinSendDlgItemMsg(hwnd,(1402+ulOffset+pI->usHalftone),BM_SETCHECK,
                          MPFROMSHORT(1),(MPARAM)0);

        return 0;
     case WM_COMMAND:
        switch(LOUSHORT(mp1))
	{
           case DID_OK:
              /*-- get our layer info out of the spin --*/

              pStorage = (PVOID)ulStorage;

              WinSendDlgItemMsg(hwnd,
                                (1407 + ulOffset),
                                SPBM_QUERYVALUE,
                                (MPARAM)(pStorage),
                                MPFROM2SHORT(0,0));

              if (ulStorage[0] >= 1 && ulStorage[0] <= 10 )
                 pI->uslayer = (USHORT)ulStorage[0];
              break;
           case DID_CANCEL:
              WinDismissDlg(hwnd,FALSE);
              return 0;
        }
        WinDismissDlg(hwnd,TRUE);
        return 0;
     case WM_CONTROL:
        if (HIUSHORT(mp1) == VN_SELECT)
        {
	   idItem = (ULONG)WinSendDlgItemMsg(hwnd,
                                             (1401+ ulOffset),
                                             VM_QUERYSELECTEDITEM, 
                                             NULL,
                                             NULL);

	   ulClr = (ULONG)WinSendDlgItemMsg(hwnd,
                                            (1401+ ulOffset),
                                            VM_QUERYITEM,
                                            MPFROMLONG(idItem),
                                            NULL);
        }
        return 0; 
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*----------------------------------------------------------------------*/
VOID * ImageSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj)
{
   pImage  pImg = (pImage)pObj;

   ptl.x = PXVALUE(ptl.x);
   ptl.y = PYVALUE(ptl.y);

   if (pImg->uslayer == pwi->uslayer || pwi->bSelAll)
   {
      if (!pImg->ImgDeleted)
      {
         if (ptl.x >= pImg->x && ptl.x <= ( pImg->x + pImg->cxdest))
         {
            if (ptl.y >= pImg->y && ptl.y <= ( pImg->y + pImg->cydest))
	    {
	          return (void *)pImg;
            }
         }
      }
   }
   return (void *)0;
}
/*------------------------------------------------------------------------*/
/* ImageStretch: stretch the image destination to the new given rectangle */
/*------------------------------------------------------------------------*/
void ImageStretch(pImage pImg,PRECTL prclNew,WINDOWINFO *pwi)
{

   pImg->x = PXVALUE(prclNew->xLeft);     // Dest Lower LH Corner
   pImg->y = PYVALUE(prclNew->yBottom);
   pImg->cxdest = (prclNew->xRight - prclNew->xLeft);
   pImg->cydest = (prclNew->yTop - prclNew->yBottom);

   return;
}
/*------------------------------------------------------------------------*/
void ImageCrop(POINTL ptlStart, POINTL ptlEnd, pImage pImg, WINDOWINFO *pwi)
{

   POINTL LowerLeft,UpperRight;
   ULONG  bytesperline,newdatasize;
   ULONG  cxBytes;
   ULONG  xBytes;
   ULONG  xSource,ySource,cxSource,cySource;
   ULONG  x,y,n,p,restbits;
   PBYTE  pNewImageData,ImgDat;
   
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


   xSource =  LowerLeft.x - WXVALUE(pImg->x);
   ySource =  LowerLeft.y - WYVALUE(pImg->y);

   cxSource = (UpperRight.x - LowerLeft.x);
   cySource = (UpperRight.y - LowerLeft.y);


   pImg->selx = LowerLeft.x;
   pImg->sely = LowerLeft.y;
   pImg->y    = PYVALUE(pImg->sely);
   pImg->x    = PXVALUE(pImg->selx);

   /* 
   ** Here we go, after we defined were the bitblit should start within
   ** the imagedata via the xsource,ysource,cxsource and cysource we
   ** start now the real work, namely picking out the imagedata of the 
   ** selected part. So finally cxsource etc will be put in the bitmap
   ** header to specify the new imagesize.
   */

   /* if this is a stretched image take it into account */
   if ( pImg->pbmp2->cx != pImg->cxdest)
   {
      cxSource = (cxSource * pImg->pbmp2->cx) / pImg->cxdest;
      xSource  = (xSource * pImg->pbmp2->cx) / pImg->cxdest;
   }
  
   if (pImg->pbmp2->cy != pImg->cydest )
   {
      cySource = (cySource * pImg->pbmp2->cy) / pImg->cydest;
      ySource  = (ySource * pImg->pbmp2->cy) / pImg->cydest;
   }

   restbits =   cxSource % 4;
   if (restbits)
   {
      cxSource = cxSource + ( 4 - restbits);
   }

   restbits =   cySource % 4;
   if (restbits)
   {
      cySource = cySource + ( 4 - restbits);
   }


   bytesperline = ((cxSource * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
   newdatasize = bytesperline * cySource;

   cxBytes = ((pImg->pbmp2->cx      * pImg->pbmp2->cBitCount + 31)/32) * 4 * pImg->pbmp2->cPlanes;
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
   pImg->cy        = 0;
   pImg->cx        = 0;

   pImg->cy  = pImg->pbmp2->cy;
   pImg->cx  = pImg->pbmp2->cx;

   pImg->cxdest   = (UpperRight.x - LowerLeft.x);
   pImg->cydest   = (UpperRight.y - LowerLeft.y);

   pImg->selcx = pImg->cx+1;
   pImg->selcy = pImg->cy+1;

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

    aBitmap[0].x = WXVALUE(pImg->x);          // Dest Lower LH Corner
    aBitmap[0].y = WYVALUE(pImg->y);
    aBitmap[1].x = WXVALUE(pImg->x) +pImg->cxdest;   // Dest Upper RH Corner
    aBitmap[1].y = pImg->cydest + WYVALUE(pImg->y);
    aBitmap[2].x = 0;     // Source Lower LH Corner
    aBitmap[2].y = 0;
    aBitmap[3].x = pImg->pbmp2->cx;   // Source Upper RH Corner
    aBitmap[3].y = pImg->pbmp2->cy;   

    aBitmap[0].x *= pwi->uXfactor;
    aBitmap[0].y *= pwi->uYfactor;
    aBitmap[1].x *= pwi->uXfactor;
    aBitmap[1].y *= pwi->uYfactor;


    rcl.xLeft    = WXVALUE(pImg->x);
    rcl.xRight   = WXVALUE(pImg->x) + pImg->cxdest;
    rcl.yBottom  = WYVALUE(pImg->y);
    rcl.yTop     = rcl.yBottom + pImg->cydest;

    /*
    ** Calculate the center of the cicular clipping area.
    */

    ptlCentre.x = WXVALUE(pImg->x) + (rcl.xRight - pImg->x)/ 2;
    ptlCentre.y = rcl.yBottom + (rcl.yTop   - rcl.yBottom)/2;

    ptlCentre.x *= pwi->uXfactor;
    ptlCentre.y *= pwi->uYfactor;


    arcpParms.lP = (rcl.xRight - rcl.xLeft)/ 2; /* radius in x direction*/
    arcpParms.lQ = (rcl.yTop   - rcl.yBottom)/2; /* radius in y direction*/
    arcpParms.lR = 0L;
    arcpParms.lS = 0L;


    arcpParms.lP *= pwi->uXfactor;
    arcpParms.lQ *= pwi->uYfactor;


    Multiplier = 0x0000ff00L;
    StartAngle = 0x00000000L;
    SweepAngle = MAKEFIXED(360,0);


    /*
    ** First make a circular black hole!!!
    */


    /*
    ** Make the clipping path for the image 
    */

    GpiBeginPath( hps, 1L);  /* define a clip path    */
    GpiSetCurrentPosition(hps,&ptlCentre);
    GpiSetArcParams(hps,&arcpParms);
    GpiPartialArc(hps,
                  &ptlCentre,
                  Multiplier,
                  StartAngle,
                  SweepAngle);
    GpiEndPath(hps);
    GpiSetClipPath(hps,1L,SCP_AND);

         GpiWCBitBlt(hps,  /* presentation space                      */
         hbm,              /* bit-map handle                          */
         4L,               /* four points needed to compress          */
         aBitmap,          /* points for source and target rectangles */
         ROP_SRCCOPY,      /* copy source replacing target            */
         BBO_IGNORE);      /* discard extra rows and columns          */

//    GpiBitBlt( hps,                        /* fill with 0's         */
//               NULLHANDLE,
//               2L,
//               &aBitmap[0],
//               ROP_ZERO,
//               BBO_IGNORE);
//    GpiBitBlt(hps, 
//              pwi->hpsMem, 
//              4, 
//              aBitmap, 
//              ROP_SRCPAINT, 
//              BBO_OR); 

   GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */

   return TRUE;
}


/*------------------------------------------------------------------------*/

PIMAGE ReadBmpFile(PIMAGE pimage)
{
   PBITMAPFILEHEADER2 pbfh2;                 /* can address any file types */
   PBITMAPINFOHEADER2 pbmp2;                 /* address any info headers */
   PBITMAPINFOHEADER  pbmpold;
   PBITMAPINFOHEAD_WIN pbmpwin;             /* windows 3.1 info header   */
   USHORT nColors,i;
   unsigned char *pBits,*pArray, *pWin;
   PRGB2 pRGB2;
   RGB   *pRGB;
   RGBWIN *pRGBWIN;

   if (!pimage->pFileData)
      return (PIMAGE)NULL;
   
   pArray= NULL;  /* Used for Bitmap array data */
   pbfh2 = (PBITMAPFILEHEADER2) pimage->pFileData;
   pbmp2 = NULL;                   /* only set this when we validate type */

   switch (pbfh2->usType)
   {
      case BFT_BITMAPARRAY:
         pArray = pimage->pFileData;
         pArray += (((PBITMAPARRAYFILEHEADER2)pimage->pFileData)->offNext);
         if (pArray)
            pimage->pFileData = pArray;
         pbfh2 = &(((PBITMAPARRAYFILEHEADER2)pimage->pFileData)->bfh2);
         pbmp2 = &pbfh2->bmp2;    /* pointer to info header (readability) */
         nColors = 1 << ( pbmp2->cBitCount * pbmp2->cPlanes );

         /*
         ** Since the bitmap array can be of two different formats,
         ** 1.2 bmp and 2.0 we should initialize pBits and pArray.
         */

         pBits = pArray = pimage->pFileData + pbfh2->offBits;
         break;

      case BFT_BMAP:
         pbmp2 = &pbfh2->bmp2;    /* pointer to info header (readability) */
         nColors = 1 << ( pbmp2->cBitCount * pbmp2->cPlanes );
         pBits = pimage->pFileData + pbfh2->offBits;
         break;
      case BFT_COLORICON:
      case BFT_ICON:        
         pbfh2 = (PBITMAPFILEHEADER2) pimage->pFileData;
         pbmp2 = &pbfh2->bmp2;    /* pointer to info header (readability) */    
         nColors = 1 << ( pbmp2->cBitCount * pbmp2->cPlanes );
         if (pbmp2->cbFix == sizeof(BITMAPINFOHEADER))
         {
            pimage->pFileData +=  sizeof(BITMAPFILEHEADER)+2*sizeof(RGB);
            pbfh2   = (PBITMAPFILEHEADER2) pimage->pFileData;
            pbmp2   = &pbfh2->bmp2;    /* pointer to info header (readability) */    
         }
         else
            pbmp2=NULL;
         break;

         default:      /* these formats aren't supported; don't set any ptrs */
         case BFT_POINTER:
         case BFT_COLORPOINTER:
            pbmp2 = NULL;
            break;

      }   /* end switch (pbfh2->usType) */

      if (pbmp2 == NULL)
         return (PIMAGE)NULL;

  
      if (pbmp2->cbFix == 12)           /* old format? */
      {
         pbmpold = (PBITMAPINFOHEADER)pbmp2;
         
         nColors = 1<< (pbmpold->cBitCount * pbmpold->cPlanes);
         printf("Old OS/2 1.2 format nColors=%d\n",nColors);

         if  (nColors <=256)
         {
            pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc(((ULONG)sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*nColors),
                                                        sizeof(char));
         }
         else
         {
            pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)sizeof(BITMAPINFOHEADER2),sizeof(char));
         }

         if (!pimage->pbmp2)
            return (PIMAGE)NULL;

         pimage->pbmp2->cx        = pbmpold->cx;
         pimage->pbmp2->cy        = pbmpold->cy;
         
         pimage->ImageDataSize = ROUNDTODWORD(pbmpold->cx * pbmpold->cBitCount)*pbmpold->cy;
         pimage->ImgData = (unsigned char *)calloc(pimage->ImageDataSize,sizeof(char));

         if (!pimage->ImgData)
         {
            free(pimage->pbmp2);
            return (PIMAGE)NULL;
         }
         pimage->pbmp2->cbFix     = sizeof(BITMAPINFOHEADER2);
         pimage->pbmp2->cPlanes   = pbmpold->cPlanes;
         pimage->pbmp2->cBitCount = pbmpold->cBitCount;
         pimage->pbmp2->ulCompression = BCA_UNCOMP;
         pimage->pbmp2->cxResolution  = 0;
         pimage->pbmp2->cyResolution  = 0;
         pimage->pbmp2->usUnits       = BRU_METRIC;
         pimage->pbmp2->usRecording   = 0; 
         pimage->pbmp2->usRendering   = 0; 
         pimage->pbmp2->ulColorEncoding = BCE_RGB;
         pimage->pbmp2->cclrUsed      = nColors;
         pimage->pbmp2->cclrImportant = 0;
         pimage->pbmp2->cbImage = pimage->ImageDataSize;

         if (nColors)
         {
            /*
            ** Use pBits to find colortable in the destination buffer.
            */
            pBits = (unsigned char *)pimage->pbmp2;
            pRGB2 = (RGB2 *)(pBits + sizeof(BITMAPINFOHEADER2));
            /*
            ** Use pBits to find colortable in source buffer.
            */
            pBits = (unsigned char *)pbmpold;
            pRGB  = (RGB  *)(pBits + 12);
            for (i=0; i < nColors; i++)
            {
               pRGB2->bRed   = pRGB->bRed;
               pRGB2->bGreen = pRGB->bGreen;
               pRGB2->bBlue  = pRGB->bBlue;
               pRGB2->fcOptions = 0;
               pRGB++;
               pRGB2++;
            }
            /*
            ** Finally set the pBits at the imagedata in the source!
            */
            if (pArray)
               pBits = pArray;
            else
               pBits = (unsigned char *)pRGB;
         }
         else
         {
            /* 
            ** Wow! true color
            */
            pBits = (unsigned char *)(pBits + sizeof(BITMAPINFOHEADER));
         }
         memcpy(pimage->ImgData,pBits,pimage->ImageDataSize);
      }
      else if (pbmp2->cbFix == sizeof(BITMAPINFOHEAD_WIN))
      {
         pbmpwin = (BITMAPINFOHEAD_WIN *)pbmp2;
         nColors = 1 << pbmpwin->biBitCount;

         pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)(sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*nColors),
                                                     sizeof(char));

         pimage->ImageDataSize = ((pbmpwin->biBitCount * pbmpwin->biWidth + 31)/32);
         pimage->ImageDataSize *= ( 4 * pbmpwin->biPlanes );
         pimage->ImageDataSize *= pbmpwin->biHeight;

         pimage->pbmp2->cbFix     = sizeof(BITMAPINFOHEADER2);
         pimage->pbmp2->cPlanes   = pbmpwin->biPlanes;
         pimage->pbmp2->cBitCount = pbmpwin->biBitCount;
         pimage->pbmp2->cx        = pbmpwin->biWidth;
         pimage->pbmp2->cy        = pbmpwin->biHeight;
         pimage->pbmp2->ulCompression = BCA_UNCOMP;
         pimage->pbmp2->cxResolution  = 0;
         pimage->pbmp2->cyResolution  = 0;
         pimage->pbmp2->usUnits       = BRU_METRIC;
         pimage->pbmp2->usRecording   = 0; 
         pimage->pbmp2->usRendering   = 0; 
         pimage->pbmp2->ulColorEncoding = BCE_RGB;
         pimage->pbmp2->cclrUsed      = nColors;
         pimage->pbmp2->cclrImportant = 0;
         pimage->pbmp2->cbImage = pimage->ImageDataSize;

         pimage->ImgData = (unsigned char *)calloc(pimage->ImageDataSize,sizeof(char));

         pBits = (unsigned char *)pimage->pbmp2;
         pRGB2 = (RGB2 *)(pBits + sizeof(BITMAPINFOHEADER2));
         pBits = (unsigned char *)pbmpwin;
         pRGBWIN  = (RGBWIN  *)(pBits + sizeof(BITMAPINFOHEAD_WIN));
         for (i=0; i < nColors; i++)
         {
            pRGB2->bRed   = pRGBWIN->rgbRed;
            pRGB2->bGreen = pRGBWIN->rgbGreen;
            pRGB2->bBlue  = pRGBWIN->rgbBlue;
            pRGB2->fcOptions = 0;
            pRGBWIN++;
            pRGB2++;
         }
         pWin = (unsigned char *)pbmpwin;
         pWin +=(sizeof(BITMAPINFOHEAD_WIN)+nColors*sizeof(RGBWIN));
         if (pWin && pimage->ImgData)
            memcpy(pimage->ImgData,pWin,pimage->ImageDataSize);
      }
      else 
      {
         /*
         ** The OS/2 iconeditor seems to make bitmaps within the header
         ** no image datasize, so we check for it.
         */
         if (!pbmp2->cbImage && ( pbmp2->cx && pbmp2->cy))
         {
            pimage->ImageDataSize = ROUNDTODWORD(pbmp2->cx * pbmp2->cBitCount)*pbmp2->cy;
            pbmp2->cbImage = pimage->ImageDataSize;
         }

         if (nColors <= 256)
         {
            pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)(sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*nColors),
                                                       sizeof(char));
         }
         else
         {
            pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)sizeof(BITMAPINFOHEADER2),
                                                        sizeof(char));
            nColors = 0;
         }
         pimage->ImgData = (unsigned char *)calloc(pbmp2->cbImage,sizeof(char));
         pimage->ImageDataSize = pbmp2->cbImage;

         memcpy(pimage->pbmp2,pbmp2,(sizeof(BITMAPINFOHEADER2)+sizeof(RGB2)*nColors));
         memcpy(pimage->ImgData,pBits,pbmp2->cbImage);
      }
     return pimage;
}
/*------------------------------------------------------------------------*/
/* PCX file format stuff..................                                */
/* Get a PCX Bytes out of the buffer, including the repeatcount.          */
/*------------------------------------------------------------------------*/

USHORT pcx_getb(USHORT *c, USHORT *n, PBYTE fp, USHORT maxn)
{
   UINT i;
   USHORT nBytes=0;
   static int csave=-1, nsave=-1;

   if ( !fp )
   {
      return nsave = csave = -1;
   }

   if ( nsave == -1 )
   {
      *n = 1;

      if ( !fp)
         return 0;
      else
      {
         i= *fp++;
         nBytes++;
      }
      if ( (i & 0xc0) == 0xc0 )
      {  *n = i & 0x3f;
         if ( !fp)
            return 0;
         else
         {
            i= *fp++;
            nBytes++;
         }
      }

      *c = i;
   } 
   else
   {  *n = nsave;
      *c = csave;
      nsave = csave = -1;
   }

   if ( *n > maxn )
   {  nsave = *n - maxn;
      csave = *c;
      *n = maxn;
   };
   return nBytes;
}
/*------------------------------------------------------------------------*/
/* Name : ReadPcxFile.                                                    */
/*                                                                        */
/* Reads the pcx file data and converts it to a OS/2 2.x bitmap format..  */
/*------------------------------------------------------------------------*/
PIMAGE ReadPcxFile(PIMAGE pimage)
{
unsigned char  bInf;
USHORT xMax,xMin,yMax,yMin;
unsigned char *pBegin, *pB,*phelp;
unsigned char * pData;          /* just a help var            */
ULONG l;
USHORT c,n,i,j;
PRGB2 pRGB2;
USHORT BytesPerLine;
USHORT cxBytes,RestBytes,total;
USHORT Planes;
USHORT nColors,nBytes;            /* Number of colors supported */
unsigned char PCXHead[128];

#define MakeWord(x)        (*(x) + (*(x+1) << 8));

   c = 0;  /* avoiding one warning. */

   pBegin = pimage->pFileData;

   for ( i = 0; i < 128; i++)                        /* Read PCX Header */
      PCXHead[i] = *(pimage->pFileData+i);
   
   pBegin = pimage->pFileData;

   /*
   **   Get Zsoft flag!
   */

   if ( PCXHead[0] != 10)                /* 0A = Zsoft file */
      return (PIMAGE)NULL;

   if ( PCXHead[1] == 0)
   {
      printf("Zsoft PCPaintBrush version 2.5 \n");

   }
   else if ( PCXHead[1] == 2)
   {
      printf("Zsoft PCPaintBrush version 2.8 with palette");
   }
   else if ( PCXHead[1] == 3)    // Zsoft PCPaintBrush version 2.8 without palette
   {
      return (PIMAGE)NULL;
   }
   else if ( PCXHead[1] == 4)      
   {
      printf("Zsoft PCPaintBrush version for Windows\n");
   }

   // Supported  = Zsoft PCPaintBrush version 3.0 and higher!

   printf("Encoding byte = %d \n",PCXHead[2]); 

  /* Number of colors =  1 << (cBitCount * cPlanes) */

  if (PCXHead[1] < 5 )
     nColors = 1 << (PCXHead[3] * PCXHead[65] );
  else
     nColors = 256;

  pimage->pbmp2= (BITMAPINFOHEADER2 *)calloc((ULONG)(sizeof(BITMAPINFOHEADER2)+ sizeof(RGB2)*nColors),
                                          sizeof(char));

   /*
   ** Set a pointer to the beginning of the colorpart.
   */

   /* eerst terug naar bytes....*/

   pData = (PBYTE)pimage->pbmp2;
   pRGB2 = (RGB2 *)(pData + sizeof(BITMAPINFOHEADER2));

   pimage->pbmp2->cBitCount = PCXHead[3];  /* Bits per pixel */


   printf("Nr of Bits/pixel %d \n",pimage->pbmp2->cBitCount);


   phelp = (pBegin + 4);        /* Get Xmin */

   xMin   = MakeWord(phelp); phelp +=2;
   yMin   = MakeWord(phelp); phelp +=2;
   xMax   = MakeWord(phelp); phelp +=2;
   yMax   = MakeWord(phelp); phelp +=2;


   pimage->pbmp2->cbFix     = sizeof(BITMAPINFOHEADER2); // (Full struct less CLUT)
   pimage->pbmp2->cx        = (ULONG)((xMax - xMin) +1);
   pimage->pbmp2->cy        = (ULONG)((yMax - yMin) +1);


   pimage->pFileData = pBegin;
   pimage->pFileData +=65;                /*Number of planes at byte 65 */

  
   pimage->pbmp2->cPlanes   =*pimage->pFileData++;

   printf("Number of planes %d \n",pimage->pbmp2->cPlanes);


   BytesPerLine = MakeWord(pimage->pFileData);

   printf("Number of bytes/line for one plane  %d \n",BytesPerLine);

   nColors = 1 << ( pimage->pbmp2->cBitCount * pimage->pbmp2->cPlanes );

   pimage->pbmp2->ulCompression = BCA_UNCOMP;
   pimage->pbmp2->cxResolution  = 0;
   pimage->pbmp2->cyResolution  = 0;
   pimage->pbmp2->usUnits       = BRU_METRIC;
   pimage->pbmp2->usRecording   = 0; 
   pimage->pbmp2->usRendering   = 0; 
   pimage->pbmp2->ulColorEncoding = BCE_RGB;
   pimage->pbmp2->cclrUsed      = nColors;
   pimage->pbmp2->cclrImportant = 0;


   /* Set pimage->pFileData at the start of the color tab */

   if (nColors == 256)
   {
      pimage->pFileData = pBegin;
      pimage->pFileData += pimage->filesize - 768;
      for ( i = 0; i < 256; i++)
      {
         pRGB2->bRed  = *(pimage->pFileData++);
         pRGB2->bGreen= *(pimage->pFileData++);
         pRGB2->bBlue = *(pimage->pFileData++);
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
   }
   else
   {
      pimage->pFileData = pBegin;
      pimage->pFileData += 16;

      for ( i = 0; i < nColors; i++)
      {
         pRGB2->bRed  =  *(pimage->pFileData++);
         pRGB2->bGreen=  *(pimage->pFileData++);
         pRGB2->bBlue =  *(pimage->pFileData++);
         pRGB2->fcOptions = 0;
         pRGB2++;
      }
   }

   /*
   ** Calculate including padding
   */

   printf("xMax    = %d pimage->pbmp2->cx = %d \n",xMax, pimage->pbmp2->cx);
   printf("yMax    = %d pimage->pbmp2->cy = %d \n",yMax, pimage->pbmp2->cy);
   printf("nColors = %d \n",nColors);

   total = pimage->pbmp2->cx;
   total *= pimage->pbmp2->cBitCount;
   total *= pimage->pbmp2->cPlanes;
   cxBytes = (total + 7) >> 3;
   RestBytes = cxBytes % 4;    
   if (RestBytes) 
      RestBytes = 4 - RestBytes;          /* Make it a multiple of 4 bytes */ 
   cxBytes = cxBytes + RestBytes;

   pimage->ImageDataSize = cxBytes * pimage->pbmp2->cy;

   pimage->pbmp2->cbImage       = pimage->ImageDataSize;


   printf("pimage->ImageDataSize %d \n",pimage->ImageDataSize);

   pimage->ImgData = (unsigned char *)calloc(pimage->ImageDataSize,sizeof(char));
  
    /* Save pointer at the beginning of the imagedata */

   pB      = pimage->ImgData; 

   pimage->pFileData = pBegin;
   pimage->pFileData +=70;
   xMax = MakeWord(pimage->pFileData); pimage->pFileData +=2;
   yMax = MakeWord(pimage->pFileData);

   pimage->pFileData = pBegin;

   pimage->pFileData += 128;  /* End of PCX header */

   /* for each scanline do ...*/
   for (yMin = 0; yMin < pimage->pbmp2->cy; yMin++)
   {
      printf("ScanLineNumber=%d\n",yMin);
      for ( Planes = 0; Planes < pimage->pbmp2->cPlanes; Planes++ )
      {  
         for ( n=i=0; i<BytesPerLine; i += n )
         {  
            nBytes = pcx_getb( (USHORT *)&c, (USHORT *)&n, (UCHAR *)pimage->pFileData, BytesPerLine - i );
            if (!nBytes)
            {
               free(pimage->ImgData);
               free(pimage->pbmp2);
               pimage->status = IMG_WRONGFORMAT;
               return pimage;
            }
            else
               pimage->pFileData +=nBytes;
            for ( j=0; j < n; j++ )
               *(pimage->ImgData++) = c;
         }
      }
      if (BytesPerLine < cxBytes)
         pimage->ImgData += (cxBytes - BytesPerLine);
   }
  pimage->ImgData = pB;

  for ( l=0; l < (pimage->ImageDataSize/ 2 ); l++)
  {
     bInf = *(pimage->ImgData+( pimage->ImageDataSize - l ));
     *(pimage->ImgData+( pimage->ImageDataSize - l )) = *(pimage->ImgData+ l);
     *(pimage->ImgData+ l) = bInf;
  }
  pimage->status = 0;

  return pimage;
}
/*------------------------------------------------------------------------*/
/* Exports the selected image as an OS/2 bitmap to disk                   */
/*------------------------------------------------------------------------*/
BOOL SaveImage(pImage pImg)
{
   ULONG             nColors;
   Loadinfo          linf;
   BITMAPFILEHEADER2 bmpfh2;
   PBYTE             data;
   PRGB2             pRGB2;

   if (!pImg)
      return FALSE;

   /*
   ** Try to get a filename...
   */
   linf.dlgflags = FDS_SAVEAS_DIALOG;
   linf.szFileName[0]=0;
   strcpy(linf.szExtension,".BMP");
   if (!FileGetName(&linf))
      return FALSE;

   linf.handle = open(linf.szFileName,O_WRONLY | O_TRUNC | O_BINARY | O_CREAT,S_IREAD | S_IWRITE); 

   if (linf.handle == -1)
    {
       WinMessageBox(HWND_DESKTOP,
                     hwndClient,
                     (PSZ)"Cannot open requested file", 
                     (PSZ)"File open error", 0,
                     MB_OK | 
                     MB_APPLMODAL |
                     MB_MOVEABLE | 
                     MB_ICONEXCLAMATION);

       return FALSE;
    }

    nColors = 1 << ( pImg->pbmp2->cBitCount * pImg->pbmp2->cPlanes );

    bmpfh2.usType = BFT_BMAP;   /* 'BM' */
    bmpfh2.cbSize = sizeof(BITMAPFILEHEADER2);
    bmpfh2.xHotspot = 0;
    bmpfh2.yHotspot = 0;
    bmpfh2.offBits  = 0;

    /*
    ** Check for palette...
    */

    if (nColors <= 256 )
    {
       bmpfh2.offBits  = (sizeof(RGB2) * nColors ) + sizeof(BITMAPFILEHEADER2);
       data = (unsigned char *)pImg->pbmp2;
       pRGB2 = (RGB2 *)(data + sizeof(BITMAPINFOHEADER2));
    }
    else
    {
       /*
       ** No palette so true color
       */
       bmpfh2.offBits  = sizeof(BITMAPINFOHEADER2);
       pRGB2 = (RGB2 *)0;
    }

    memcpy((void *)&bmpfh2.bmp2,(void *)pImg->pbmp2,sizeof(BITMAPINFOHEADER2));
    
    /*
    ** Save the bitmap fileheader...
    */
    write(linf.handle,(PVOID)&bmpfh2,(ULONG)sizeof(BITMAPFILEHEADER2));

    if (pRGB2) /* If there is a palette save it */
    {
        write(linf.handle,(PVOID)pRGB2,
              (ULONG)(nColors * sizeof(RGB2)) );
    }
    write(linf.handle,(PVOID)pImg->ImgData,(ULONG)pImg->pbmp2->cbImage);

    close( linf.handle);

    return TRUE;
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
BOOL SaveBitmap (HBITMAP hbm, HPS hpsMem)
{
   Loadinfo          linf;
   BITMAPFILEHEADER2 bfh;   /* includes the bitmapinfoheader2 at the end*/
   PBITMAPINFO2      pbmi;  /* is infoheader2 + pointer to rgb table    */
   PBYTE             pbScan;
   RGB2              *pRGB2;
   ULONG             lBmpDataSize;
   USHORT            sScanLineSize;
   USHORT            nColors;
   BOOL              bRet;
   ULONG             sScans;
                              // Get bitmap information
   /*
   ** Try to get a filename...
   */
   linf.dlgflags = FDS_SAVEAS_DIALOG;
   linf.szFileName[0]=0;
   strcpy(linf.szExtension,".BMP");
   if (!FileGetName(&linf))
      return FALSE;

   linf.handle = open(linf.szFileName,
                    O_WRONLY | O_TRUNC | O_BINARY | O_CREAT,S_IREAD | S_IWRITE); 

   if (linf.handle < 0)
      return FALSE;

   bfh.usType = BFT_BMAP;   /* 'BM' */
   bfh.xHotspot = 0;
   bfh.yHotspot = 0;
   bfh.offBits  = 0;
   bfh.bmp2.cbFix = sizeof (BITMAPINFOHEADER2);

   bRet = GpiQueryBitmapInfoHeader(hbm, &bfh.bmp2);

   if(!bRet)
   {
      DispError(hab,hwndFrame);      
      return FALSE;
   }
   sScanLineSize = ((bfh.bmp2.cBitCount * bfh.bmp2.cx + 31) / 32) * 4 *
                     bfh.bmp2.cPlanes;

   lBmpDataSize  = (LONG) sScanLineSize * bfh.bmp2.cy ;

   nColors = 1 << (bfh.bmp2.cBitCount * bfh.bmp2.cPlanes);


   bfh.usType = BFT_BMAP;   /* 'BM' */
   bfh.cbSize = sizeof(BITMAPFILEHEADER2);
   bfh.xHotspot = 0;
   bfh.yHotspot = 0;
   bfh.offBits  = 0;

   // Create memory DC and PS, and set bitmap in it

   if (!GpiSetBitmap (hpsMem, hbm))
      DispError(hab,hwndFrame);

   // Allocate memory for BITMAPINFO table & scans

   pbmi = calloc( sizeof(BITMAPINFOHEADER2) + (nColors * sizeof(RGB2)),1);

   memcpy((void *)pbmi,&bfh.bmp2,sizeof(BITMAPINFOHEADER2));

   pbScan = calloc(lBmpDataSize,sizeof(char));

   sScans = GpiQueryBitmapBits (hpsMem, 0L, bfh.bmp2.cy, pbScan, pbmi);

   if(!sScans)
   {
      DispError(hab,hwndFrame);
      free(pbScan);
      close( linf.handle);
      return FALSE;
   }

   if (nColors && nColors <=256)
   {
      bfh.offBits  = (sizeof(RGB2) * nColors ) + sizeof(BITMAPFILEHEADER2);
      pRGB2 = &pbmi->argbColor[0];     /* Color definition record                */
   }
   else
   {
       bfh.offBits  = sizeof(BITMAPFILEHEADER2);
       pRGB2 = (RGB2 *)0;
   }
   /*
   ** Save the bitmap fileheader...
   */
   _write(linf.handle,(PVOID)&bfh,(ULONG)sizeof(BITMAPFILEHEADER2));

    if (pRGB2) /* If there is a palette save it */
    {
       _write(linf.handle,(PVOID)pRGB2,(ULONG)(nColors * sizeof(RGB2)));
    }

   /*
   ** Save Image data..
   */
   _write(linf.handle,(PVOID)pbScan,(ULONG)lBmpDataSize);

   close( linf.handle);
   free (pbScan) ;
   free (pbmi);
   return 0 ;
}
/*-------------------------------------------------------------------------*/
/*  Name       : GetBitmap                                                 */
/*                                                                         */
/*  Description: If a bounding rectangle has been defined, create a screen */
/*               compatible device context and a screen compatible bit map.*/
/*               Select the bit map into the DC, create a                  */
/*               suitably sized GPI PS and associate it with the DC.       */
/*               BitBlt that part of the                                   */
/*               screen defined by the rectangle into the new bit map.     */
/*               Destroy all other resources but return the bit map handle */
/*                                                                         */
/*  Parameters : [none]                                                    */
/*                                                                         */
/*  Return     : TRUE on success.                                          */
/*-------------------------------------------------------------------------*/
BOOL ExportSel2Bmp(WINDOWINFO *pwi,RECTL *prcl)
{
   HBITMAP bmap;
   HDC     hdcMem;
   HPS     hpsMem;
   BITMAPINFOHEADER2 bmpMemory;
   HPOINTER tptr;
   SIZEL    sizlWork;
   POINTL   ptl;
   BOOL    bRet = TRUE;
   /*
   ** Do not allow anny interaction with the user, so 
   ** make a clock pointer....
   */

   tptr = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);

   WinSetPointer(HWND_DESKTOP, tptr);

   CreateBitmapHdcHps(&hdcMem,&hpsMem);

   GpiCreateLogColorTable ( hpsMem, 
                            LCOL_RESET, 
                            LCOLF_RGB, 0, 0, NULL );

   /*
   ** It is a bounding rectangle, so we only want what is inside.
   */
   sizlWork.cx = (prcl->xRight - prcl->xLeft);
   sizlWork.cy = (prcl->yTop - prcl->yBottom);

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

   bmap = GpiCreateBitmap(hpsMem,
                          (PBITMAPINFOHEADER2)&bmpMemory,
                          0L,
                          (PBYTE)NULL,
                          (PBITMAPINFO2)NULL);

   GpiSetBitmap(hpsMem,bmap);
   /*
   ** First off all draw a square and fill this with the 
   ** background color of the drawing area.
   */
   ptl.x = 0;
   ptl.y = 0;
   GpiMove(hpsMem,&ptl);
   ptl.x = sizlWork.cx;
   ptl.y = sizlWork.cy;
   GpiSetPattern(hpsMem,PATSYM_SOLID);
   GpiSetMix(hpsMem, FM_OVERPAINT);
   GpiSetColor(hpsMem,pwi->lBackClr);
   GpiBox(hpsMem,DRO_FILL,&ptl,0L,0L);
   /*
   ** Start drawing the selected elements in our 
   ** memoryPS..
   */
   for ( pwi->usdrawlayer = MINLAYER; pwi->usdrawlayer <= MAXLAYER; pwi->usdrawlayer++)
      ObjDrawSelected(hpsMem,pwi,prcl);

   /*
   ** Save the bitmap to disk.
   */
   bRet = SaveBitmap (bmap,hpsMem);

   /*
   ** Set the normal pointer back
   ** cleanup and go....
   */
   WinSetPointer(HWND_DESKTOP, 
                 WinQuerySysPointer(HWND_DESKTOP,SPTR_ARROW,FALSE));

   GpiAssociate(hpsMem, (HDC)0L);
   GpiDestroyPS(hpsMem);
   DevCloseDC(hdcMem);
   return bRet;
}  /*  end of GetBitmap  */
