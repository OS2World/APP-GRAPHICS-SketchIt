#define  INCL_WIN
#define  INCL_GPI
#define  IDD_FONT      2002
#define  MESSAGELEN    50
#define  IDD_MSGBOX    1001
#define  MAXSETID      254

#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_fnt.h"


static SIZEF  FontDlgSizfx; /* Used by blocktext to get latest selected size*/

ULONG FontIndex=0;
char  FontFaceName[150]; /*Contains the current active font name */

extern HWND hwndMFrame; /* drwmain.c*/

/*-----------------------------------------------[ public ]---------------*/
/* getFontDlgSizfx - get latest selected font size..... drwbtext.c only!  */
/*------------------------------------------------------------------------*/
BOOL getFontDlgSizfx( SIZEF *pSizef)
{   
   *pSizef = FontDlgSizfx;

   if (FontDlgSizfx.cx > 0 && FontDlgSizfx.cy > 0)
      return TRUE;
   else
      return FALSE;
}
/*-----------------------------------------------[ public  ]--------------*/
/*  getFontSetID                                                          */
/*                                                                        */
/*  Description : Return an unused set id (lcid) or GPI_ERROR for error   */
/*                (or none available).                                    */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
LONG getFontSetID(HPS hps)
{
  int    i;
  LONG   lLcid  = GPI_ERROR;
  LONG   lCount;
  BOOL   fRet = FALSE;
  PLONG  alLcids = NULL;
  PLONG  alTypes;
  PSTR8  aNames;

  lCount = GpiQueryNumberSetIds(hps); /* Query the number of set id's */

  if (!lCount)                /* if the number of id's is zero         */
     return (1L);             /* set return setid to 1.                */

  if (lCount != GPI_ALTERROR)
  {
     alLcids = (PLONG)malloc((SHORT)(16 * lCount )); /* allocate memory for */
     alTypes = (PLONG)(alLcids + lCount);            /* all id's, types and */
     aNames  = (PSTR8)(alTypes + lCount);            /* names.              */
    
     if (alLcids)
     {
        fRet = GpiQuerySetIds(hps,          /* PS handle                    */
                              lCount,       /* number of set ids queried    */
                              alTypes,      /* returned types.              */
                              aNames,       /* returned names.              */
                              alLcids);     /* returned set ids.            */
     }

     if (fRet)
     {
        for (lLcid = 1; lLcid < MAXSETID; lLcid++)
        {
           for (i = 0; (i < (int)lCount) && (alLcids[i]!= lLcid); i++);
   
           if (i == (int)lCount)
              break;
        }
        if (lLcid >= MAXSETID )
           lLcid = GPI_ERROR;
     }
     if (alLcids)
        free (alLcids);
  }
  return (lLcid);
}
/*-----------------------------------------------[ private ]---------------*/
/* CAPS_HORIZONTAL_FONT_RES                                                */
/*     Effective horizontal device resolution in pels per inch, for the    */
/*     purpose of selecting fonts.                                         */
/*                                                                         */
/* For printers, this is the actual device resolution, but for displays    */
/* it may differ from the actual resolution for reasons of legibility.     */
/*                                                                         */
/* CAPS_VERTICAL_FONT_RES                                                  */
/*     Effective vertical device resolution in pels per inch, for the      */
/*     purpose of selecting fonts.                                         */
/*-------------------------------------------------------------------------*/
static VOID ConvertVectorFontSize(HPS hps, SIZEF *psizef, PFATTRS pfattrs)
{
  HDC    hDC;
  LONG   lxFontResolution;
  LONG   lyFontResolution;
  POINTL ptl;
  POINTL ptlf;
  LONG   lcx,lcy;   /* long value */
  BOOL   bYNeg = FALSE;
  BOOL   bXNeg = FALSE;

   if (psizef->cy < 0)
   {
      psizef->cy *= -1;
      bYNeg       = TRUE;
   }

   if (psizef->cx < 0)
   {
      psizef->cx *= -1;
      bXNeg       = TRUE;
   }
  /*
   *   Query device context for the screen and then query
   *   the resolution of the device for the device context.
   */
  hDC = GpiQueryDevice(hps);
  DevQueryCaps( hDC, CAPS_HORIZONTAL_FONT_RES, (LONG)1, &lxFontResolution);
  DevQueryCaps( hDC, CAPS_VERTICAL_FONT_RES,   (LONG)1, &lyFontResolution);
  /*
   *   Calculate the size of the character box, based on the
   *   point size selected and the resolution of the device.
   *   The size parameters are of type FIXED, NOT int.
   *   NOTE: 1 point == 1/72 of an inch.
   */
  lcy  = MAKELONG( HIUSHORT( psizef->cy ), 0 );
  lcx  = MAKELONG( HIUSHORT( psizef->cx ), 0 );
  /*
  ** Get lost fraction
  */
  ptlf.x = ( psizef->cx & 0x0000FFFF );
  ptlf.y = ( psizef->cy & 0x0000FFFF );

  ptl.x = (lcx * lxFontResolution ) / 72 ;
  ptl.y = (lcy * lyFontResolution ) / 72 ;

  ptlf.x = (ptlf.x * lxFontResolution ) / 72 ;
  ptlf.y = (ptlf.y * lyFontResolution ) / 72 ;

  GpiConvert(hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);
  GpiConvert(hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptlf);

  psizef->cx = MAKEFIXED(ptl.x,0);
  psizef->cy = MAKEFIXED(ptl.y,0);
  /*
  ** Add fraction.
  */
  psizef->cx += ptlf.x;
  psizef->cy += ptlf.y;

  pfattrs->lMaxBaselineExt = MAKELONG( HIUSHORT( psizef->cy ), 0 );
  pfattrs->lAveCharWidth   = MAKELONG( HIUSHORT( psizef->cx ), 0 );

  if (bYNeg)
     psizef->cy *= -1;

  if (bXNeg)
     psizef->cx *= -1;
  
}   /* end ConvertVectorPointSize() */
/*---------------------------FontDlg--------------------------------*/
BOOL FontDlg(WINDOWINFO * pwi,FATTRS *pF,BOOL bObj)
{
   FONTDLG fontDlg;
   CHAR szTitle[MESSAGELEN];
   CHAR szFamily[CCHMAXPATH];
   FATTRS fattrs;
   SIZEF  sizfx;
   FONTMETRICS fm;

   setFont(pwi,pF,pwi->sizfx);
   GpiQueryFontMetrics(pwi->hps,sizeof(FONTMETRICS),&fm);

   memset(&fontDlg, 0, sizeof(fontDlg));            /* initialize all fields */

   fontDlg.cbSize     = sizeof(FONTDLG);                  /* sizeof(FONTDLG) */
   fontDlg.hpsScreen  = WinGetScreenPS(HWND_DESKTOP);  /* Screen presentation space */
   fontDlg.hpsPrinter = NULLHANDLE;            /* Printer presentation space */

   if (bObj)
      strcpy(szTitle,"FontDialog for selected text object");
   else
      strcpy(szTitle,"DrawIt FontDialog");

   strcpy(szFamily, fm.szFamilyname); /* Family name of font   */

   fontDlg.fxPointSize  = pwi->fxPointsize;
   fontDlg.pszTitle     = szTitle;       /* Application supplied title      */
   fontDlg.pszPreview   = "DrawIt JDK-Software";   /* String to print in preview wndw */
   fontDlg.pszPtSizeList = NULL;          /* Application provided size list  */
   fontDlg.pfnDlgProc    = NULL;          /* Dialog subclass procedure       */
   fontDlg.pszFamilyname = szFamily;      /* point to Family name of font    */
   fontDlg.fl           = FNTS_CENTER |   /* FNTS_* flags - dialog styles    */
                          FNTS_VECTORONLY;
   fontDlg.flFlags      = 0;              /* FNTF_* state flags              */
   fontDlg.flType       = (LONG)pF->fsType;
   fontDlg.flTypeMask   = 0;              /* Mask of which font types to use */
   fontDlg.flStyle      = 0;              /* The selected style bits         */
   fontDlg.flStyleMask  = 0;              /* Mask of which style bits to use */
   fontDlg.clrFore      = CLR_NEUTRAL;    /* Selected foreground color       */
   fontDlg.clrBack      = CLR_BACKGROUND; /* Selected background color       */
   fontDlg.ulUser       = 0;              /* Blank field for application     */
   fontDlg.lReturn      = 0;              /* Return Value of the Dialog      */
   fontDlg.lEmHeight         = 0;
   fontDlg.lXHeight          = 0;
   fontDlg.lExternalLeading  = 0;
   fontDlg.sNominalPointSize = 0;
   fontDlg.usWeight          = 0; /* The boldness of the font */
   fontDlg.usWidth           = 0; /* The width of the font     */
   fontDlg.x            = 0;              /* X coordinate of the dialog      */
   fontDlg.y            = 0;              /* Y coordinate of the dialog      */
   fontDlg.usDlgId      = IDD_FONT;       /* ID of a custom dialog template  */
   fontDlg.usFamilyBufLen = sizeof(szFamily); /*Length of family name buffer */
   /*
    *   Bring up the standard Font Dialog
    */

   if(WinFontDlg(HWND_DESKTOP, hwndMFrame, &fontDlg) != DID_OK)
   {
      WinReleasePS(fontDlg.hpsScreen);
      return FALSE;
   }
   WinReleasePS(fontDlg.hpsScreen);
   /*
   ** If the use pressed the ESC key we still got here.
   ** so check if the fontname is filled in.
   */
   if (!fontDlg.fAttrs.szFacename[0])
   {
      return FALSE;
   }
   /*
    *   If outline font, calculate the maxbaselineext and
    *   avecharwidth for the point size selected
    */
   memcpy(&fattrs,&fontDlg.fAttrs,sizeof(FATTRS));
   memcpy(pF,&fattrs,sizeof(FATTRS));

   /* Save the font for later use in the textsegments */

   /*
   ** Set the global var FontFaceName to use it later when
   ** it is stored in the text object when it is closed!
   */
   sizfx.cx = fontDlg.fxPointSize;
   sizfx.cy = fontDlg.fxPointSize;

   FontDlgSizfx = sizfx; /* Put in the local static, later used by blocktext */

   strcpy(FontFaceName,fontDlg.fAttrs.szFacename);

   /*
   ** Copy the name in the windowinfo struct,
   ** The statusline needs it!!
   ** If we were not modifying the font of the selected object.
   */
   if (!bObj)
      sprintf(pwi->fontname,"%d.%s",(fontDlg.fxPointSize >> 16),fontDlg.fAttrs.szFacename);

   /*
   ** If we are called from the main module we should update
   ** charbox size in the windowinfo struct.
   */
   if (pwi && !bObj)
   {
      memcpy(&pwi->fattrs,&fattrs,sizeof(FATTRS));

      pwi->sizfx.cx = sizfx.cx;
      pwi->sizfx.cy = sizfx.cy;
      pwi->fxPointsize = fontDlg.fxPointSize;
      setFont(pwi,&pwi->fattrs,pwi->sizfx);
   }
   return TRUE;
}   /* End of SetFont()*/
/* ----------------------------------------------[ Private ]------------*/
/*  SelectScalableFont							*/
/*                        						*/
/*     This function is	used to	determine the lMatch component for	*/
/*     a requested vector font.						*/
/*									*/
/*     Upon Entry:							*/
/*									*/
/*     HPS   hPS;	  = Presentation Space				*/
/*     CHAR *pszFacename; = Font Face Name				*/
/*									*/
/*     Upon Exit:							*/
/*									*/
/*     SelectScalableFont = lMatch Number for Requested	Font		*/
/*									*/
/* --------------------------------------------------------------------	*/
LONG SelectScalableFont(HPS hPS, CHAR *pszFacename)
{
LONG	     cFonts;		   /* Fonts Count			*/
LONG	     lFontsTotal = 0L;	   /* Fonts Total Count			*/
LONG	     lMatch = 1L;	   /* Font Match Value			*/
PFONTMETRICS pfm;		   /* Font Metrics Pointer		*/
register INT i;			   /* Loop Counter			*/

		       /* Get the number of fonts for the face name	*/
		       /* provided					*/
pfm = NULL;

DosAllocMem((PPVOID)(PVOID)&pfm, 
            (ULONG)(sizeof(FONTMETRICS) *  
            (cFonts = GpiQueryFonts(hPS, QF_PUBLIC, pszFacename, &lFontsTotal,
	    sizeof(FONTMETRICS), (PFONTMETRICS)NULL))), PAG_READ | PAG_WRITE |	PAG_COMMIT);

		       /* Make a pointer for the memory	allocated for	*/
		       /* the font metrics and get the font metrics for	*/
		       /* the number of	fonts for the face name		*/
		       /* provided					*/

GpiQueryFonts(hPS, QF_PUBLIC, pszFacename, &cFonts,
	      sizeof(FONTMETRICS), pfm);

		       /* Loop through the font	metrics	returned to	*/
		       /* locate the desired font by matching the x and	*/
		       /* y device resolution of the font and the point	*/
		       /* size						*/

for ( i	= 0; i < (INT)cFonts; i++ )
   if (	(pfm[i].sXDeviceRes == 1000) &&	(pfm[i].sYDeviceRes == 1000) )
       {
		       /* Font found, get the match value to allow the	*/
		       /* exact	font to	be selected by the calling	*/
		       /* application					*/

       lMatch =	pfm[i].lMatch;
       break;
       }
		       /* Release the memory allocated for the font	*/
		       /* metrics array					*/
DosFreeMem(pfm);
		       /* Return the match value to the	calling		*/
		       /* application					*/
return(lMatch);
}
/*------------------------------------------------------------------------*/
void setFont(WINDOWINFO * pwi,PFATTRS pfattrs, SIZEF sizefx)
{
   GpiDeleteSetId(pwi->hps, pwi->lcid);
   GpiSetCharSet (pwi->hps, LCID_DEFAULT);
   ConvertVectorFontSize(pwi->hps,&sizefx,pfattrs);
   pfattrs->lMatch = SelectScalableFont(pwi->hps,pfattrs->szFacename);
   GpiCreateLogFont(pwi->hps,(PSTR8)pfattrs->szFacename,pwi->lcid,pfattrs);
   GpiSetCharSet(pwi->hps,pwi->lcid);

   if (pwi->ulUnits == PU_PELS && pwi->uXfactor > 1.0 )
   {
      /*
      ** Conversion to a bitmap !!
      ** See drwimg.c for factor calculation.
      */
      sizefx.cx *= pwi->uXfactor;
      sizefx.cy *= pwi->uYfactor;
   }
   GpiSetCharBox(pwi->hps,&sizefx);
}
/*------------------------------------------------------------------------*/
/* InitFont: Is only called when the appl is started up. This must be done*/
/* because if the user starts moving the text a fontid is used to set the */
/* charset. Evenso this id is set in the Textsegment which is closed      */
/* before the font dialog is used etc.                                    */
/* Input : HPS      : presentation space                                  */
/*         Facename : Facename of the font to be displayed.               */
/*         Reason   : FONT_INIT only used at startuptime.                 */
/*                    FONT_SET  to set the font.                          */
/* sizefx.cx & sizefx.cy should be delivered as fixed int!!!              */
/*------------------------------------------------------------------------*/
void FontInit(HPS hps, PSZ facename, USHORT reason, SIZEF * pSizefx)
{
   FATTRS fattrs;
   FIXED  fxpointsize;

   fattrs.usRecordLength    = sizeof(FATTRS);
   fattrs.fsSelection       = 0;
   fattrs.lMatch            = 0;
   fattrs.idRegistry        = 0;
   fattrs.usCodePage        = GpiQueryCp(hps);
   fattrs.fsType            = 0;
   fattrs.fsFontUse         = FATTR_FONTUSE_OUTLINE;
   fattrs.lMaxBaselineExt   = 0;
   fattrs.lAveCharWidth     = 0;

   if (reason == FONT_INIT)
   {
      /*
      ** Font init is called in the main module at the WM_CREATE without
      ** facename and with facename in the WM_PRESPARAM message of the
      ** font pallette.
      */
      GpiDeleteSetId(hps, 3L);
      GpiSetCharSet(hps, LCID_DEFAULT);

      if (!facename)
      {
         strcpy (fattrs.szFacename, "Courier");
         strcpy (FontFaceName,"Courier");
         fxpointsize = MAKEFIXED(12,0);
      }
      else
      {
         strcpy(fattrs.szFacename,facename); /* From fontpallette */
         strcpy (FontFaceName,facename);
         fxpointsize = MAKEFIXED(pSizefx->cx,0);
      }

      pSizefx->cy = fxpointsize;
      pSizefx->cx = fxpointsize;

      ConvertVectorFontSize(hps,pSizefx, &fattrs);

      GpiCreateLogFont(hps,(PSTR8)fattrs.szFacename,3L,&(fattrs));

      GpiSetCharSet(hps,3L);
      GpiSetCharBox(hps,pSizefx);
   }
   else if (reason == FONT_SET)
   {
      strcpy (fattrs.szFacename, facename);
      ConvertVectorFontSize(hps,pSizefx, &fattrs );
      GpiCreateLogFont(hps,(PSTR8)fattrs.szFacename,2L,&(fattrs));
      GpiSetCharSet(hps,2L);
   }
}
