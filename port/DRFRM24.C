/*------------------------------------------------------------------------*/
/*  Name: drwfrm24.c                                                      */
/*                                                                        */
/*  Description : Functions for loading the old 2.4 file.                 */
/*                                                                        */
/*                                                                        */
/* Private functions:                                                     */
/*   appendobj      : Appends an object to the local chain of objects     */
/*   delobj         : Removes an object from the local chain.             */
/*   delChain       : Removes complete chain when old file cannot be read */
/*   FileLoadSquare : Reads a square from the old file.                   */
/*   FileLoadCircle : Reads a circle from the old file.                   */
/*   FileLoadSpline : Reads a spline from the old file.                   */
/*   FileLoadText   : Reads text from from file.                          */
/*   readFile       : This function reads the complete file.              */
/*                                                                        */
/* Public functions :                                                     */
/*   ReadOldFile    : Collects some pointers and starts the load thread.  */
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
#include "drwcanv.h"
#define PICTHREADSTACK 16384
/*
** Thread variables for multithreaded loading of a pic file.
*/
TID         ThreadID;       /* New thread ID (returned)                   */
ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
ULONG       StackSize;      /* Size in bytes of new thread's stack        */

static POBJECT pbase,pend;

/*
** Helper struct to be used for multithreading.
*/
typedef struct oldfiletype
{
   WINDOWINFO *pwi;
   pLoadinfo pli;
} OLDFILETYPEH;

static OLDFILETYPEH oldf;
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
   }
   return;
}
/*-------------------------------------------------------------------------*/
#if 0 /* NOT USED!!!!! */
static void delobj(POBJECT pObj)
{
   POBJECT TmpPrev,TmpNext;

   if (!pObj)
      return;

   TmpNext = pObj->Next;
   TmpPrev = pObj->Previous;

   if (TmpPrev && TmpNext)
   {
      TmpPrev->Next     = TmpNext;
      TmpNext->Previous = TmpPrev;
   }
   else if (TmpPrev && !TmpNext)
   {
      /*
      ** End of chain so bring back the endof chain pointer.
      */
      pend = pend->Previous;
      TmpPrev->Next = NULL;
   }
   else if (!TmpPrev && TmpNext)
   {
      TmpNext->Previous = NULL;
      pbase = TmpNext;
   }
   else if (!TmpPrev && !TmpNext)
   {
      pbase = NULL;
   }

   free((void *)pObj);
   pObj = NULL;
   return;
}
#endif
/*-------------------------------------------------------------------------*/
static FileLoadSquare(int hfile, PSQUARE pSqr, WINDOWINFO *pwi)
{
   static Sqr24 sqr;
   PBYTE  p;
   int iSize;
   int iDontRead;
   int iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));
   iSize = sizeof(Sqr24) - iDontRead;
   p = (PBYTE)&sqr;
   p += iDontRead;     /* jump over ustype & object */
   iRead = read(hfile,(PVOID)p,iSize);

   if (iRead > 0)
   {
      pSqr->line           = sqr.SqrLine;
      pSqr->ustype         = CLS_SQR;
      memcpy(pSqr->ptlf,&sqr.ptlf,(4 * sizeof(POINTLF)));
      pSqr->ptlSqrCenter   = sqr.ptlSqrCenter;
      pSqr->lPattern       = sqr.lPattern;
      pSqr->fColor         = sqr.fColor;
      pSqr->gradient       = sqr.SqrGrad;
      pSqr->uslayer        = sqr.uslayer;
      pSqr->ptlfCenter     = sqr.ptlfCenter;
      pSqr->fountain       = pwi->fountain;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static FileLoadCircle(int hfile, PCIRCLE pCir, WINDOWINFO *pwi)
{
   static Cir24 cir;
   PBYTE  p;
   int iSize;
   int iDontRead;
   int iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   iSize = sizeof(Cir24) - iDontRead;
   p = (PBYTE)&cir;
   p += iDontRead;
   iRead = read(hfile,(PVOID)p,iSize);

   if (iRead > 0)
   {
      pCir->ustype       = CLS_CIR;
      pCir->m_bClose     = TRUE;
      pCir->StartAngle   = cir.StartAngle;
      pCir->SweepAngle   = cir.SweepAngle;
      pCir->line         = cir.CirLine;
      pCir->fColor       = cir.fColor;
      pCir->Rotate       = cir.Rotate;
      pCir->uslayer      = cir.uslayer;
      pCir->ptlPosn      = cir.ptlPosn;
      pCir->lPattern     = cir.lPattern;
      pCir->gradient     = cir.CirGrad;
      pCir->arcpParms    = cir.arcpParms;
      pCir->ptlfCenter   = cir.ptlfCenter;
      pCir->fountain     = pwi->fountain;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static FileLoadSpline(int hfile, PSPLINE pSpl, WINDOWINFO *pwi)
{
   static Spl24 spl;
   PBYTE  p;
   int iSize;
   int iDontRead;
   int iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));

   iSize = sizeof(Spl24) - iDontRead;
   p = (PBYTE)&spl;
   p += iDontRead;
   iRead = read(hfile,(PVOID)p,iSize);
   if (iRead > 0)
   {
      pSpl->ustype            = CLS_SPLINE;
      pSpl->line              = spl.SplLine;
      pSpl->nrpts             = spl.nrpts;
      pSpl->lPattern          = spl.lPattern;
      pSpl->gradient          = spl.SplGrad;
      pSpl->fColor            = spl.fColor;
      pSpl->uslayer           = spl.uslayer;
      pSpl->LinDeleted        = spl.LinDeleted;
      pSpl->LinMultiSelected  = spl.LinMultiSelected;
      pSpl->ulState           = spl.ulState;
      pSpl->pptl              = spl.pptl;/*????????*/
      pSpl->ptlfCenter        = spl.ptlfCenter;
      pSpl->fountain          = pwi->fountain;

      iSize = pSpl->nrpts * sizeof(POINTLF);
      pSpl->pptl = (PPOINTLF)calloc((ULONG)iSize,sizeof(char));
      iRead = read( hfile,(PVOID)pSpl->pptl,iSize);
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static int FileLoadText(int hfile, PTEXT pTxt, WINDOWINFO *pwi)
{
   static Text24 TxtOldType;
   PBYTE  p;
   int iSize;
   int iDontRead;
   int iRead;
   /*
   ** The class info is already loaded from file.
   ** So we load the stuff without the ustype.
   */
   iDontRead = (sizeof(OBJECT) + sizeof(USHORT));
   iSize = sizeof(Text24) - iDontRead;
   p = (PBYTE)&TxtOldType;
   p += iDontRead;
   iRead = read(hfile,(PVOID)p,iSize);

   memset(&pTxt->fattrs,0,sizeof(FATTRS));

   pTxt->fattrs.usRecordLength = sizeof(FATTRS);        /* Length of record  */
   pTxt->fattrs.usCodePage     = GpiQueryCp(pwi->hps);  /* Code page         */
   pTxt->fattrs.fsFontUse      = FATTR_FONTUSE_OUTLINE; /* Outline fonts only*/

   if (iRead == iSize)
   {
      pTxt->ustype =  CLS_TXT;
      strcpy(pTxt->Str,TxtOldType.Str);     /* String element              */
      pTxt->ptl      = TxtOldType.ptl;      /* String place floating point */
      pTxt->lShadeX  = TxtOldType.lShadeX;  /* nr of Units in x dir        */
      pTxt->lShadeY  = TxtOldType.lShadeY;  /* nr of Units in y dir        */
      pTxt->uslayer  = TxtOldType.uslayer;  /* layer it belongs to         */
      pTxt->lPattern = TxtOldType.lPattern; /* filling pattern of text     */
      pTxt->gradient = TxtOldType.TxtGrad;  /* gradient Filling            */
      strcpy(pTxt->fattrs.szFacename,TxtOldType.facename);
      pTxt->TxtColor   = TxtOldType.TxtColor;   /* Text Color     */
      pTxt->ShadeColor = TxtOldType.ShadeColor; /* shade color    */
      pTxt->TxtOutlineColor = TxtOldType.TxtOutlineColor;
      pTxt->TxtBackGroundColor = TxtOldType.TxtBackGroundColor;
      pTxt->TxtShear   = TxtOldType.TxtShear;      /*Char shear     */
      pTxt->ShadeShear = TxtOldType.ShadeShear;    /*Shadow shear   */
      pTxt->ulState    = TxtOldType.ulState;       /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
      pTxt->TxtCircular= TxtOldType.TxtCircular;   /*see definition */
      pTxt->sizfx      = TxtOldType.sizfx;
      pTxt->LineWidth  = TxtOldType.LineWidth;   /* Outline width    */
      pTxt->ptlfCenter = TxtOldType.ptlfCenter;  /* Rotation point   */
      pTxt->gradl      = TxtOldType.gradl;    /* Rotation vector. */
      pTxt->fRotate    = TxtOldType.fRotate;     /* pi rad rotated.  */
      pTxt->fountain   = pwi->fountain;
   }
   return iRead;
}
/*-------------------------------------------------------------------------*/
static void _System readFile(OLDFILETYPEH *pf)
{
   ULONG    ulBytes;
   PBYTE    p,ptmp;
   PVOID    pFileBegin;     /* beginning of file data         */
   int      iRead;          /* Number of bytes read from file.*/
   char     buf[50];
   USHORT   *usclass,*s;
   BOOL     bRead=FALSE;
   BOOL     bError = FALSE;


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

      if ( *usclass >= 1 && *usclass <= 10 && iRead)
      {
         bRead = TRUE;
         /*
         ** Allocate memory for the new type of structure
         ** A stand alone object is created.
         */
         ptmp = p = (PBYTE)pObjNew(*usclass);
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
               iRead = read(pf->pli->handle,(PVOID)p,ulBytes);
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
               iRead = FileLoadSquare(pf->pli->handle,(PSQUARE)ptmp,pf->pwi); /* convert */
               break;
            case CLS_CIR:
               iRead = FileLoadCircle(pf->pli->handle,(PCIRCLE)ptmp,pf->pwi); /* convert */
               if (iRead < 0 || !cirCheck((POBJECT)ptmp))
               {
                  free((void *)ptmp);
                  bError = TRUE;
               }
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
   AppendChain(pbase);
   GroupReconstruct(pbase);
   /*
   ** Post main thread a message to tell that we are done, so it
   ** can do a repaint.
   */
   WinPostMsg(pf->pwi->hwndClient,UM_ENDDIALOG,(MPARAM)0,(MPARAM)0);
}

BOOL ReadOldFile(pLoadinfo pli,WINDOWINFO *pwi)
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

   ThreadFlags = 0;                /* Indicate that the thread is to */
                                   /* be started immediately         */
   StackSize = PICTHREADSTACK;     /* Set the size for the new       */
                                   /* thread's stack                 */
   oldf.pwi = pwi;
   oldf.pli = pli;

   rc = DosCreateThread(&ThreadID,(PFNTHREAD)readFile,(ULONG)&oldf,
                        ThreadFlags,StackSize);
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
