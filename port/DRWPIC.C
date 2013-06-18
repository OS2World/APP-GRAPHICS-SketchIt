/*------------------------------------------------------------------------*/
/*  Name: drwpic.c                                                        */
/*                                                                        */
/*  Description : Converts a lotus pic file to an OS/2 Metafile.          */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_GPI
#define INCL_WIN
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <io.h>
#include <string.h>
#include <memory.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_fnt.h"
#include "drwutl.h"
#include "drwmeta.h"
#include "dlg_hlp.h"
#include "resource.h"

#define PICTHREADSTACK 16384
/*
** constants as specified by Lotus 
*/
#define HEADERSIZE    17
#define PICFORMWIDTH  3200 
#define PICFORMHEIGHT 2311 
/*
** My constants.
*/
#define PICFONTID     4L
#define PICFONT       "Courier"
#define PICFONT1      "Courier Italic"
#define PICLINTYPE    LINETYPE_SOLID
#define PICFILLING    PATSYM_SOLID
#define MAXERROR      200
#define MAXSETID      254
static unsigned char picheader[]={ 0x01,0x00,0x00,0x00,0x01,0x00,
                                   0x08,0x00,0x44,0x00,0x00,0x00,
                                   0x00,0x0c,0x7f,0x09,0x06};

static GRADIENTL agradl[2] = { 100,0,      /* horizontal text */
                               0,100};     /* vertical text   */
                             
static char *picFillPattern[] = { "Solid","Halftone","Horizontal",
                                  "Diagonal","Blank"};

static char *picLineType[] = { "Solid","Dash","Dot"};

typedef struct _picdef
{
   pMetaimg pMeta;
   WINDOWINFO *pwi;
   LONG lfont1;
   LONG lfont2;
   long linetype;
   long filling;
   int  hfile;
   char szfontname[100];
   char szfontnam1[100];
}PICDEF;


static PICDEF picdef; /* used to contain the picture defaults */
/*
** Thread variables for multithreaded loading of a pic file.
*/
TID         ThreadID;       /* New thread ID (returned)                   */
ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
ULONG       StackSize;      /* Size in bytes of new thread's stack        */
/*-----------------------------------------------[ private ]--------------*/
/*  picSetDefaults.                                                       */
/*                                                                        */
/*  Description : Sets the defaults to be used when converting the PIC    */
/*                file to an OS/2 Metafile.                               */
/*                                                                        */
/*  Parameters  : NONE.                                                   */
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
static void picSetDefaults(WINDOWINFO *pwi)
{
   strcpy(picdef.szfontname,PICFONT);
   strcpy(picdef.szfontnam1,PICFONT1);
   picdef.linetype = PICLINTYPE;
   picdef.filling  = PICFILLING;
   picdef.lfont1   = 0;
   picdef.lfont2   = 0;
   picdef.pwi      = pwi;
   picdef.pMeta    = NULL;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picGetFontname.                                                       */
/*                                                                        */
/*  Description : Get only the fontname to be used in the pic file        */
/*                conversion. Use for this the standard fontdialog.       */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
static void picGetFontName(HWND hOwner, char *pszFontname)
{
   FONTDLG fontDlg;
   HPS     hps;
   FONTMETRICS fontMetrics;
   CHAR szTitle[100];
   CHAR szFamily[CCHMAXPATH];

   static fxPointSize = 0;            /* keep track of this for vector fonts */

 
   hps = WinGetScreenPS(HWND_DESKTOP);

   memset(&fontDlg, 0, sizeof(fontDlg));      /* initialize all fields */
   /*
   ** Get the current font attributes
   */
   GpiQueryFontMetrics(hps, sizeof(FONTMETRICS), &fontMetrics);
   /*
   ** Initialize the FONTDLG structure with the current font
   */
   fontDlg.cbSize     = sizeof(FONTDLG);                  /* sizeof(FONTDLG) */
   fontDlg.hpsScreen  = hps;                   /* Screen presentation space  */
   fontDlg.hpsPrinter = NULLHANDLE;            /* Printer presentation space */

   strcpy(szTitle,"PIC FontDialog");
   strcpy(szFamily, fontMetrics.szFamilyname); /* Family name of font        */
   fontDlg.fxPointSize  = fxPointSize;    /* Point size the user selected    */

   fontDlg.pszTitle      = szTitle;       /* Application supplied title      */
   fontDlg.pszPreview    = NULL;          /* String to print in preview wndw */
   fontDlg.pszPtSizeList = NULL;          /* Application provided size list  */
   fontDlg.pfnDlgProc    = NULL;          /* Dialog subclass procedure       */
   fontDlg.pszFamilyname = szFamily;      /* point to Family name of font    */
   fontDlg.fl           = FNTS_CENTER |   /* FNTS_* flags - dialog styles    */
			   FNTS_INITFROMFATTRS | FNTS_VECTORONLY;
   fontDlg.flFlags      = 0;              /* FNTF_* state flags              */
   fontDlg.flType       = (LONG) fontMetrics.fsType;
   fontDlg.flTypeMask   = 0;              /* Mask of which font types to use */
   fontDlg.flStyle      = 0;              /* The selected style bits         */
   fontDlg.flStyleMask  = 0;              /* Mask of which style bits to use */
   fontDlg.clrFore      = CLR_BLACK;      /* Selected foreground color       */
   fontDlg.clrBack      = CLR_WHITE;      /* Selected background color       */
   fontDlg.ulUser       = 0;              /* Blank field for application     */
   fontDlg.lReturn      = 0;              /* Return Value of the Dialog      */
   fontDlg.lEmHeight    = 0;              /* Em height of the current font   */
   fontDlg.lXHeight     = 0;              /* X height of the current font    */
   fontDlg.lExternalLeading = 0;          /* External Leading of font        */
   fontDlg.sNominalPointSize = fontMetrics.sNominalPointSize;
   fontDlg.usWeight = fontMetrics.usWeightClass; /* The boldness of the font */
   fontDlg.usWidth = fontMetrics.usWidthClass;  /* The width of the font     */
   fontDlg.x            = 0;              /* X coordinate of the dialog      */
   fontDlg.y            = 0;              /* Y coordinate of the dialog      */
   fontDlg.usDlgId      = 0;              /* ID of a custom dialog template  */
   fontDlg.usFamilyBufLen = sizeof(szFamily); /*Length of family name buffer */
   /*
    *   Bring up the standard Font Dialog
    */

   if(WinFontDlg(HWND_DESKTOP,hOwner,&fontDlg) == DID_OK)
      strcpy (pszFontname, fontDlg.fAttrs.szFacename );
 
   WinReleasePS(fontDlg.hpsScreen);
   return;
}
/*-----------------------------------------------[ private ]--------------*/
/*  swap.                                                                 */
/*                                                                        */
/*  Description : Byte swapper.                                           */
/*                                                                        */
/*  Parameters  : USHORT *- pointer containing the two bytes to be swapped*/
/*                                                                        */
/*  Returns     : NONE.                                                   */
/*------------------------------------------------------------------------*/
static void swap(SHORT *usVal)
{
   USHORT a,b;

   a = *usVal & 0x00FF;
   a <<= 8;
   b = *usVal & 0xFF00;
   b >>=8;
   a |= b;
   *usVal = (SHORT)a;
}
/**
*** TEMPORARY here.
**/
VOID picVectorFontSize(HPS hps, SIZEF *psizef, PFATTRS pfattrs)
{
  HDC   hDC;
  LONG  lxFontResolution;
  LONG  lyFontResolution;
  POINTL ptl;

  /*
   *   Query device context for the screen and then query
   *   the resolution of the device for the device context.
   */
  hDC = GpiQueryDevice(hps);
  DevQueryCaps( hDC, CAPS_HORIZONTAL_FONT_RES, (LONG)1, &lxFontResolution);
  DevQueryCaps( hDC, CAPS_VERTICAL_FONT_RES, (LONG)1, &lyFontResolution);

  /*
   *   Calculate the size of the character box, based on the
   *   point size selected and the resolution of the device.
   *   The size parameters are of type FIXED, NOT int.
   *   NOTE: 1 point == 1/72 of an inch.
   */

  ptl.x = (psizef->cx * lxFontResolution ) / 72 ;
  ptl.y = (psizef->cy * lyFontResolution ) / 72 ;

  GpiConvert(hps,CVTC_DEVICE,CVTC_DEFAULTPAGE,1,&ptl);

  psizef->cx = (FIXED)(ptl.x);
  psizef->cy = (FIXED)(ptl.y);

  pfattrs->lMaxBaselineExt = MAKELONG( HIUSHORT( psizef->cy ), 0 );
  pfattrs->lAveCharWidth   = MAKELONG( HIUSHORT( psizef->cx ), 0 );

}   /* end ConvertVectorPointSize() */
/*-----------------------------------------------[ private ]--------------*/
/*  picColor.     opcode 0xBn where n is the color number.                */
/*                                                                        */
/*  Description : Sets the color for subsequent drawing.                  */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picColor(HPS hps, unsigned char opCode)
{
   LONG lColor = 0L;

   lColor = (LONG)(opCode - 0xB0);
   if (!lColor) lColor = CLR_BLACK;
   return (BOOL)GpiSetColor(hps,lColor);
}
/*-----------------------------------------------[ private ]--------------*/
/*  picMove.      opcode 0xA0                                             */
/*                                                                        */
/*  Description : Sets the current position within the area (3200*2311)   */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picMove(int hfile, HPS hps, unsigned char opCode)
{
   SHORT x,y;
   POINTL ptl;
   int    i;

   i = read(hfile,&x,sizeof(USHORT));
   if (i < 0) 
      return FALSE;
   swap(&x);
   i = read(hfile,&y,sizeof(USHORT));
   if (i < 0) 
      return FALSE;
   swap(&y);
   ptl.x = (LONG)x;
   ptl.y = (LONG)y;
   GpiMove(hps,&ptl);
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picDraw.      opcode 0xA2                                             */
/*                                                                        */
/*  Description : Draw a line to location X,Y.                            */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picDraw(int hfile, HPS hps, unsigned char opCode)
{
   SHORT x,y;
   POINTL ptl;
   int i;
   GpiSetLineType(hps,picdef.linetype);

   i = read(hfile,&x,sizeof(USHORT));
   if (i < 0) return FALSE;
   swap(&x);
   i = read(hfile,&y,sizeof(USHORT));
   if (i < 0) return FALSE;
   swap(&y);
   ptl.x = (LONG)x;
   ptl.y = (LONG)y;
   return (BOOL)GpiLine(hps,&ptl);
}
/*-----------------------------------------------[ private ]--------------*/
/*  picFill.      opcode 0x30                                             */
/*                                                                        */
/*  Description : Draw a filled polygon.                                  */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picFill(int hfile, HPS hps, unsigned char opCode)
{
   SHORT x,y;
   unsigned char cPoints; /* number of points-1 */
   unsigned char i;
   int iRead;

   POINTL ptl;
   POINTL ptlStart;

   GpiSetLineType(hps,picdef.linetype);
   GpiSetPattern(hps,picdef.filling);

   iRead = read(hfile,&cPoints,sizeof(unsigned char));
   if (iRead < 0) return FALSE;

   GpiBeginArea(hps,BA_BOUNDARY | BA_ALTERNATE);
   /*
   ** Get starting point
   */
   iRead = read(hfile,&x,sizeof(USHORT));
   if (iRead < 0) return FALSE;
   swap(&x);
   iRead = read(hfile,&y,sizeof(USHORT));
   if (iRead < 0) return FALSE;
   swap(&y);
   ptlStart.x = (LONG)x;
   ptlStart.y = (LONG)y;
   GpiMove(hps,&ptlStart);

   for ( i = 1; i <= cPoints; i++)
   {
      read(hfile,&x,sizeof(USHORT));
      swap(&x);
      read(hfile,&y,sizeof(USHORT));
      swap(&y);
      ptl.x = (LONG)x;
      ptl.y = (LONG)y;
      GpiLine(hps,&ptl);
   }

   GpiLine(hps,&ptlStart);
   GpiCloseFigure(hps);
   GpiEndArea(hps);

   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picFillOutl   opcode 0xD0                                             */
/*                                                                        */
/*  Description : Draw a filled polygon with outline.                     */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picFillOutl(int hfile, HPS hps, unsigned char opCode)
{
   SHORT x,y;
   unsigned char cPoints; /* number of points-1 */
   unsigned char i;

   POINTL ptl;
   POINTL ptlStart;

   GpiSetLineType(hps,picdef.linetype);
   GpiSetPattern(hps,picdef.filling);

   read(hfile,&cPoints,sizeof(unsigned char));

   GpiBeginArea(hps,BA_BOUNDARY | BA_ALTERNATE);

   /*
   ** Get starting point
   */
   read(hfile,&x,sizeof(USHORT));
   swap(&x);
   read(hfile,&y,sizeof(USHORT));
   swap(&y);
   ptlStart.x = (LONG)x;
   ptlStart.y = (LONG)y;
   GpiMove(hps,&ptlStart);

   for ( i = 1; i <= cPoints; i++)
   {
      read(hfile,&x,sizeof(USHORT));
      swap(&x);
      read(hfile,&y,sizeof(USHORT));
      swap(&y);
      ptl.x = (LONG)x;
      ptl.y = (LONG)y;
      GpiLine(hps,&ptl);
   }

   GpiLine(hps,&ptlStart);
   GpiCloseFigure(hps);
   GpiEndArea(hps);

   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picSize.      opcode 0xAC                                             */
/*                                                                        */
/*  Description : Set the size of subsequently drawn text.                */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picSize(int hfile,HPS hps,SIZEF *pSizefx)
{
   SHORT cx,cy;
   int i;
   /*
   ** Get size.
   */
   i = read(hfile,&cx,sizeof(USHORT));
   if (i < 0) return FALSE;
   swap(&cx);
   i = read(hfile,&cy,sizeof(USHORT));
   if (i < 0) return FALSE;
   swap(&cy);
   pSizefx->cx = MAKEFIXED(cx,0);
   pSizefx->cy = MAKEFIXED(cy,0);
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picFont.      opcode 0xA7                                             */
/*                                                                        */
/*  Description : Sets the font to use when drawing text.                 */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picFont(int hfile,HPS hps, unsigned char opCode,SIZEF *pSizefx)
{
   unsigned char n;
   int i;
   FATTRS fattrs;
   FIXED  dfxpointsize;

   fattrs.usRecordLength    = sizeof(FATTRS);
   fattrs.fsSelection       = 0;
   fattrs.lMatch            = 0;
   fattrs.idRegistry        = 0;
   fattrs.usCodePage        = GpiQueryCp(hps);
   fattrs.fsType            = 0;
   fattrs.fsFontUse         = FATTR_FONTUSE_OUTLINE;
   fattrs.lMaxBaselineExt   = 0;
   fattrs.lAveCharWidth     = 0;
   /*
   ** Get font number.
   */
   i = read(hfile,&n,sizeof(unsigned char));


   if (!n)
   {
      if (picdef.lfont1)
      {
         /*
         ** Logical font has been created so just set it and return.
         */
         GpiSetCharSet(hps,picdef.lfont1);
         return TRUE;
      }
      strcpy (fattrs.szFacename,picdef.szfontname);

      picdef.lfont1 = getFontSetID(hps);
   }
   else
   {
      if (picdef.lfont2)
      {
         /*
         ** Logical font has been created so just set it and return.
         */
         GpiSetCharSet(hps,picdef.lfont2);
         return TRUE;
      }
      strcpy (fattrs.szFacename,picdef.szfontnam1);
      picdef.lfont2 = getFontSetID(hps);
   }
   dfxpointsize = MAKEFIXED(24,0);

   pSizefx->cy = dfxpointsize;
   pSizefx->cx = dfxpointsize;

   picVectorFontSize(hps,pSizefx, &fattrs);
   if (!n)
   {
      GpiCreateLogFont(hps,(PSTR8)fattrs.szFacename,picdef.lfont1,&(fattrs));
      GpiSetCharSet(hps,picdef.lfont1);
   }
   else
   {
      GpiCreateLogFont(hps,(PSTR8)fattrs.szFacename,picdef.lfont2,&(fattrs));
      GpiSetCharSet(hps,picdef.lfont2);
   }

   if (i > 0)
      return TRUE;
   else
      return FALSE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picText.      opcode 0xA8                                             */
/*                                                                        */
/*  Description : Draws some text at the current position.                */
/*                                                                        */
/*  Parameters  : HPS hps - Metafile hps.                                 */
/*                UCHAR   - opCode.                                       */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picText(int hfile,HPS hps, SIZEF *pSizefx)
{
   unsigned char n;
   LONG lColor;
   POINTL ptlCurrent;
   PPOINTL aptl;           /* Pointer een array van punten */
   POINTL  nptl[10];       /* QueryTextbox needs 5 points  */
   RECTL   rcl;
   unsigned char c[250];
   long    dx,dy;
   int i;

   aptl = nptl;            /* To give querytextbox space to work with  */

   /*
   ** Get string direction.
   ** 0x00 = horizontal
   ** 0x10 = vertical.
   ** 0x20 = upsidedown.
   ** 0x30 = vertical heading down.
   */
   lColor = GpiQueryColor(hps);
   GpiSetColor(hps,CLR_BLACK);
   i = read(hfile,&n,sizeof(unsigned char));
   if (i < 0) return FALSE;


   GpiSetCharBox(hps,pSizefx);

   
   i = 0;
   do
   {
     read(hfile,&c[i++],sizeof(unsigned char));
   } while (c[i-1]);

   GpiQueryCurrentPosition(hps,&ptlCurrent);
   GpiQueryTextBox(hps,(LONG)strlen(c),
                   c,TXTBOX_COUNT,aptl);

   rcl.yBottom  = nptl[TXTBOX_BOTTOMLEFT].y;
   rcl.yTop     = nptl[TXTBOX_TOPLEFT].y;
   rcl.xRight   = nptl[TXTBOX_BOTTOMRIGHT].x;
   rcl.xLeft    = nptl[TXTBOX_TOPLEFT].x;


   if (n >= 0x10 && n <= 0x18)
   {
      GpiSetCharAngle (hps, &agradl[1]);
   }
   else if ( n <= 0x08 )
   {
      switch(n)
      {
         case 0x00: 
            /*
            ** Current position is the center of the
            ** string box in this case.
            */
            dx = rcl.xRight - rcl.xLeft;
            dy = rcl.yTop - rcl.yBottom;
            dx /= 2;
            ptlCurrent.x -= dx;
            GpiSetCurrentPosition(hps,&ptlCurrent);
            GpiSetCharAngle (hps, &agradl[0]);
            break;
         case 0x02: 
            /*
            ** Top center must be used for text alignment.
            */
            dx = rcl.xRight - rcl.xLeft;
            dy = rcl.yTop - rcl.yBottom;
            dx /= 2;
            dy /= 2;
            ptlCurrent.x -= dx;
            ptlCurrent.y -= dy;
            GpiSetCurrentPosition(hps,&ptlCurrent);
            GpiSetCharAngle (hps, &agradl[0]);
            break;
         case 0x03: 
            /*
            ** Right center must be used for text alignment.
            */
            dx = rcl.xRight - rcl.xLeft;
            ptlCurrent.x -= dx;
            GpiSetCurrentPosition(hps,&ptlCurrent);
            GpiSetCharAngle (hps, &agradl[0]);
            break;
         default:
            GpiSetCharAngle (hps, &agradl[0]);
            break;
      }
   }
   else
      GpiSetCharAngle (hps, &agradl[0]);


   GpiCharString(hps,(LONG)strlen(c),c);
   GpiSetColor(hps,lColor);
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picHeader                                                             */
/*                                                                        */
/*  Description : Reads the header.                                       */
/*                                                                        */
/*                int hfile - file pointer.                               */
/*                                                                        */
/*  Returns     : BOOL TRUE on correct header.                            */
/*------------------------------------------------------------------------*/
static BOOL picHeader(int hfile)
{
   unsigned char header[HEADERSIZE];
   int i;

   i = read(hfile,header,HEADERSIZE);
   if ( i < 0)
      return FALSE;

   for (i = 0; i < HEADERSIZE; i++)
   {
      if (picheader[i]!=header[i])
         return FALSE;
   }   
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  picParsing                                                            */
/*                                                                        */
/*  Description : Parses the entire file and jumps to the approriate      */
/*                function to handle the opCode in detail.                */
/*                                                                        */
/*                int hfile - file pointer.                               */
/*                HPS hps   - handle to a presentation space.             */
/*                                                                        */
/*  Returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
static BOOL picParsing(HPS hps,int hfile)
{
   unsigned char opCode;
   SIZEF sizefx,sz;
   BOOL  bRet = TRUE;

   while ((read(hfile,&opCode,sizeof(unsigned char) )) > 0 && bRet)
   {
      switch(opCode)
      {
         case 0xA0:
            bRet = picMove(hfile,hps,opCode);
            break;
         case 0xA2:
            bRet = picDraw(hfile,hps,opCode);
            break;
         case 0x30:
            bRet = picFill(hfile,hps,opCode);
            break;
         case 0xD0:
            bRet = picFillOutl(hfile,hps,opCode);
            break;
         case 0xAC:
            bRet = picSize(hfile,hps,&sizefx);
            break;
         case 0xA7:
            bRet = picFont(hfile,hps,opCode,&sz);
            break;
         case 0xA8:
            bRet = picText(hfile,hps,&sizefx);
            break;
         default:
            if (opCode >= 0xB0 && opCode <= 0xBF)
               bRet = picColor(hps, opCode);
            else if (opCode >= 0x60 && opCode <= 0x6f)
            {
               close(hfile);
               return TRUE;
            }
      }
   }
   close(hfile);
   return bRet;
}
/*------------------------------------------------------------------------*/
/*  waitDlgProc.                                                          */
/*                                                                        */
/*  Description : Shows a small dialog window telling the user to wait    */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY waitDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   SWP swp;   /* Screen Window Position Holder */

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

         break;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*------------------------------------------------------------------------*/
/*  picDlgProc.                                                           */
/*                                                                        */
/*  Description : Services the dialog window which enables the user to    */
/*                define the font type to be used in the pic file.        */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT EXPENTRY picDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
   SWP swp;     /* Screen Window Position Holder */
   SHORT sItem;
   int i;

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

	 WinSetDlgItemText(hwnd,ID_LTFONTNAME,picdef.szfontname);
	 WinSetDlgItemText(hwnd,ID_LTFONTNAME1,picdef.szfontnam1);
         for (i=0; i < 5; i++)
            WinSendDlgItemMsg(hwnd,ID_DRPFILLING,LM_INSERTITEM, 
                          MPFROM2SHORT(LIT_END, 0), MPFROMP(picFillPattern[i]));

         WinSendDlgItemMsg(hwnd,ID_DRPFILLING,LM_SELECTITEM, 
                           MPFROMSHORT(0),MPFROMSHORT(TRUE));

         for (i=0; i < 3; i++)
            WinSendDlgItemMsg(hwnd,ID_DRPLINETYPE,LM_INSERTITEM, 
                          MPFROM2SHORT(LIT_END, 0), MPFROMP(picLineType[i]));

         WinSendDlgItemMsg(hwnd,ID_DRPLINETYPE,LM_SELECTITEM, 
                           MPFROMSHORT(0),MPFROMSHORT(TRUE));

         break;
      case WM_COMMAND:
	 switch(LOUSHORT(mp1))
	 {
            case DID_OK:
               sItem = SHORT1FROMMR(WinSendDlgItemMsg(hwnd,ID_DRPLINETYPE,
                                                      LM_QUERYSELECTION, 
                                                      MPFROMSHORT(LIT_FIRST), 
                                                      MPVOID));
               switch (sItem)
               {
                   case 1:  picdef.linetype = LINETYPE_SHORTDASH; break;
                   case 2:  picdef.linetype = LINETYPE_DOT;       break;
                   default: picdef.linetype = LINETYPE_SOLID;     break;
               }


               sItem = SHORT1FROMMR(WinSendDlgItemMsg(hwnd,ID_DRPFILLING,
                                                      LM_QUERYSELECTION, 
                                                      MPFROMSHORT(LIT_FIRST), 
                                                      MPVOID));


               switch (sItem)
               {
                   case 1:  picdef.filling = PATSYM_HALFTONE;  break;
                   case 2:  picdef.filling = PATSYM_HORIZ;     break;
                   case 3:  picdef.filling = PATSYM_DIAG1;     break;
                   case 4:  picdef.filling = PATSYM_BLANK;     break;
                   default: picdef.filling = PATSYM_SOLID;     break;
               }
               WinDismissDlg(hwnd,DID_OK);
               return 0;
            case DID_CANCEL:
               WinDismissDlg(hwnd,DID_CANCEL);
               return 0;
            case DID_HELP:
               ShowDlgHelp(hwnd);
               return 0;
            case ID_PBFONT:
               picGetFontName(hwnd,picdef.szfontname);
               WinSetDlgItemText(hwnd,ID_LTFONTNAME,picdef.szfontname);
               return 0;
            case ID_PBFONT1:
               picGetFontName(hwnd,picdef.szfontnam1);
               WinSetDlgItemText(hwnd,ID_LTFONTNAME1,picdef.szfontnam1);
               return 0;
         }
         return (MRESULT)0;
   }
   return(WinDefDlgProc(hwnd, msg, mp1, mp2));
}
/*-----------------------------------------------[ private ]--------------*/
/*  picCreateHmf.                                                         */
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
static void _System picCreateHmf(PICDEF *pcdef )
{
   HDC 		  hdc;		/* DC handle  			*/
   HPS            hps;          /* Presentation space handle   	*/
   DEVOPENSTRUC   dopData;     	/* DEVOPENSTRUC structure   	*/
   SIZEL          sizl;         /* max client area size         */
   static MATRIXLF matlf = {0x20000,0L,0L,0L,0x20000,0L,20L,20L,1L};

   sizl.cx = PICFORMWIDTH;
   sizl.cy = PICFORMHEIGHT;

   dopData.pszLogAddress	= NULL;
   dopData.pszDriverName	= (PSZ)"DISPLAY";
   dopData.pdriv              	= NULL;
   dopData.pszDataType        	= NULL;
   dopData.pszComment         	= NULL;
   dopData.pszQueueProcName   	= NULL;
   dopData.pszQueueProcParams 	= NULL;
   dopData.pszSpoolerParams   	= NULL;
   dopData.pszNetworkParams   	= NULL;

   hdc = DevOpenDC(hab,OD_METAFILE,"*",
                   9L,(PDEVOPENDATA)&dopData,
                   (HDC)NULL);

   hps = GpiCreatePS(hab,hdc,&sizl,
		     PU_LOMETRIC | GPIF_DEFAULT | GPIT_NORMAL  | GPIA_ASSOC);

   GpiSetDefaultViewMatrix(hps,9L,&matlf,TRANSFORM_REPLACE);

   picParsing(hps,pcdef->hfile);

   GpiAssociate(hps,(HDC)NULL);

   pcdef->pMeta->hmf = DevCloseDC(hdc);

   if (pcdef->pMeta->hmf)
   {
      SetMetaSizes(pcdef->pMeta, pcdef->pwi); /* see drwmeta.c        */
      pObjAppend((POBJECT)pcdef->pMeta);      /* Append to main chain */
      WinPostMsg(pcdef->pwi->hwndClient,UM_IMGLOADED,(MPARAM)pcdef->pMeta,(MPARAM)0);
   }
   GpiSetCharSet(hps, LCID_DEFAULT);
   GpiDeleteSetId(hps, picdef.lfont1);
   GpiDeleteSetId(hps, picdef.lfont2);

   GpiDestroyPS(hps);
   close(pcdef->hfile);
   pcdef->hfile = 0;
}
/*------------------------------------------------------------------------*/
static BOOL picRunLoader(WINDOWINFO *pwi, HAB hab,int hfile)
{
   APIRET rc;
   BOOL bRet = FALSE;
   char pszError[150];
   /*
   ** First check the header before going further.
   */
   if (!hfile || hfile == -1)
   {
      WinLoadString(hab,
                   (HMODULE)0,STRID_NOFILE,sizeof(pszError),pszError);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                    pszError,
                    "Error",0,
                    MB_OK | MB_MOVEABLE | MB_ICONEXCLAMATION);
      return FALSE;
   }

   if (!picHeader(hfile))
   {
      WinLoadString(hab,
                   (HMODULE)0,STRID_PICINVALID,sizeof(pszError),pszError);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                    pszError,
                    "Error",0,
                    MB_OK | MB_MOVEABLE | MB_ICONEXCLAMATION);
      close(hfile);
      return bRet;
   }

   if ( WinDlgBox(HWND_DESKTOP,pwi->hwndClient,
                  (PFNWP)picDlgProc,
                  (HMODULE)0,PIC_FILEDLG,(VOID *)0) == DID_OK)
   {


      picdef.hfile = hfile;
      picdef.pMeta = (pMetaimg)newMetaObject();

      ThreadFlags = 0;                /* Indicate that the thread is to */
                                      /* be started immediately         */
      StackSize = PICTHREADSTACK;     /* Set the size for the new       */
                                      /* thread's stack                 */

      rc = DosCreateThread(&ThreadID,(PFNTHREAD)picCreateHmf,(ULONG)&picdef,
                           ThreadFlags,StackSize);

      if (!rc)
         bRet = TRUE;
   }
   return bRet;
}
/*-----------------------------------------------[ public ]---------------*/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOL picLoad(WINDOWINFO *pwi)
{
   static         Loadinfo  li;
   BOOL           bRet = TRUE;
   int            hfile;

   picSetDefaults(pwi);

   li.dlgflags = FDS_OPEN_DIALOG;

   strcpy(li.szExtension,".PIC");

   if (!FileGetName(&li,pwi))
      return FALSE;

   hfile = open(li.szFileName,O_RDONLY | O_BINARY,S_IREAD);

   if (!picRunLoader(pwi,hab,hfile))
     bRet = FALSE;

   return bRet;
}
