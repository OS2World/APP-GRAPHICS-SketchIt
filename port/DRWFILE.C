/*------------------------------------------------------------------------*/
/*  Name: drwfile.c                                                       */
/*                                                                        */
/*  Description : Functions for loading the drawit file.                  */
/*                                                                        */
/*                                                                        */
/* Private functions:                                                     */
/*   appendobj      : Appends an object to the local chain of objects     */
/*   delobj         : Removes an object from the local chain.             */
/*   delChain       : Removes complete chain when old file cannot be read */
/*   FileLoadSquare : Reads a square from the old file.                   */
/*   FileLoadCircle : Reads a circle from the old file.                   */
/*   FileLoadSpline : Reads a spline from the old file.                   */
/*   FileLoadLine   : Reads a line (new in version 3.0)                   */
/*   FileLoadText   : Reads text from from file.                          */
/*   readFile       : This function reads the complete file.              */
/*                                                                        */
/* Public functions :                                                     */
/*   LoadDrwFile    : Collects some pointers and starts the load thread.  */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_sqr.h"
#include "dlg_cir.h"
#include "dlg_txt.h"
#include "dlg_lin.h"
#include "dlg_img.h"
#include "drwtrian.h"  /* triangle stuff           */
#include "drwsplin.h"  /* polylines & splines      */
#include "drwmeta.h"   /* Metafile...              */
#include "drwutl.h"
#include "drwgrp.h"    /* Grouping of objects...   */
#include "drwbtext.h"
#include "drwcanv.h"
#include "resource.h"

#define PICTHREADSTACK 16384 * 2
/*
** Thread variables for multithreaded loading of a pic file.
*/

static POBJECT pbase,pend;

/*
** Helper struct to be used for multithreading.
*/
typedef struct filetype
{
   WINDOWINFO *pwi;
   pLoadinfo pli;
   TID         ThreadID;       /* New thread ID (returned)                   */
   ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
   ULONG       StackSize;      /* Size in bytes of new thread's stack        */
} FILETYPEH;

/*
** BASECLASS DEFINITION!!
*/
typedef struct _bobj
{
   OBJECT   obj;
   USHORT   ustype;      /* type of object          */
   BATTR    bt;          /* Base attributes.        */
} *PBSOBJ;


static FILETYPEH oldf;

/*-----------------------------------------------[ private ]--------------*/
/* readFileMarker.                                                        */
/*                                                                        */
/* Description  : Some objects like the image and block text use markers  */
/*                this allows a better error checking while loading the   */
/*                file. In the future error messages can be added...      */
/*                                                                        */
/* Parameters   : int hFile - handle of the file.                         */
/*------------------------------------------------------------------------*/
static BOOL readFileMarker(int hFile, char *pszMarker)
{
   char szMark[50];
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
/*-------------------------------------------------------------------------*/
static void appendobj(POBJECT pobj)
{
   POBJECT ptmp;

   if (!pbase)
   {
      pbase = pobj;
      pend           = pbase;              /*Start of list */
      pend->Next     = NULL;
      pend->Previous = NULL;
      pend->usClass  = pobj->usClass;
      pobj->bLocked  = FALSE;
   }
   else if (pend->Next==NULL)
   {
      /*
      ** Add an element to the end of the list.
      */
      pend->Next = pobj;
      ptmp = pend;
      pend = pend->Next;
      pend->Previous = ptmp;
      pend->Next     = NULL;
      pend->usClass  = pobj->usClass;
      pobj->bLocked  = FALSE;
   }
   return;
}
/*-------------------------------------------------------------------------*/
static int FileLoadSquare(int hfile, PSQUARE pSqr, WINDOWINFO *pwi,
                          POBJECT *pConvert)
{
   static SQUARE sqr;
   PBSOBJ psqrbase,psplbase;
   SPLINE *pSpl;
   SQR29  sqr29;
   POBJECT pObj;
   PBYTE  p;
   int iSize,i;
   int iDontRead;
   int iRead;
   ULONG   ulBytes;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */

   *pConvert = NULL;

   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat  == FILE_DRAWIT30 )
   {
      iSize = sizeof(SQUARE) - iDontRead;
      p = (PBYTE)&sqr;
      p += iDontRead;     /* jump over ustype & object */
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
   {
      iSize = sizeof(SQR29) - iDontRead;
      p = (PBYTE)&sqr29;
      p += iDontRead;     /* jump over ustype & object */
      iRead = read(hfile,(PVOID)p,iSize);
   }

   if (iRead > 0)
   {
      if (pwi->lFileFormat  != FILE_DRAWIT30 &&
          pwi->lFileFormat  != FILE_DRAWIT32 )
      {
        for (i = 0; i < 4; i++)
           sqr.ptlf[i] = sqr29.ptlf[i];
        sqr.ustype          = CLS_SQR;
        sqr.bt.line         = sqr29.line;       /* Used line def in square */
        sqr.ptlSqrCenter    = sqr29.ptlSqrCenter;
        sqr.bt.lPattern     = sqr29.lPattern;   /* Filling pattern         */
        sqr.bt.fColor       = sqr29.fColor;     /* Filling color.          */
        sqr.bt.gradient     = sqr29.gradient;   /* gradient Filling        */
        sqr.bt.usLayer      = sqr29.uslayer;    /* layer it belongs to     */
        sqr.bt.ptlfCenter   = sqr29.ptlfCenter; /* Rotation center.        */
        sqr.bt.fountain     = sqr29.fountain;   /* fountain fill           */
        sqr.bt.Shade.lShadeType  = SHADE_NONE;
        sqr.bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
        sqr.bt.Shade.lUnits      = pwi->Shade.lUnits;
      }

      memcpy(pSqr,&sqr,sizeof(SQUARE)); /* Overrides object class in pobject*/

      *pConvert = (POBJECT)pObjNew(NULL,CLS_SPLINE);

      psqrbase = (PBSOBJ)pSqr;
      psplbase = (PBSOBJ)*pConvert;
      psplbase->bt     = psqrbase->bt;
      psplbase->ustype = CLS_SPLINE;
      pSpl             = (SPLINE *)*pConvert;
      pSpl->nrpts      = 4;
      pSpl->ulState    = SPL_CLOSED;
      pSpl->ustype     = CLS_SPLINE;
      pObj             = (POBJECT)pSpl;
      pObj->usClass    = CLS_SPLINE;
      pObj->moveOutline = SplinMoveOutLine;
      pObj->paint       = DrawSplineSegment;
      pObj->getInvalidationArea   = SplineInvArea;

      ulBytes          = 4 * sizeof(POINTLF);
      pSpl->pptl       = (PPOINTLF)calloc((ULONG)ulBytes,sizeof(char));

      for (i = 0; i < 4; i++)
      {
         pSpl->pptl[i]    =  pSqr->ptlf[i];
         pSpl->pptl[i].x +=  pSqr->ptlSqrCenter.x;
         pSpl->pptl[i].y +=  pSqr->ptlSqrCenter.y;
      }
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadBlockText(int hfile, PBLOCKTEXT pT, WINDOWINFO *pwi)
{
   static blocktext bText;
   BLOCK31 btxt31;
   PBYTE  p;
   ULONG  ulBytes;
   POBJECT pObj;
   int iSize,i;
   int iDontRead;
   int iRead = -1;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

    if (pwi->lFileFormat  == FILE_DRAWIT32 )
    {
        iSize = sizeof(BLOCKTEXT) - iDontRead;
        p = (PBYTE)&bText;
        p += iDontRead;
        iRead = read(hfile,(PVOID)p,iSize);
    }
    else if (pwi->lFileFormat  == FILE_DRAWIT30 )
    {
        iSize = sizeof(BLOCK31) - iDontRead;
        p = (PBYTE)&btxt31;
        p += iDontRead;
        iRead = read(hfile,(PVOID)p,iSize);
    }
    else
        return -1; /* ERROR */

    if (pwi->lFileFormat  == FILE_DRAWIT30 )
    {
        pT->ustype          = CLS_BLOCKTEXT;
        pT->bt.line         = btxt31.bt.line;
        pT->bt.fColor       = btxt31.bt.fColor;        /* filling color           */
        pT->bt.ptlfCenter   = btxt31.bt.ptlfCenter;
        pT->bt.fountain     = btxt31.bt.fountain;
        pT->bt.arrow        = btxt31.bt.arrow;
        pT->bt.usLayer      = btxt31.bt.usLayer;       /* layer it belongs to     */
        pT->bt.lPattern     = btxt31.bt.lPattern;
        pT->bt.gradient     = btxt31.bt.gradient;
        pT->usColumns       = btxt31.usColumns;     /* Number of columns available.                */
        pT->usPage          = btxt31.usPage;        /* PageNumber.                                 */
        pT->nAlign          = btxt31.nAlign;        /* Text alignment in column.                   */
        pT->nSpace          = btxt31.nSpace;        /* Spacing.                                    */
        pT->ulColColor      = btxt31.ulColColor;    /* Column color. In most cases transparent.    */
        pT->fattrs          = btxt31.fattrs;        /* Font attribute structure.                   */
        pT->sizfx           = btxt31.sizfx;         /* Size of the fontbox floating point normalized*/

        for (i=0; i < MAX_COLUMNS; i++)
           pT->rclf[i] = btxt31.rclf[i];            /* Column rectangles in a normalized rectangles*/

        pT->lTextlen        = btxt31.lTextlen;      /* Number of chars in textblock including \0   */
        /*
        ** New in 3.2
        */
        pT->bt.Shade.lShadeType  = SHADE_NONE;
        pT->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
        pT->bt.Shade.lUnits      = pwi->Shade.lUnits;
    }
    else
    {
        memcpy(pT,&bText,sizeof(BLOCKTEXT));
    }
   pObj = (POBJECT)pT;
   pObj->usClass = CLS_BLOCKTEXT;
   pObj->moveOutline = BlockTextMoveOutLine; 
   pObj->paint       = drawBlockText;
   pObj->getInvalidationArea   = BlockTextInvArea;

   pT->ustype    = CLS_BLOCKTEXT;

   ulBytes = pT->lTextlen;

   if ( ulBytes >  0 )
   {
      pT->pszText = (char *)calloc((ULONG)ulBytes,sizeof(char));
      iRead = read(hfile,(PVOID)pT->pszText,ulBytes);
   }
   /*
   ** Try to load the end of blocktext marker.
   */
   if (!readFileMarker(hfile,EOF_BLOCKTEXT))
      return 0;

   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadCircle(int hfile, PCIRCLE pCir, WINDOWINFO *pwi)
{
   static CIRCLE cir;
   CIR29 cir29;
   CIR31 cir31;         /* version 3.0 uptil 3.1c */
   PBYTE  p;
   POBJECT pObj;
   int iSize;
   int iDontRead;
   int iRead = -1;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat  == FILE_DRAWIT32 )
   {
      iSize = sizeof(CIRCLE) - iDontRead;
      p = (PBYTE)&cir;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT30 )
   {
      iSize = sizeof(CIR31) - iDontRead;
      p = (PBYTE)&cir31;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT29 )
   {
      iSize = sizeof(CIR29) - iDontRead;
      p = (PBYTE)&cir29;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
       return -1;   /* ERROR */

   if (iRead > 0)
   {
      if (pwi->lFileFormat  == FILE_DRAWIT29 )
      {
         pCir->ustype         = CLS_CIR;        /* type of object */
         pCir->m_bClose       = TRUE;
         pCir->StartAngle     = cir29.StartAngle;
         pCir->SweepAngle     = cir29.SweepAngle;
         pCir->bt.line        = cir29.line;
         pCir->bt.fColor      = cir29.fColor;        /* filling color           */
         pCir->bt.ptlfCenter  = cir29.ptlfCenter;
         pCir->bt.fountain    = cir29.fountain;
         pCir->bt.arrow       = cir29.Arrow;
         pCir->bt.usLayer     = cir29.uslayer;       /* layer it belongs to     */
         pCir->bt.lPattern    = cir29.lPattern;
         pCir->bt.gradient    = cir29.gradient;
         pCir->Rotate         = cir29.Rotate;
         pCir->ptlPosn        = cir29.ptlPosn;
         pCir->arcpParms      = cir29.arcpParms;

         pCir->bt.Shade.lShadeType  = SHADE_NONE;
         pCir->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pCir->bt.Shade.lUnits      = pwi->Shade.lUnits;

      }
      else if (pwi->lFileFormat  == FILE_DRAWIT30 )          /* version 3.0 uptil 3.1c */
      {
         pCir->ustype         = CLS_CIR;        /* type of object */
         pCir->m_bClose       = TRUE;
         pCir->StartAngle     = cir31.StartAngle;
         pCir->SweepAngle     = cir31.SweepAngle;
         pCir->bt.line        = cir31.bt.line;
         pCir->bt.fColor      = cir31.bt.fColor;        /* filling color           */
         pCir->bt.ptlfCenter  = cir31.bt.ptlfCenter;
         pCir->bt.fountain    = cir31.bt.fountain;
         pCir->bt.arrow       = cir31.bt.arrow;
         pCir->bt.usLayer     = cir31.bt.usLayer;       /* layer it belongs to     */
         pCir->bt.lPattern    = cir31.bt.lPattern;
         pCir->bt.gradient    = cir31.bt.gradient;
         pCir->Rotate         = cir31.Rotate;
         pCir->ptlPosn        = cir31.ptlPosn;
         pCir->arcpParms      = cir31.arcpParms;
         pCir->bt.Shade.lShadeType  = SHADE_NONE;
         pCir->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pCir->bt.Shade.lUnits      = pwi->Shade.lUnits;
      }
      else
      {
         memcpy(pCir,&cir,sizeof(CIRCLE));
      }
      pObj = (POBJECT)pCir;
      pObj->usClass = CLS_CIR;
      pObj->paint		= DrawCircleSegment;
      pObj->moveOutline	= CirMoveOutLine;
      pObj->getInvalidationArea   = CircleInvArea;
      pCir->ustype =  CLS_CIR;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadLine(int hfile, PLINE pLin, WINDOWINFO *pwi)
{
   static  Lines lin;
   LNS31   lns31;
   LNS29   lns29;
   PBYTE   p;
   POBJECT pObj;
   int     iSize;
   int     iDontRead;
   int     iRead = -1;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat  == FILE_DRAWIT32 )     /* version 3.2 */
   {
      iSize = sizeof(Lines) - iDontRead;
      p = (PBYTE)&lin;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT30 ) /* version 3.0 uptill 3.1c */
   {
      iSize = sizeof(LNS31) - iDontRead;
      p = (PBYTE)&lns31;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT29 )
   {
      iSize = sizeof(LNS29) - iDontRead;
      p = (PBYTE)&lns29;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
       return -1; /* ERROR */


   if (iRead > 0)
   {
      if (pwi->lFileFormat == FILE_DRAWIT30)
      {
         pLin->ustype               = CLS_LIN;        /* type of object */
         pLin->bt.line              = lns31.bt.line;
         pLin->bt.usLayer           = lns31.bt.usLayer;
         pLin->bt.ptlfCenter        = lns31.bt.ptlfCenter;
         pLin->bt.arrow             = lns31.bt.arrow;
         pLin->bt.Shade.lShadeType  = SHADE_NONE;
         pLin->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pLin->bt.Shade.lUnits      = pwi->Shade.lUnits;
         pLin->ptl1                 = lns31.ptl1;
         pLin->ptl2                 = lns31.ptl2;
      }
      else if (pwi->lFileFormat  == FILE_DRAWIT29)
      {
         pLin->ustype        = CLS_LIN;        /* type of object */
         pLin->bt.line       = lns29.line;
         pLin->bt.ptlfCenter = lns29.ptlfCenter;
         pLin->bt.usLayer    = lns29.uslayer;
         pLin->bt.arrow      = lns29.Arrow;
         pLin->bt.Shade.lShadeType  = SHADE_NONE;
         pLin->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pLin->bt.Shade.lUnits      = pwi->Shade.lUnits;
         pLin->ptl1          = lns29.ptl1;
         pLin->ptl2          = lns29.ptl2;
      }
      else
      {
         memcpy(pLin,&lin,sizeof(Lines));
      }
      pObj = (POBJECT)pLin;
      pObj->usClass = CLS_LIN;
	  pObj->paint    = DrawLineSegment;
      pObj->moveOutline = LineMoving;
      pObj->getInvalidationArea   = LineInvArea;
      pLin->ustype =  CLS_LIN;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadSpline(int hfile, PSPLINE pSpl, WINDOWINFO *pwi)
{
   static SPLINE spl;
   SPL29  spl29;
   SPL31  spl31;                     /* version 3.0 uptil 3.1c */
   PBYTE  p;
   POBJECT pObj;
   int iSize;
   int iDontRead;
   int iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat  == FILE_DRAWIT32 )         /* version 3.2 */
   {
      iSize = sizeof(SPLINE) - iDontRead;
      p = (PBYTE)&spl;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT30 )    /* version 3.0 uptil 3.1c */
   {
      iSize = sizeof(SPL31) - iDontRead;
      p = (PBYTE)&spl31;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat  == FILE_DRAWIT29 )
   {
      iSize = sizeof(SPL29) - iDontRead;
      p = (PBYTE)&spl29;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
       return -1;   /* ERROR */


   if (iRead > 0)
   {
      if (pwi->lFileFormat  == FILE_DRAWIT30)    /* version 3.0 uptil 3.1c */
      {
         pSpl->ustype       = CLS_SPLINE;        /* type of object */
         pSpl->bt.line         = spl31.bt.line;
         pSpl->bt.usLayer      = spl31.bt.usLayer;
         pSpl->bt.ptlfCenter   = spl31.bt.ptlfCenter;
         pSpl->bt.arrow.lSize  = DEF_ARROWSIZE;
         pSpl->bt.arrow.lEnd   = DEF_LINEEND;
         pSpl->bt.arrow.lStart = DEF_LINESTART;
         pSpl->nrpts           = spl31.nrpts; 
         pSpl->bt.lPattern     = spl31.bt.lPattern;
         pSpl->bt.gradient     = spl31.bt.gradient;
         pSpl->bt.fColor       = spl31.bt.fColor;
         pSpl->ulState         = spl31.ulState;
         pSpl->bt.fountain     = spl31.bt.fountain;

         pSpl->bt.Shade.lShadeType  = SHADE_NONE;               /* New in 3.2 */
         pSpl->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pSpl->bt.Shade.lUnits      = pwi->Shade.lUnits;

      }
      else if ( pwi->lFileFormat  == FILE_DRAWIT29 )
      {
         pSpl->ustype       = CLS_SPLINE;        /* type of object */
         pSpl->bt.line         = spl29.line;
         pSpl->bt.usLayer      = spl29.uslayer;
         pSpl->bt.ptlfCenter   = spl29.ptlfCenter;
         pSpl->bt.arrow        = spl29.Arrow;
         pSpl->bt.lPattern     = spl29.lPattern;
         pSpl->bt.gradient     = spl29.gradient;
         pSpl->bt.fColor       = spl29.fColor;
         pSpl->bt.fountain     = spl29.fountain;
         pSpl->nrpts           = spl29.nrpts; 
         pSpl->ulState         = spl29.ulState;

         pSpl->bt.Shade.lShadeType  = SHADE_NONE;               /* New in 3.2 */
         pSpl->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pSpl->bt.Shade.lUnits      = pwi->Shade.lUnits;

      }
      else
      {
         memcpy(pSpl,&spl,sizeof(SPLINE));
      }

      pSpl->ustype  = CLS_SPLINE;
      pObj = (POBJECT)pSpl;
      pObj->usClass = CLS_SPLINE;
	  pObj->moveOutline = SplinMoveOutLine;
	  pObj->paint       = DrawSplineSegment;
      pObj->getInvalidationArea   = SplineInvArea;

      iSize = pSpl->nrpts * sizeof(POINTLF);
      pSpl->pptl = (PPOINTLF)calloc((ULONG)iSize,sizeof(char));
      iRead = read( hfile,(PVOID)pSpl->pptl,iSize);
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadImage(int hfile,pImage pImg,WINDOWINFO *pwi)
{
   Image     Img;
   PBYTE     p;
   POBJECT   pObj;
   int       iSize;
   int       iDontRead;
   int       iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat  == FILE_DRAWIT29 ||  
       pwi->lFileFormat  == FILE_DRAWIT30 ||
       pwi->lFileFormat  == FILE_DRAWIT32 )
   {
      iSize = sizeof(Image) - iDontRead;
      p  = (PBYTE)&Img;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
       return -1; /* ERROR */

   if (iRead > 0)
   {
      memcpy(pImg,&Img,sizeof(Image));

      pImg->ustype   = CLS_IMG;
      pObj = (POBJECT)pImg;
      pObj->usClass  = CLS_IMG;
      pObj->paint       = DrawImgSegment;
      pObj->moveOutline = ImgMoveOutLine;
      pObj->getInvalidationArea   = ImgInvArea;
      pObj->pDrwPal  = NULL;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadText(int hfile, PTEXT pTxt, WINDOWINFO *pwi)
{
   TEXT  Txt;
   TXT29 Txt29;
   TXT31 Txt31;                   /* version 3.0 uptil 3.1c */
   PBYTE p;
   POBJECT pObj;
   int   iSize;
   int   iDontRead;
   int   iRead = -1;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   if (pwi->lFileFormat == FILE_DRAWIT29 )
   {
      iSize = sizeof(TXT29) - iDontRead;
      p = (PBYTE)&Txt29;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat == FILE_DRAWIT30 )
   {
      iSize = sizeof(TXT31) - iDontRead;
      p = (PBYTE)&Txt31;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else if (pwi->lFileFormat == FILE_DRAWIT32 )
   {
      /*
      ** Latest format....
      */
      iSize = sizeof(TEXT) - iDontRead;
      p = (PBYTE)&Txt;
      p += iDontRead;
      iRead = read(hfile,(PVOID)p,iSize);
   }
   else
       return -1;  /* ERROR */

   memset(&pTxt->fattrs,0,sizeof(FATTRS));

   pTxt->fattrs.usRecordLength = sizeof(FATTRS);        /* Length of record  */
   pTxt->fattrs.usCodePage     = GpiQueryCp(pwi->hps);  /* Code page         */
   pTxt->fattrs.fsFontUse      = FATTR_FONTUSE_OUTLINE; /* Outline fonts only*/

   if (iRead > 0)
   {
      if (pwi->lFileFormat == FILE_DRAWIT29 )
      {
         pTxt->ustype        = CLS_TXT;
         strcpy(pTxt->Str,Txt29.Str);            /* String element  */
         pTxt->ptl           = Txt29.ptl;         /* String place    */
         pTxt->lShadeX       = Txt29.lShadeX;     /* nr of Units in x dir    */
         pTxt->lShadeY       = Txt29.lShadeY;     /* nr of Units in y dir    */
         pTxt->bt.usLayer    = Txt29.uslayer;     /* layer it belongs to     */
         pTxt->bt.lPattern   = Txt29.lPattern;    /* filling pattern of text */
         pTxt->bt.gradient   = Txt29.gradient;    /* gradient Filling        */
         pTxt->bt.fountain   = Txt29.fountain;    /* fountain fill    */
         pTxt->bt.ptlfCenter = Txt29.ptlfCenter;  /* Rotation point   */
         pTxt->bt.fColor     = Txt29.TxtColor;    /* Text Color     */
         pTxt->bt.ShadeColor      = Txt29.ShadeColor;  /* shade color    */
         pTxt->TxtOutlineColor    = Txt29.TxtOutlineColor;
         pTxt->TxtBackGroundColor = Txt29.TxtBackGroundColor;
         pTxt->TxtShear           = Txt29.TxtShear;    /*Char shear     */
         pTxt->ShadeShear         = Txt29.ShadeShear;  /*Shadow shear   */
         pTxt->ulState            = Txt29.ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
         pTxt->TxtCircular        = Txt29.TxtCircular; /*see above definition */
         pTxt->sizfx              = Txt29.sizfx;
         pTxt->LineWidth          = Txt29.LineWidth;   /* Outline width    */
         pTxt->gradl              = Txt29.gradl;       /* Rotation vector. */
         pTxt->fRotate            = Txt29.fRotate;     /* pi rad rotated.  */

         pTxt->bt.Shade.lShadeType  = SHADE_NONE;               /* New in 3.2 */
         pTxt->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pTxt->bt.Shade.lUnits      = pwi->Shade.lUnits;

         memcpy(&pTxt->fattrs,&Txt29.fattrs,sizeof(FATTRS));
      }
      else if (pwi->lFileFormat == FILE_DRAWIT30 ) /* version 3.0 uptil 3.1c */
      {
         pTxt->ustype        = CLS_TXT;
         strcpy(pTxt->Str,Txt31.Str);                /* String element  */
         pTxt->ptl           = Txt31.ptl;            /* String place    */
         pTxt->lShadeX       = Txt31.lShadeX;        /* nr of Units in x dir    */
         pTxt->lShadeY       = Txt31.lShadeY;        /* nr of Units in y dir    */
         pTxt->bt.usLayer    = Txt31.bt.usLayer;     /* layer it belong to     */
         pTxt->bt.lPattern   = Txt31.bt.lPattern;    /* filling pattern of text */
         pTxt->bt.gradient   = Txt31.bt.gradient;    /* gradient Filling        */
         pTxt->bt.fountain   = Txt31.bt.fountain;    /* fountain fill    */
         pTxt->bt.ptlfCenter = Txt31.bt.ptlfCenter;  /* Rotation point   */
         pTxt->bt.fColor     = Txt31.bt.fColor;      /* Text Color     */
         pTxt->bt.ShadeColor = Txt31.bt.ShadeColor;  /* shade color    */
         pTxt->TxtOutlineColor    = Txt31.TxtOutlineColor;
         pTxt->TxtBackGroundColor = Txt31.TxtBackGroundColor;
         pTxt->TxtShear           = Txt31.TxtShear;    /*Char shear     */
         pTxt->ShadeShear         = Txt31.ShadeShear;  /*Shadow shear   */
         pTxt->ulState            = Txt31.ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
         pTxt->TxtCircular        = Txt31.TxtCircular; /*see above definition */
         pTxt->sizfx              = Txt31.sizfx;
         pTxt->LineWidth          = Txt31.LineWidth;   /* Outline width    */
         pTxt->gradl              = Txt31.gradl;       /* Rotation vector. */
         pTxt->fRotate            = Txt31.fRotate;     /* pi rad rotated.  */
         pTxt->bt.Shade.lShadeType  = SHADE_NONE;               /* New in 3.2 */
         pTxt->bt.Shade.lShadeColor = pwi->Shade.lShadeColor;
         pTxt->bt.Shade.lUnits      = pwi->Shade.lUnits;
         memcpy(&pTxt->fattrs,&Txt31.fattrs,sizeof(FATTRS));
      }
      else
      {
         memcpy(pTxt,&Txt,sizeof(TEXT));
      }
      pTxt->ustype =  CLS_TXT;
      pObj = (POBJECT)pTxt;
      pObj->usClass = CLS_TXT;
      pObj->paint       = DrawTextSegment;
      pObj->moveOutline = TxtMoveOutLine;
      pObj->getInvalidationArea   = TextInvArea;

      pObj->bDirty  = TRUE;     /* Used in dlg_txt.c to calc width table */
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static void _System readFile(FILETYPEH *pf)
{
   ULONG    ulBytes;
   PBYTE    p,ptmp;
   PVOID    pFileBegin;     /* beginning of file data         */
   int      iRead;          /* Number of bytes read from file.*/
   char     buf[50];
   USHORT   *usclass,*s;
   BOOL     bRead=FALSE;
   BOOL     bError = FALSE;
   HWND	    hFrame;        /* Frame windowhandle of mainwindow*/
   char     szError[150];
   char     *pszString;
   /*
   ** Must be set to zero for each import. else a second import will
   ** cause problems.
   */
   pbase = NULL;
   pend  = NULL;
   /*
   ** If there is already a chain we just append the file to this
   ** chain else we create a new one. This is only used for reconstruction
   ** work of the chain in drwgrp.c Groupreconstruct().
   */
   pFileBegin = (PVOID)buf; /* 50 bytes room */

   do
   {
      bRead = FALSE;
      iRead = read(pf->pli->handle,pFileBegin,sizeof(USHORT));

      usclass = (USHORT *)pFileBegin;

      if ( *usclass >= 1 && *usclass <= 11 && iRead)
      {
         bRead = TRUE;
         /*
         ** Allocate memory for the new type of structure
         ** A stand alone object is created.
         */
         ptmp = p = (PBYTE)pObjNew(NULL, *usclass);
         /*
         ** Jump over the non persistent part of the structure.
         */
         p += sizeof(OBJECT);
         s = (USHORT *)p;
         *s = (USHORT)*usclass;

         p += sizeof(USHORT);

         ulBytes = ObjGetSize(*usclass) - (sizeof(OBJECT) + sizeof(USHORT));

         switch(*usclass)
         {
            case CLS_IMG:
//               iRead = read(pf->pli->handle,(PVOID)p,ulBytes);
               iRead = FileLoadImage(pf->pli->handle,(pImage)ptmp,pf->pwi);
               if (! (iRead = FileGetImageData(pf->pwi,pf->pli,(pImage)ptmp) ))
               {
                  free((void *)ptmp);
                  bError = TRUE;
               }
               break;
            case CLS_SPLINE:
               iRead = FileLoadSpline(pf->pli->handle,(PSPLINE)ptmp,pf->pwi); /* conv */
               break;
            case CLS_META:
               iRead = read(pf->pli->handle,(PVOID)p,ulBytes);
               if (!FileGetMetaData(pf->pli,(POBJECT)ptmp))
               {
                  free((void *)ptmp);
                  bError = TRUE;
               }
               break;
            case CLS_TXT:
               iRead = FileLoadText(pf->pli->handle,(PTEXT)ptmp,pf->pwi); /* convert */
               break;
            case CLS_SQR:
               {
                  POBJECT pConv; /* Convertion to spline..... */
                  iRead = FileLoadSquare(pf->pli->handle,(PSQUARE)ptmp,pf->pwi,&pConv); /* convert */
                  free((void *)ptmp);
                  ptmp  = (PBYTE)pConv;
               }
               break;
            case CLS_LIN:
               iRead = FileLoadLine(pf->pli->handle,(PLINE)ptmp,pf->pwi); /* convert */
               break;
            case CLS_CIR:
               iRead = FileLoadCircle(pf->pli->handle,(PCIRCLE)ptmp,pf->pwi); /* convert */
               if (iRead < 0 ) //|| !cirCheck((POBJECT)ptmp))
               {
                  free((void *)ptmp);
                  bError = TRUE;
               }
               break;
            case CLS_BLOCKTEXT:
               iRead = FileLoadBlockText(pf->pli->handle,(PBLOCKTEXT)ptmp,pf->pwi); /* convert */
               break;
            default:
               iRead = read(pf->pli->handle,(PVOID)p,ulBytes);
               break;
         }
      }

      if (iRead > 0 && bRead && !bError)
      {
        appendobj((POBJECT)ptmp);
      }

   } while ( iRead > 0 && !bError && bRead);

   close( pf->pli->handle);
   if (pbase)
   {
      AppendChain(pbase);
      GroupReconstruct(pbase);
   }
   else
   {
      /*
      ** File could not be loaded so clear the titlebar of
      ** the application.
      */
      WinLoadString((HAB)0,
                    (HMODULE)0, 
                    (LONG)IDS_WRONGJSPFORMAT,sizeof(szError),(PSZ)szError);
      pszString = strdup(szError);
      WinPostMsg(pf->pwi->hwndClient,UM_JSPERROR,(MPARAM)pszString,(MPARAM)0);
   }
   /*
   ** Post main thread a message to tell that we are done, so it
   ** can do a repaint.
   */
   WinPostMsg(pf->pwi->hwndClient,UM_ENDDIALOG,(MPARAM)0,(MPARAM)0);
}
/*-----------------------------------------------[ public ]------------------*/
/* Name        : LoadDrwFile.                                                */
/*                                                                           */
/* Description : Sets up the read thread for loading the JSP file.           */
/*                                                                           */
/* Returns     : TRUE on success.                                            */
/*---------------------------------------------------------------------------*/
BOOL LoadDrwFile(pLoadinfo pli,WINDOWINFO *pwi)
{
   APIRET  rc;
   BOOL    bOldChain;

   /*
   ** First try to read the forminfo. Before starting the thread
   ** because of windowing functions in filegetform....
   */
   bOldChain = ObjectsAvail();
   if (!fileGetForm(pli,pwi,bOldChain))
      return FALSE;

   oldf.ThreadFlags = 0;            /* Indicate that the thread is to */
                                    /* be started immediately         */
   oldf.StackSize = PICTHREADSTACK; /* Set the size for the new       */
                                    /* thread's stack                 */
   oldf.pwi = pwi;
   oldf.pli = pli;

   rc = DosCreateThread(&oldf.ThreadID,(PFNTHREAD)readFile,(ULONG)&oldf,
                        oldf.ThreadFlags,oldf.StackSize);
   if (rc)
   {
      /*
      ** Could not start thread so close file here!
      */
      close(pli->handle);
      return FALSE;
   }
   return TRUE;
}
