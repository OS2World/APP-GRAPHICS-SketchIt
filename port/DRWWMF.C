/*---------------------------------------------------------------------------*/
/* File        : meta.cpp                                                    */
/*                                                                           */
/* Description : Converts a Windows WMF file into an OS/2 metafile.          */
/*---------------------------------------------------------------------------*/
#define INCL_GPI
#define INCL_WIN
#define INCL_DOSPROCESS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <io.h>
#include <memory.h>

#include "drwtypes.h"
#include "dlg.h"
#include "dlg_file.h"
#include "dlg_fnt.h"
#include "drwutl.h"
#include "drwmeta.h"
#include "drwwmf.h"
#include "dlg_hlp.h"
#include "resource.h"

#define METARECORDSIZE      0x08
#define SUBMETARECORDSIZE   0x06
#define PI                  3.141592654
#define TWO_PI             (2.0 * 3.141592654)
typedef long ( * metaproc )(HPS hps, int iFile, int iSize);

#define DRW_PIE   0x01
#define DRW_ARC   0x02
#define DRW_CHORD 0x03

#define PICTHREADSTACK 16384

unsigned char szBuf[8192];

VOID picVectorFontSize(HPS hps, SIZEF *psizef, PFATTRS pfattrs);

typedef struct _submetarecord
{
    unsigned int  dwSize;      /* Size in words(!) */
    unsigned short  wFunction;   /* Function number  */
} SUBMETARECORD;

typedef struct _WindowsMetaHeader
{
  unsigned short  FileType;       /* Type of metafile (0=memory, 1=disk) */
  unsigned short  HeaderSize;     /* Size of header in WORDS (always 9) */
  unsigned short  Version;        /* Version of Microsoft Windows used */
  unsigned int  FileSize;         /* Total size of the metafile in WORDs */
  unsigned short  NumOfObjects;   /* Number of objects in the file */
  unsigned int  MaxRecordSize;    /* The size of largest record in WORDs */
  unsigned short  NumOfParams;    /* Not Used (always 0) */
} WMFHEAD;


typedef struct _PlaceableMetaHeader
{
  unsigned int  key;    /* Magic number (always 9AC6CDD7h   */
  unsigned short handle;/* Metafile handle number (0)       */
  SHORT left;           /* Left coordinate in metafile units*/
  SHORT top;            /* Top coordinate in metafile units */
  SHORT right;          /* Right coordinate in metafile units*/
  SHORT bottom;         /* Bottom coordinate in metafile units*/
  unsigned short  Inch; /* Number of metafile units per inch*/
  unsigned int reserved;/* Reserved always (0)              */
  unsigned short checksum; /* Checksum value for prev 10 words */
} PLACEABLEMETAHEADER;

#define PLACEABLEHEADERSIZE 0x16


typedef enum
{
  OBJ_EMPTY,
  OBJ_BITMAP,
  OBJ_BRUSH,
  OBJ_PATTERNBRUSH,
  OBJ_FONT,
  OBJ_PEN,
  OBJ_REGION,
  OBJ_PALETTE
} ObjectType;

typedef struct _drawingobject
{
  ObjectType type;
  union
  {
     LOGBRUSH brush;
     LOGPEN pen;
//    BitmapObject bitmap;
//    PatternBrushObject pbrush;
//    FontObject font;
//    PaletteObject palette;
  } u;
} Object;

/*
** Helping hand in the second thread....
*/
typedef struct _wmfdef
{
   pMetaimg    pMeta;
   WINDOWINFO *pwi;
   int         hfile;
   SHORT       xPos;
   SHORT       yPos;
   LONG        lFontID;
   SIZEF       sizefx;
}WMFDEF;
static WMFDEF wmfdef; /* used to contain the picture defaults */
/*
** Thread variables for multithreaded loading of a pic file.
*/
TID         ThreadID;       /* New thread ID (returned)                   */
ULONG       ThreadFlags;    /* When to start thread,how to allocate stack */
ULONG       StackSize;      /* Size in bytes of new thread's stack        */
/*
** Drawing status needed for many things.....
*/
typedef struct _drawingstate
{
   HAB hab;            /* Anchor block handle                     */
   HPS hps;            /* Here we draw the WMF stuff in...        */
   HDC hdc;            /* DC handle                               */
   HMQ hmq;            /* Message queue handle                    */
   HMF hmf;
   long cxWorld;       /* The width of the complete drawing world */
   long cyWorld;       /* The height of the complete drawing world*/
   long yOrg;          /* The origin in the given world.  y-pos   */
   long xOrg;          /* The origin in the given world.  x-pos   */
   long xViewOrg;      /* The viewport origin in x-direction.     */
   long yViewOrg;      /* The viewport origin in y-direction.     */
   LOGBRUSH lBrush;    /* Current brush etc.                      */
   LOGPEN   lPen;
   LONG lFillMode;     /* BA_WINDING | BA_ALTERNATE.              */
   ULONG lTextColor;
   Object *pObjects;   /* Array of brushes/pens etc see header func*/
   short  sCurrent;
   ULONG lBackColor;   /* Background color */
} drawingstate;

/*---------------------------------------------------------------------------*/
static char  *pszError[] = {"Fatal error in metafile\nTo many GDI objects"};


static drawingstate gdistate; /* The gdi state! */
static int nObjects;          /* Read number of objects from wmf header */

static BOOL wmfFont(HPS hps, WMFDEF *pcdef );

/*
** newObject...
*/
Object * newObject(ObjectType type)
{
   int i;

   for (i = 0; i < nObjects; i++)
   {
      if (gdistate.pObjects[i].type == OBJ_EMPTY)
      {
         gdistate.pObjects[i].type = type;
         return &gdistate.pObjects[i];
      }
   }
   WinPostMsg(wmfdef.pwi->hwndClient,UM_WMFERROR,(MPARAM)pszError[0],(MPARAM)0);
   if (gdistate.pObjects)
      free(gdistate.pObjects);
   DosExit(EXIT_THREAD,0); /* Kill this thread after this fatal error */
   return NULL;
}
/*
** maptoViewport coords
*/
void mapToViewport(long *xPos, long *yPos)
{
    long xViewport;
    long yViewport;

    xViewport = (*xPos - gdistate.xOrg) - gdistate.xViewOrg;
    yViewport = (*yPos - gdistate.yOrg) - gdistate.yViewOrg;

    *xPos = xViewport;
    *yPos = yViewport;
}
/*-----------------------------------------------[ private ]-----------------*/
/* Name : converColor                                                        */
/*                                                                           */
/* Description : Converts the color from BGR to RGB (WIN->OS/2)              */
/*---------------------------------------------------------------------------*/
static void convertColor(unsigned long *plColor)
{
   USHORT Colors[4];

   Colors[0] = (USHORT)((*plColor & 0x00ff0000) >> 16); /* Blue  */
   Colors[1] = (USHORT)((*plColor & 0x0000ff00) >> 8);  /* Green */
   Colors[2] = (USHORT)(*plColor & 0x000000ff);         /* Red   */

   *plColor =  (LONG)Colors[0];         /* Blue lowest  part */
   *plColor |= (LONG)(Colors[1] << 8);  /* Green middel part */
   *plColor |= (LONG)(Colors[2] << 16); /* Red middel part   */
}
/*---------------------------------------------------------------------------*/
void CreateHmf(drawingstate *pcdef)
{
   DEVOPENSTRUC   dopData;     	/* DEVOPENSTRUC structure   	*/
   SIZEL          sizl;         /* max client area size         */
   static MATRIXLF matlf = {0x20000,0L,0L,0L,0x20000,0L,20L,20L,1L};

   if (pcdef->hps)
      return;        /* only once! */

   gdistate.lFillMode = BA_ALTERNATE; /* default fillingmode */

   sizl.cx = pcdef->cxWorld;
   sizl.cy = pcdef->cyWorld;

   dopData.pszLogAddress	= NULL;
   dopData.pszDriverName	= (PSZ)"DISPLAY";
   dopData.pdriv              	= NULL;
   dopData.pszDataType        	= NULL;
   dopData.pszComment         	= NULL;
   dopData.pszQueueProcName   	= NULL;
   dopData.pszQueueProcParams 	= NULL;
   dopData.pszSpoolerParams   	= NULL;
   dopData.pszNetworkParams   	= NULL;

   pcdef->hdc = DevOpenDC(pcdef->hab,OD_METAFILE,(PSZ)"*",
                          9L,(PDEVOPENDATA)&dopData,
                          (HDC)NULL);

   pcdef->hps = GpiCreatePS(pcdef->hab,pcdef->hdc,&sizl,
		            PU_LOMETRIC | GPIF_DEFAULT | 
		            GPIT_NORMAL | GPIA_ASSOC);

   GpiCreateLogColorTable ( pcdef->hps, 
                            LCOL_RESET, LCOLF_RGB, 0, 0, NULL );

   GpiSetDefaultViewMatrix(pcdef->hps,9L,&matlf,TRANSFORM_REPLACE);

   wmfFont(pcdef->hps,&wmfdef);

   return;
}
/*
** setPenWidth()
*/
static BOOL setPenWidth(HPS hps)
{
   BOOL bReturn = FALSE;

   if ( gdistate.lPen.lopnWidth.x > 1)
   {
      bReturn = TRUE;
      GpiSetLineWidthGeom(hps,gdistate.lPen.lopnWidth.x);
      GpiBeginPath( hps, 1L);  /* define a clip path    */
   }
   return bReturn;
}
/*
** setPattern
*/
static BOOL setPattern( HPS hps )
{
    if (gdistate.lBrush.lbStyle == BS_SOLID)
    {    
         GpiSetPattern(hps,PATSYM_SOLID);
         return TRUE;
    } 
    else if (gdistate.lBrush.lbStyle == BS_HOLLOW)
    {
       return FALSE;
    }
    else if (gdistate.lBrush.lbStyle == BS_NULL)
    {
       return FALSE;
    }
    
    if (gdistate.lBrush.lbStyle == BS_HATCHED )
    {
       /*
       ** Setup pattern if available.....
       */
       switch (gdistate.lBrush.lbHatch)
       {
          case HS_BDIAGONAL:
             GpiSetPattern(hps,PATSYM_DIAG1);
             break;
          case HS_FDIAGONAL:
             GpiSetPattern(hps,PATSYM_DIAG3);
             break;
          case HS_VERTICAL:
             GpiSetPattern(hps,PATSYM_VERT);
             break;
          case HS_HORIZONTAL:
             GpiSetPattern(hps,PATSYM_HORIZ);
             break;
       }
       return TRUE;
    }
    return FALSE;
}
/*-----------------------------------------------[ private ]-----------------*/
/* Name :beginArea                                                           */
/*---------------------------------------------------------------------------*/
static BOOL beginArea(HPS hps)
{
    LONG flOptions;

    if (gdistate.lBrush.lbStyle == BS_NULL ||
        gdistate.lBrush.lbStyle == BS_HOLLOW)
    {
       return FALSE;
    }

    if ( gdistate.lBrush.lbColor == gdistate.lPen.lopnColor)
       flOptions = BA_BOUNDARY;
    else
       flOptions = BA_NOBOUNDARY;

    GpiSetColor(hps,gdistate.lBrush.lbColor);

    if (setPattern(hps))
    {
       GpiBeginArea(hps,flOptions | gdistate.lFillMode);
       return TRUE;
    }
    return FALSE;
}
/*---------------------------------------------------------------------------*/
static long drawPolyPoints(HPS hps, short spoints, short *ppoints, BOOL bClose)
{
    short i,x;
    POINTL *pptl;
    LONG  lPoints;
    BOOL  bArea   = FALSE;
    BOOL  bSetPen = FALSE;
    POINTL ptl;
    
    lPoints = (spoints*2) + 1;

    pptl = (POINTL *)malloc( ( lPoints*sizeof(POINTL)) );

    for ( i = 0, x=0; i < spoints*2; x++ )
    {
        pptl[x].x = ppoints[i];
        pptl[x].y = ppoints[i+1];
        mapToViewport(&pptl[x].x, &pptl[x].y);
        pptl[x].y = gdistate.cyWorld - pptl[x].y;
        i += 2;
    }
    GpiQueryCurrentPosition(hps,&ptl); /* remember curr position */

    bArea = FALSE;

    if (bClose )
    {
         bArea = beginArea(hps);
         GpiMove(hps,&pptl[0]);
         lPoints = (LONG)((spoints));
         GpiPolyLine(hps,lPoints-1,(PPOINTL)&pptl[1]);
         GpiCloseFigure(hps);
         if (bArea)
           GpiEndArea(hps);
         
         if (gdistate.lPen.lopnStyle == PS_NULL)
            bArea = FALSE;

         if (bArea && gdistate.lBrush.lbColor != gdistate.lPen.lopnColor)
         {
            /*
            ** We are filled but the outline seems to be another color so....
            */
            GpiSetColor(hps,gdistate.lPen.lopnColor);
            bSetPen = setPenWidth(hps);
            GpiMove(hps,&pptl[0]);
            GpiPolyLine(hps,lPoints-1,(PPOINTL)&pptl[1]);
            GpiCloseFigure(hps);
         }
    }
    else /*bclose==FALSE*/
    {
       if (gdistate.lPen.lopnStyle != PS_NULL)
       {
          bSetPen = setPenWidth(hps);
          GpiMove(hps,&pptl[0]);
          lPoints = (LONG)((spoints));
          GpiPolyLine(hps,lPoints-1,(PPOINTL)&pptl[1]);
       }
    }

    if (bSetPen)
    {
       GpiEndPath(hps);
       GpiStrokePath (hps, 1, 0);
    }

    GpiMove(hps,&ptl); /* restore current position */
    free((void *)pptl);
    return 1L;
}
/*
** drawBox
*/
static void drawBox(HPS hps, POINTL ptlStart, POINTL ptlEnd,
                    long xRound,long yRound)
{
    BOOL bPen = FALSE;


    mapToViewport(&ptlStart.x, &ptlStart.y);      /* Map points in view */
    ptlStart.y = gdistate.cyWorld - ptlStart.y;   /* Make y OS/2 ....   */
    GpiMove(hps,&ptlStart);                       /* Move to starting pt*/

    mapToViewport(&ptlEnd.x, &ptlEnd.y);
    ptlEnd.y = gdistate.cyWorld - ptlEnd.y;

    if ( setPattern( hps ))
    {
       GpiSetColor(hps,gdistate.lBrush.lbColor);
       GpiBox(hps,DRO_FILL,&ptlEnd,xRound,yRound);
    }

    if (gdistate.lBrush.lbColor != gdistate.lPen.lopnColor)
    {
       bPen = setPenWidth(hps);
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       GpiBox(hps,DRO_OUTLINE,&ptlEnd,xRound,yRound);
       if (bPen)
       {
          GpiEndPath(hps);
          GpiStrokePath (hps, 1, 0);
       }
    }    
    return;
}
/*
** META_SETWINDOWORG
*/
static long meta_setwindoworg(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints[2];

    iRead = iSize;


    if ( iRead != (sizeof (unsigned short) * 2))
    {
        iRead = read(iFile,(void *)szBuf,iRead);
        return iRead;
    }

    /*
    ** Read params
    */
    iRead = read(iFile,(void *)spoints,iRead);

    gdistate.xOrg = (long)spoints[1];
    gdistate.yOrg = (long)spoints[0];
    return iRead;
}
/*---------------------------------------------------------------------------
** META_SETWINDOWEXT
---------------------------------------------------------------------------*/
static long meta_setwindowext(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints[2];

    iRead = iSize;
    
    if ( iRead != (sizeof (unsigned short) * 2))
    {
        iRead = read(iFile,(void *)szBuf,iRead);
        return iRead;
    }

    /*
    ** Read params
    */
    iRead = read(iFile,(void *)spoints,iRead);
    /*
    ** Remember the size of our world in the local structure.
    */
    gdistate.cxWorld = (long)spoints[1];
    gdistate.cyWorld = (long)spoints[0];

    CreateHmf(&gdistate);

    return iRead;
}
/*
** META_CREATEPALETTE
*/
static long meta_createpalette(HPS hps, int iFile, int iSize)
{
    int iRead;


    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);
    newObject(OBJ_PALETTE);

    return iRead;
}
/*
** META_SELECTPALETTE
*/
static long meta_selectepalette(HPS hps, int iFile, int iSize)
{
   int iRead;
   SHORT *ps;
   SHORT sTemp;
//   Object *pObj;

    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (SHORT *)szBuf;

    if (*ps < nObjects )
    {
       sTemp = *ps;
//       pObj = &gdistate.pObjects[sTemp];

       gdistate.sCurrent = sTemp;
    }
    return iRead;
}
/*
** META_REALIZEPALETTE
*/
static long meta_realizepalette(HPS hps, int iFile, int iSize)
{
    int iRead;

    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*
** META_SELECTOBJECT
*/
static long meta_selectobject(HPS hps, int iFile, int iSize)
{
    int    iRead;
    short  *ps;
    Object *pObj;
    short  sTemp;

    iRead = iSize;

    iRead = read(iFile,(void *)szBuf,iRead);
    ps = (SHORT *)szBuf;
    if (*ps < nObjects )
    {
       sTemp = *ps;
       pObj = &gdistate.pObjects[sTemp];

       gdistate.sCurrent = sTemp;

       switch (pObj->type)
       {
          case OBJ_PEN:
             gdistate.lPen.lopnColor   = pObj->u.pen.lopnColor;
             gdistate.lPen.lopnStyle   = pObj->u.pen.lopnStyle;
             gdistate.lPen.lopnWidth.x = pObj->u.pen.lopnWidth.x;
             gdistate.lPen.lopnWidth.y = pObj->u.pen.lopnWidth.y;
             break;
          case OBJ_BRUSH:
             gdistate.lBrush.lbColor  =  pObj->u.brush.lbColor;
             gdistate.lBrush.lbStyle  =  pObj->u.brush.lbStyle;
             gdistate.lBrush.lbHatch  =  pObj->u.brush.lbHatch;
             break;
       }
    }
    return iRead;
}
/*
** META_CREATEPENINDERECT
*/
static long meta_createpenindirect(HPS hps, int iFile, int iSize)
{
    int iRead;
    short *ps;
    long  *pl;
    Object *pObj;
    LOGPEN lPen;

    iRead = iSize;

    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (SHORT *)&szBuf; /* Get pen 16bit! */
    lPen.lopnStyle = (SHORT)*ps;
    ps = (SHORT *)&szBuf[2];
    lPen.lopnWidth.x = *ps;
    ps = (SHORT *)&szBuf[4];
    lPen.lopnWidth.y = *ps;
    pl = (LONG *)&szBuf[6];
    lPen.lopnColor   = *pl;

    lPen.lopnColor &= 0x00FFFFFF;

    convertColor(&lPen.lopnColor);
    GpiSetColor(hps,lPen.lopnColor);
    pObj = newObject(OBJ_PEN);
    
    pObj->u.pen.lopnColor =   lPen.lopnColor;
    pObj->u.pen.lopnStyle =   lPen.lopnStyle;
    pObj->u.pen.lopnWidth.x = lPen.lopnWidth.x;
    pObj->u.pen.lopnWidth.y = lPen.lopnWidth.y;

    gdistate.lPen.lopnColor   = pObj->u.pen.lopnColor;
    gdistate.lPen.lopnStyle   = pObj->u.pen.lopnStyle;
    gdistate.lPen.lopnWidth.x = pObj->u.pen.lopnWidth.x;
    gdistate.lPen.lopnWidth.y = pObj->u.pen.lopnWidth.y;
    return iRead;
}
/*
** META_SAVEDC
*/
static long meta_savedc(HPS hps, int iFile, int iSize)
{
    int iRead;

    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_restoredc(HPS hps, int iFile, int iSize)
{
    int iRead;

    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);

    GpiSetClipPath(hps, 0L, SCP_RESET);  /* clear the clip path   */
    return iRead;
}
/*
** META_intersectcliprect (IntersectClipRect(HDC, int, int, int, int))
** xLeft,yTop,xRight,yBottom
*/
static long meta_intersectcliprect(HPS hps, int iFile, int iSize)
{
    int  iRead;
    POINTL ptl;
    RECTL  rcl;
    short *ps;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    
    ps = (short *)szBuf;
    rcl.yBottom = (long)*ps;
    ps = (short *)&szBuf[2];
    rcl.xRight  = (long)*ps;
    ps = (short *)&szBuf[4];
    rcl.yTop    = (long)*ps;
    ps = (short *)&szBuf[6];
    rcl.xLeft   = (long)*ps;

    mapToViewport(&rcl.xLeft, &rcl.yTop);
    rcl.yTop = gdistate.cyWorld - rcl.yTop;

    mapToViewport(&rcl.xRight, &rcl.yBottom);
    rcl.yBottom = gdistate.cyWorld - rcl.yBottom;

    ptl.x = rcl.xLeft;
    ptl.y = rcl.yTop;
    GpiBeginPath(hps, 1L);  /* define a clip path    */
    GpiMove(hps,&ptl);
    ptl.x = rcl.xRight;
    ptl.y = rcl.yBottom;
    GpiBox(hps,DRO_OUTLINE,&ptl,0,0);
    GpiEndPath(hps);
    GpiSetClipPath(hps,1L,SCP_AND);
    return iRead;
}
/*
** META_MOVETO
*/
static long meta_moveto(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints[2];
    POINTL ptl;

    iRead = iSize;
    
    if ( iRead != (sizeof (unsigned short) * 2))
    {
        iRead = read(iFile,(void *)szBuf,iRead);
        return iRead;
    }

    /*
    ** Read params
    */
    iRead = read(iFile,(void *)spoints,iRead);

    ptl.x = (long)spoints[1];
    ptl.y = (long)spoints[0];

    mapToViewport(&ptl.x,&ptl.y);

    ptl.y = gdistate.cyWorld - ptl.y;
    GpiMove(hps,&ptl);

    return iRead;

}

/*
** META_LINETO
*/
static long meta_lineto(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints[2];
    POINTL ptl;
    BOOL bSetPen = FALSE;

    iRead = iSize;
    
    if ( iRead != (sizeof (unsigned short) * 2))
    {
        /*
        ** Skip this record
        */
        iRead = read(iFile,(void *)szBuf,iRead);
        return iRead;
    }

    /*
    ** Read params
    */
    iRead = read(iFile,(void *)spoints,iRead);
    ptl.x = (long)spoints[1];
    ptl.y = (long)spoints[0];
    mapToViewport(&ptl.x,&ptl.y);
    ptl.y = gdistate.cyWorld - ptl.y;
    if (gdistate.lPen.lopnStyle == PS_NULL)
    {
       GpiMove(hps,&ptl);
       return iRead;
    }
    GpiSetColor(hps,gdistate.lPen.lopnColor);
    bSetPen = setPenWidth(hps);


    GpiLine(hps,&ptl);

    if (bSetPen)
    {
       GpiEndPath(hps);
       GpiStrokePath (hps, 1, 0);
    }
    return iRead;

}
/*
** META_ESCAPE
** The Escape function allows applications to access 
** capabilities of a particular device not directly available through GDI. 
** Escape calls made by an application are translated and sent to the driver. 
*/
static long meta_escape(HPS hps,  int iFile, int iSize)
{
    int iRead;

    iRead = iSize;

    if (iRead <= 0)
       return -1L;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*
** META_DELETEOBJECT
*/
static long meta_deleteobject(HPS hps,  int iFile, int iSize)
{
    int iRead;
    short *ps;

    iRead = iSize;
    /*
    ** Skip this record
    */
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (short *)szBuf;
    if (*ps < nObjects )
    {
       
       gdistate.pObjects[*ps].type = OBJ_EMPTY;
       gdistate.sCurrent = -1;
    }
    return iRead;
}
/*---------------------------------------------------------------------------*/
/* Name : readHeader - reads the metafile header.                            */
/*---------------------------------------------------------------------------*/
int readHeader( int iFile)
{
    WMFHEAD wh;
    int     iRead;
    int     i;

    iRead = read(iFile,&wh,18);
    nObjects          = wh.NumOfObjects;
    gdistate.pObjects =(Object *)calloc((nObjects * sizeof(Object)),1);

    for (i = 0; i < nObjects; i++)
       gdistate.pObjects[i].type = OBJ_EMPTY;

    return iRead;
}
/*---------------------------------------------------------------------------*/
int readPlaceableHeader(int iFile)
{
    PLACEABLEMETAHEADER rh;
    unsigned char *p;
    int iRead,iR;

    rh.key = 0x9ac6cdd7;

    p = (unsigned char *)&rh;

    p += sizeof(unsigned int );

    iR = PLACEABLEHEADERSIZE;

    iR -=sizeof(unsigned int );
    iRead = read(iFile,(void *)p,iR);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static int readSubMetaRecord(int iFile, SUBMETARECORD *mr)
{
    int        iRead;

    iRead = read(iFile,mr,SUBMETARECORDSIZE);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_excludecliprect(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_ellipse(HPS hps, int iFile, int iSize)
{
    int iRead;
    POINTL ptl1,ptl2;
    POINTL ptc;
    ARCPARAMS arcp;
    short *ps;
    BOOL  bSetPen = FALSE;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (SHORT *)&szBuf[0];   /* yBottom - MS-WINDOWS COORDS*/
    ptl2.y = *ps;
    ps = (SHORT *)&szBuf[2];   /* xRight */
    ptl2.x = *ps;
    ps = (SHORT *)&szBuf[4];   /* yTop   */
    ptl1.y = *ps;
    ps = (SHORT *)&szBuf[6];   /* xLeft  */
    ptl1.x = *ps;

    mapToViewport(&ptl1.x, &ptl1.y);
    ptl1.y = gdistate.cyWorld - ptl1.y;
    mapToViewport(&ptl2.x, &ptl2.y);
    ptl2.y = gdistate.cyWorld - ptl2.y;
    /*
    ** calculate center point.
    */
    ptc.x = ptl1.x + ( ptl2.x - ptl1.x )/2;
    ptc.y = ptl2.y + ( ptl1.y - ptl2.y )/2;
    /*
    ** arc params.
    */
    arcp.lP = ( ptl2.x - ptl1.x )/2;
    arcp.lS = 0;

    arcp.lR = 0;
    arcp.lQ = ( ptl1.y - ptl2.y )/2;

    GpiSetCurrentPosition(hps,&ptc);
    GpiSetArcParams(hps,&arcp);


    if (!setPattern(hps))
    {
       /* 
       ** If there is no pattern set we just draw the outline and quit.
       ** But only if there is a visible pen.
       */
       if (gdistate.lPen.lopnStyle != PS_NULL)
       {
          bSetPen = setPenWidth(hps);
          GpiSetColor(hps,gdistate.lPen.lopnColor);
          GpiFullArc(hps,DRO_OUTLINE,MAKEFIXED(1,0));
          if (bSetPen)
          {
             GpiEndPath(hps);
             GpiStrokePath (hps, 1, 0);
          }
       }
       return iRead;
    }
    else
    {
       GpiSetColor(hps,gdistate.lBrush.lbColor);
       if (gdistate.lPen.lopnColor != gdistate.lBrush.lbColor)
          GpiFullArc(hps,DRO_FILL,MAKEFIXED(1,0));
       else
          GpiFullArc(hps,DRO_OUTLINEFILL,MAKEFIXED(1,0));
    }

    if ( gdistate.lPen.lopnColor != gdistate.lBrush.lbColor)
    {
       if ( gdistate.lPen.lopnStyle != PS_NULL)
       {

       /*
       ** We had a filled ellipse but the outline had another color
       ** so draw finally the ellipse outline.
       */
       bSetPen = setPenWidth(hps);
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       GpiFullArc(hps,DRO_OUTLINE,MAKEFIXED(1,0));
       if (bSetPen)
       {
          GpiEndPath(hps);
          GpiStrokePath (hps, 1, 0);
       }
       }
    }
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_floodfill(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static void read_arcparms(int iFile, int iSize,RECTL *rcl, 
                          POINTL *ptlStart, POINTL *ptlEnd)
{
    int    iRead;
    short *pPoint;  

    iRead  = iSize;
    iRead  = read(iFile,(void *)szBuf,iRead);
    pPoint = (short *)szBuf;

    ptlEnd->y    = (long)pPoint[0];
    ptlEnd->x    = (long)pPoint[1];
    ptlStart->y  = (long)pPoint[2];
    ptlStart->x  = (long)pPoint[3];
    rcl->yBottom = (long)pPoint[4];
    rcl->xRight  = (long)pPoint[5];
    rcl->yTop    = (long)pPoint[6];
    rcl->xLeft   = (long)pPoint[7];

    mapToViewport(&rcl->xRight, &rcl->yBottom);      /* Map points in view */
    rcl->yBottom = gdistate.cyWorld - rcl->yBottom;  /* Make y OS/2 ....   */

    mapToViewport(&rcl->xLeft, &rcl->yTop);          /* Map points in view */
    rcl->yTop = gdistate.cyWorld - rcl->yTop;        /* Make y OS/2 ....   */
    return;
}
/*--------------------------------------------------------------------------*/
static long calcAngle( long inAngle, long x, long y)
{
   long lAngle = inAngle;

   if (x < 0 && y >  0 )
      lAngle = 180 - lAngle;
   else if ( x < 0 && y < 0)
      lAngle = 180 - lAngle; /* Angle < 0 ! --=+*/
   else if ( x > 0 && y < 0 )
      lAngle = 360 + lAngle; /* Angle < 0 ! */
   else if ( y == 0 && x > 0)
      lAngle = 0; /* just overwrite */
   else if ( x == 0 && y > 0)
      lAngle = 90; /* just overwrite */
   else if ( y == 0 && x < 0)
      lAngle = 180; /* just overwrite */
   else if ( y < 0 && x == 0)
      lAngle = 270; /* just overwrite */

   return lAngle;
}
/*---------------------------------------------------------------------------*/
/* Name         : arc.                                                       */
/*---------------------------------------------------------------------------*/
static MRESULT arc(HPS hps,RECTL rcl,POINTL ptlStart,POINTL ptlEnd, int idrwMode)
{
   POINTL    ptlCenter,ptl;
   double    dStartAngle,dTemp,phi,dSweep;
   double    radius1,radius2;
   long      startAngle;
   long      sweepAngle;
   ULONG     ulLineType;
   ARCPARAMS arcp;
   BOOL      bSetPen = FALSE;

   ptlCenter.x = rcl.xLeft + ((rcl.xRight - rcl.xLeft)/2);
   ptlCenter.y = rcl.yBottom + ((rcl.yTop - rcl.yBottom)/2);

   arcp.lP = (rcl.xRight - rcl.xLeft);
   arcp.lS = 0;
   arcp.lR = 0;
   arcp.lQ = (rcl.yTop - rcl.yBottom);
   /*
   ** Length of both vectors.
   */
   radius1 = (double)sqrt((ptlStart.x * ptlStart.x) + (ptlStart.y * ptlStart.y));
   radius2 = (double)sqrt((ptlEnd.x * ptlEnd.x) + (ptlEnd.y * ptlEnd.y));

   phi = ((ptlStart.x * ptlEnd.x) + ( ptlStart.y * ptlEnd.y))/(radius1 * radius2);
   
   dSweep = acos(phi);

   dSweep   *= 360;
   dSweep   *= 65536;     /* Make fixed value */
   dSweep   /= (2 * PI);

   sweepAngle = (long)dSweep;

   dTemp      = (double)ptlStart.y;
   dStartAngle= asin(dTemp/radius1);
   startAngle = (long)((dStartAngle * 360) / (2 * PI));
   startAngle = calcAngle(startAngle,ptlStart.x,ptlStart.y);

   GpiSetCurrentPosition(hps,&ptlCenter);
   GpiSetArcParams(hps,&arcp);

   if (idrwMode == DRW_ARC || idrwMode == DRW_CHORD)
   {
      ulLineType = GpiQueryLineType(hps);
      GpiSetLineType(hps,LINETYPE_INVISIBLE);
      GpiPartialArc(hps,&ptlCenter,0x0000ff00,MAKEFIXED(startAngle,0),
                    MAKEFIXED(360,0));
      GpiSetLineType(hps,ulLineType);
   }
   
   if (idrwMode == DRW_CHORD)
      GpiQueryCurrentPosition(hps,&ptl);

   if (idrwMode == DRW_ARC)
      bSetPen = setPenWidth(hps);

   GpiPartialArc(hps,&ptlCenter,0x0000ff00,MAKEFIXED(startAngle,0),
                 sweepAngle);

   if (idrwMode == DRW_PIE)
      GpiLine(hps,&ptlCenter);

   if (idrwMode == DRW_CHORD)
      GpiLine(hps,&ptl);

   if (bSetPen)
   {
      GpiEndPath(hps);
      GpiStrokePath (hps, 1, 0);
   }
   
   return (MRESULT)0;
}
/*---------------------------------------------------------------------------*/
/* pie(hdc,xLeft,yTop,xRight,yBottom,xStart,yStart,xEnd,yEnd)                */
/*---------------------------------------------------------------------------*/
static long meta_pie(HPS hps, int iFile, int iSize)
{
    RECTL rcl;
    POINTL ptlStart,ptlEnd;
    BOOL  bPen;

    read_arcparms(iFile,iSize,&rcl,&ptlStart,&ptlEnd);

    if ( setPattern( hps ))
    {
       GpiBeginArea(hps,BA_BOUNDARY | BA_ALTERNATE);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_PIE);
       GpiEndArea(hps);
    }

    if (gdistate.lBrush.lbColor != gdistate.lPen.lopnColor)
    {
       bPen = setPenWidth(hps);
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_PIE);
       if (bPen)
       {
          GpiEndPath(hps);
          GpiStrokePath (hps, 1, 0);
       }
    }
    else
    {
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_PIE);
    }
    return 1;
}
/*---------------------------------------------------------------------------*/
static long meta_arc(HPS hps, int iFile, int iSize)
{
    RECTL  rcl;
    POINTL ptlStart,ptlEnd;

    read_arcparms(iFile,iSize,&rcl,&ptlStart,&ptlEnd);
    GpiSetColor(hps,gdistate.lPen.lopnColor);
    arc(hps,rcl,ptlStart,ptlEnd,DRW_ARC);
    return 1;
}
/*---------------------------------------------------------------------------*/
static long meta_chord(HPS hps, int iFile, int iSize)
{
    RECTL rcl;
    POINTL ptlStart,ptlEnd;
    BOOL  bPen;

    read_arcparms(iFile,iSize,&rcl,&ptlStart,&ptlEnd);

    if ( setPattern( hps ))
    {
       GpiBeginArea(hps,BA_BOUNDARY | BA_ALTERNATE);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_CHORD);
       GpiEndArea(hps);
    }

    if (gdistate.lBrush.lbColor != gdistate.lPen.lopnColor)
    {
       bPen = setPenWidth(hps);
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_CHORD);
       if (bPen)
       {
          GpiEndPath(hps);
          GpiStrokePath (hps, 1, 0);
       }
    }
    else
    {
       GpiSetColor(hps,gdistate.lPen.lopnColor);
       arc(hps,rcl,ptlStart,ptlEnd,DRW_CHORD);
    }
    return 1;
}
/*
** Rectangle(hdc,xLeft,yTop,xRight,yBottom)
*/
static long meta_rectangle(HPS hps, int iFile, int iSize)
{
    int iRead;
    short *pPoint;  
    POINTL ptlStart;
    POINTL ptlEnd;

    iRead = read(iFile,(void *)szBuf,iSize);
    pPoint = (short *)szBuf;

    ptlStart.y = (long)pPoint[0];
    ptlStart.x = (long)pPoint[1];
    ptlEnd.y   = (long)pPoint[2];
    ptlEnd.x   = (long)pPoint[3];
    drawBox(hps,ptlStart,ptlEnd,0,0);
    return iRead;
}
/*---------------------------------------------------------------------------*/
/* roundrect(hdc,xLeft,yTop,xRight,yBottom,xCornerEllipse,yCornerEllipse)    */
/*---------------------------------------------------------------------------*/
static long meta_roundrect(HPS hps, int iFile, int iSize)
{
    int   iRead;
    short *ps;
    POINTL ptlStart,ptlEnd;
    long   xCornerEllipse,yCornerEllipse;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (short *)szBuf;
    yCornerEllipse = (long)*ps;
    ps = (short *)&szBuf[2];
    xCornerEllipse = (long)*ps;
    /*
    ** Get lower right corner (yBottom,xRight)
    */
    ps = (short *)&szBuf[4];
    ptlEnd.y       = (long)*ps; /* yBottom */
    ps = (short *)&szBuf[6];
    ptlEnd.x       = (long)*ps; /* xRight  */
    /*
    ** Get upper left corner (yTop,xLeft)
    */
    ps = (short *)&szBuf[8];
    ptlStart.y     = (long)*ps; /* yTop    */
    ps = (short *)&szBuf[10];
    ptlStart.x     = (long)*ps; /* xLeft   */

    drawBox(hps,ptlStart,ptlEnd,xCornerEllipse,yCornerEllipse);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_patblt(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setpixel(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_offsetcliprgn(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
/* TextOut(hdc,xStart,yStart,lpString,nCount)                                */
/*---------------------------------------------------------------------------*/
static long meta_textout(HPS hps, int iFile, int iSize)
{
    short *ps;
    LONG   len;
    char  *pszString;
    POINTL ptl;
    int    iRead;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (SHORT *)szBuf;          /* Get string length */
    len = (LONG)*ps;
    pszString = (char *)&szBuf[2];   /* The string!       */
    ps =(SHORT *)&szBuf[2 +len];

    ptl.y = (LONG)*ps;
    ps =(SHORT *)&szBuf[2 +len + 2];
    ptl.x = (LONG)*ps;
    mapToViewport(&ptl.x,&ptl.y);
    ptl.y = gdistate.cyWorld - ptl.y;
    GpiSetColor(hps,gdistate.lTextColor);
    GpiCharStringAt(hps,&ptl,len,(PSZ)pszString);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_bitblt(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_stretchblt(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_polygon(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints = 0; /* number of points */
    short *ppoints =0;

    iRead = iSize;
    read(iFile,(void *)&spoints,sizeof(short));
    iRead -= sizeof(short);
    iRead = read(iFile,(void *)szBuf,iRead);
    ppoints = (short *)szBuf;
    drawPolyPoints(hps,spoints,ppoints,TRUE);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_fillregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_frameregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_invertregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_paintregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_selectclipregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_settextalign(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setmapperflags(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_exttextout(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setdibtodev(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_animatepalette(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setpalentries(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
/* PolyPolygon(HDC, CONST POINT *, CONST INT *, int);                        */
/*---------------------------------------------------------------------------*/
static long meta_polypolygon(HPS hps, int iFile, int iSize)
{
    int iRead;
    short *ps;
    int npolys;          /* Number of polygons               */
    int   *nrpoints;     /* Number of points in each polygon */
    short *ppoints;      /* Pointer to the array of points   */
    int size,i,iBase;
//    int z,x;
    int iMaxPoints;      /* largest polygon                  */
//    POLYGON *pgon;     /* OS/2 Polygon structure.          */

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (short *)szBuf;
    npolys = (int)*ps;
    /*
    ** Get for each polygon the number of points!
    */
    size = npolys * sizeof(int);         /* Get buffersize in bytes needed */
    nrpoints    = (int *)calloc(size,1);

//    pgon = (POLYGON *)calloc((sizeof(POLYGON) * npolys),1);

    iMaxPoints = 0;
    for (i=0,iBase=2; i < npolys; i++,iBase += 2)
    {
       ps = (short *)&szBuf[iBase];
       nrpoints[i]      = (int)*ps;

//       pgon[i].ulPoints = (ULONG)*ps;
//       pgon[i].aPointl=(POINTL *)calloc((sizeof(POINTL) * pgon[i].ulPoints),1);
//       printf("Polygon nr[%d] has [%d] points\n",i,nrpoints[i]);

       if (nrpoints[i] > iMaxPoints)
          iMaxPoints = nrpoints[i];
    }
   
    for (i = 0; i < npolys; i++)
    {
       ppoints = (short *)&szBuf[iBase];
#if 0
       for ( z = 0, x=0; z < nrpoints[i]*2; x++ )
       {
          pgon[i].aPointl[x].x = ppoints[z];
          pgon[i].aPointl[x].y = ppoints[z+1];
          mapToViewport(&pgon[i].aPointl[x].x, &pgon[i].aPointl[x].y);
          pgon[i].aPointl[x].y = gdistate.cyWorld - pgon[i].aPointl[x].y;
          z += 2;
       }
#endif
       drawPolyPoints(hps,nrpoints[i],ppoints,TRUE);
       size = (nrpoints[i] * sizeof(short)) * 2;
       iBase += size;
    }
    free ((void *)nrpoints);

//    GpiMove(hps,&pgon[0].aPointl[0]);
//    GpiPolygons(hps,npolys,pgon,POLYGON_NOBOUNDARY,POLYGON_INCL);

    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_resizepalette(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_dibbitblt(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_dibstretchblt(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_dibcreatepatternbrush(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    newObject(OBJ_PATTERNBRUSH);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_stretchdib(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_extfloodfill(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_createpatternbrush(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_createfontindirect(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    newObject(OBJ_FONT);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_createbrushindirect(HPS hps, int iFile, int iSize)
{
    int iRead;
    LONG  *pl;
    SHORT *ps;
    Object *pObj;
    LOGBRUSH lBrush;    /* Current brush etc.                      */

    iRead  = iSize;
    iRead  = read(iFile,(void *)szBuf,iRead);

    ps = (short *)&szBuf[0];
    lBrush.lbStyle = *ps;
    pl = (long  *)&szBuf[2];
    lBrush.lbColor = *pl;
    ps = (short *)&szBuf[6];
    lBrush.lbHatch = (long)*ps;
    
    lBrush.lbColor &= 0x00FFFFFF;

    convertColor(&lBrush.lbColor);
    pObj = newObject(OBJ_BRUSH);
    pObj->u.brush.lbColor =    lBrush.lbColor;
    pObj->u.brush.lbStyle =    lBrush.lbStyle;
    pObj->u.brush.lbHatch =    lBrush.lbHatch;

    gdistate.lBrush.lbColor  =  pObj->u.brush.lbColor;
    gdistate.lBrush.lbStyle  =  pObj->u.brush.lbStyle;
    gdistate.lBrush.lbHatch  =  pObj->u.brush.lbHatch;

    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_createregion(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_polyline(HPS hps, int iFile, int iSize)
{
    int iRead;
    short spoints = 0; /* number of points */
    short *ppoints;

    iRead = iSize;
    read(iFile,(void *)&spoints,sizeof(short));
    iRead -= sizeof(short);
    iRead = read(iFile,(void *)szBuf,iRead);
    ppoints = (short *)szBuf;
    GpiSetColor(hps,gdistate.lPen.lopnColor);
    drawPolyPoints(hps,spoints,ppoints,FALSE);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setbkmode(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setmapmode(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setrop2(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setrelabs(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setpolyfillmode(HPS hps, int iFile, int iSize)
{
    int iRead;
    short *ps;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    ps = (SHORT *)&szBuf[0];

    if ( *ps == WINDING)
       gdistate.lFillMode = BA_WINDING;
    else
      gdistate.lFillMode = BA_ALTERNATE;
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setstretchbltmode(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_settextcharextra(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_settextcolor(HPS hps, int iFile, int iSize)
{
    int iRead;
    LONG *pl;

    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);

    pl = (LONG *)szBuf;
    gdistate.lTextColor   = *pl;
    gdistate.lTextColor  &= 0x00FFFFFF;
    convertColor(&gdistate.lTextColor);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_settextjustification(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setviewportorg(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_setviewportext(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
/*---------------------------------------------------------------------------*/
static long meta_offsetwindoworg(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
static long meta_scalewindowext(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
static long meta_offsetviewportorg(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
static long meta_scaleviewportext(HPS hps, int iFile, int iSize)
{
    int iRead;
    iRead = iSize;
    iRead = read(iFile,(void *)szBuf,iRead);
    return iRead;
}
static long meta_setbkcolor(HPS hps, int iFile, int iSize)
{
    int iRead;
    long *pl;

    iRead = read(iFile,(void *)szBuf,iSize);
    pl = (LONG *)szBuf;
    gdistate.lBackColor   = *pl;
    gdistate.lBackColor &= 0x00FFFFFF;
    convertColor(&gdistate.lBackColor);
    return iRead;
}
/*---------------------------------------------------------------------------*/
struct _metafunction
{
   unsigned short usFunction;
   metaproc pfnMeta;
} metafunc[] =
{ 
{ META_POLYGON,meta_polygon},
{ META_POLYLINE,meta_polyline},
{ META_LINETO,meta_lineto},
{ META_MOVETO,meta_moveto},
{ META_RECTANGLE,meta_rectangle},
{ META_ROUNDRECT,meta_roundrect},
{ META_SELECTOBJECT,meta_selectobject},
{ META_DELETEOBJECT,meta_deleteobject},
{ META_ESCAPE,meta_escape},
{ META_SETWINDOWORG,meta_setwindoworg},
{ META_SETWINDOWEXT,meta_setwindowext},
{ META_CREATEPALETTE,meta_createpalette},
{ META_SELECTPALETTE,meta_selectepalette},
{ META_REALIZEPALETTE,meta_realizepalette},
{ META_CREATEPENINDIRECT,meta_createpenindirect},
{ META_SAVEDC,meta_savedc},
{ META_INTERSECTCLIPRECT,meta_intersectcliprect},
{ META_RESTOREDC,meta_restoredc},
{ META_EXCLUDECLIPRECT,meta_excludecliprect},
{ META_ARC,meta_arc},
{ META_ELLIPSE,meta_ellipse},
{ META_FLOODFILL,meta_floodfill},
{ META_PIE,meta_pie},
{ META_PATBLT,meta_patblt},
{ META_SETPIXEL,meta_setpixel},
{ META_OFFSETCLIPRGN,meta_offsetcliprgn},
{ META_TEXTOUT,meta_textout},
{ META_BITBLT,meta_bitblt},
{ META_STRETCHBLT,meta_stretchblt},
{ META_FILLREGION,meta_fillregion},
{ META_FRAMEREGION,meta_frameregion},
{ META_INVERTREGION,meta_invertregion},
{ META_PAINTREGION,meta_paintregion},
{ META_SELECTCLIPREGION,meta_selectclipregion},
{ META_SETTEXTALIGN,meta_settextalign},
{ META_CHORD,meta_chord},
{ META_SETMAPPERFLAGS,meta_setmapperflags},
{ META_EXTTEXTOUT,meta_exttextout},
{ META_SETDIBTODEV,meta_setdibtodev},
{ META_ANIMATEPALETTE,meta_animatepalette},
{ META_SETPALENTRIES,meta_setpalentries},
{ META_POLYPOLYGON,meta_polypolygon},
{ META_RESIZEPALETTE,meta_resizepalette},
{ META_DIBBITBLT,meta_dibbitblt},
{ META_DIBSTRETCHBLT,meta_dibstretchblt},
{ META_DIBCREATEPATTERNBRUSH, meta_dibcreatepatternbrush,},
{ META_STRETCHDIB, meta_stretchdib,},
{ META_EXTFLOODFILL, meta_extfloodfill,},
{ META_CREATEPATTERNBRUSH, meta_createpatternbrush,},
{ META_CREATEPENINDIRECT, meta_createpenindirect,},
{ META_CREATEFONTINDIRECT, meta_createfontindirect,},
{ META_CREATEBRUSHINDIRECT, meta_createbrushindirect,},
{ META_CREATEREGION, meta_createregion,},
{ META_SETBKMODE, meta_setbkmode,},
{ META_SETMAPMODE, meta_setmapmode,},
{ META_SETROP2, meta_setrop2,},
{ META_SETRELABS, meta_setrelabs,},
{ META_SETPOLYFILLMODE, meta_setpolyfillmode,},
{ META_SETSTRETCHBLTMODE, meta_setstretchbltmode,},
{ META_SETTEXTCHAREXTRA, meta_settextcharextra,},
{ META_SETTEXTCOLOR, meta_settextcolor,},
{ META_SETTEXTJUSTIFICATION, meta_settextjustification,},
{ META_SETVIEWPORTORG, meta_setviewportorg,},
{ META_SETVIEWPORTEXT, meta_setviewportext,},
{ META_OFFSETWINDOWORG, meta_offsetwindoworg,},
{ META_SCALEWINDOWEXT, meta_scalewindowext,},
{ META_OFFSETVIEWPORTORG, meta_offsetviewportorg,},
{ META_SCALEVIEWPORTEXT, meta_scaleviewportext,},
{ META_SETBKCOLOR,meta_setbkcolor},
{0x0000,NULL}};
/*---------------------------------------------------------------------------*/
void iterateRecords( int iFile)
{
    SUBMETARECORD mr;
    int iRead,i,iFound;
    int iSize;

    do
    {
        iRead = readSubMetaRecord(iFile,&mr);

        if (iRead < 0)
            return;

        i      = 0;
        iFound = 0;

        do
        {
            if (metafunc[i].usFunction == mr.wFunction)
            {
                iSize  = mr.dwSize * 2;      /* size in words! *2 is bytes     */
                iSize -= SUBMETARECORDSIZE;  /* Get rest bytes to read by func */
                metafunc[i].pfnMeta(gdistate.hps,iFile,iSize);
                iFound = 1;
            }
            else
               i++;

        } while (!iFound && metafunc[i].pfnMeta);
    } while (metafunc[i].pfnMeta);
}
/*------------------------------------------------------------------------*/
static BOOL wmfFont(HPS hps, WMFDEF *pcdef )
{
   FATTRS fattrs;
   FIXED  dfxpointsize;

   int iFactor = gdistate.cxWorld / 2000;

   fattrs.usRecordLength    = sizeof(FATTRS);
   fattrs.fsSelection       = 0;
   fattrs.lMatch            = 0;
   fattrs.idRegistry        = 0;
   fattrs.usCodePage        = GpiQueryCp(hps);
   fattrs.fsType            = 0;
   fattrs.fsFontUse         = FATTR_FONTUSE_OUTLINE;
   fattrs.lMaxBaselineExt   = 0;
   fattrs.lAveCharWidth     = 0;
   dfxpointsize             = MAKEFIXED(12*iFactor,0);
   pcdef->sizefx.cy         = dfxpointsize;
   pcdef->sizefx.cx         = dfxpointsize;

   picVectorFontSize(hps,&pcdef->sizefx, &fattrs);

   strcpy(fattrs.szFacename,"Courier");
   pcdef->lFontID = getFontSetID(hps);
   GpiCreateLogFont(hps,(PSTR8)fattrs.szFacename,pcdef->lFontID,&(fattrs));
   GpiSetCharSet(hps,pcdef->lFontID);
   GpiSetCharBox(hps,&pcdef->sizefx);
   return TRUE;
}
/*-----------------------------------------------[ private ]--------------*/
/*  wmfCreateHmf.                                                         */
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
static void _System wmfCreateHmf(WMFDEF *pcdef )
{
   iterateRecords( pcdef->hfile);

   GpiAssociate(gdistate.hps,(HDC)NULL);

   pcdef->pMeta->hmf = DevCloseDC(gdistate.hdc);

   if (pcdef->pMeta->hmf)
   {
      SetMetaSizes(pcdef->pMeta, pcdef->pwi); /* see drwmeta.c        */
      if (pcdef->xPos && pcdef->yPos)
         setMetaPosition(pcdef->pwi,(POBJECT)pcdef->pMeta,(LONG)pcdef->xPos,(LONG)pcdef->yPos);
      pObjAppend((POBJECT)pcdef->pMeta);      /* Append to main chain */
      WinPostMsg(pcdef->pwi->hwndClient,UM_IMGLOADED,(MPARAM)pcdef->pMeta,(MPARAM)0);
   }
   GpiDestroyPS(gdistate.hps);
   close(pcdef->hfile);

   if (gdistate.pObjects)
      free(gdistate.pObjects);
   gdistate.pObjects = NULL;
   pcdef->hfile = 0;

   GpiSetCharSet(gdistate.hps, LCID_DEFAULT);
   GpiDeleteSetId(gdistate.hps,pcdef->lFontID);
}
/*---------------------------------------------------------------------------*/
/* Name : loadMetaFile.                                                      */
/*---------------------------------------------------------------------------*/
static BOOL wmfRunLoader(WINDOWINFO *pwi,HAB hab,int hfile,MPARAM mp2)
{
   unsigned int  iType;        /* FileType   */
   int iRead;                  /* Bytes read */
   APIRET rc;
   char szError[150];

   memset((void *)&gdistate,0,sizeof(drawingstate));
   memset((void *)&wmfdef,0,sizeof(WMFDEF));

   if (!hfile || hfile == -1)
   {
      WinLoadString(hab,
                   (HMODULE)0,STRID_NOFILE,sizeof(szError),szError);
      WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                    szError,
                    "Error",0,
                    MB_OK | MB_MOVEABLE | MB_ICONEXCLAMATION);
      return FALSE;
   }
   
   iRead = read(hfile,&iType,sizeof(unsigned int ));

   if (iType == 0x9AC6CDD7)
   {
      iRead +=readPlaceableHeader(hfile);

      if (readHeader(hfile) > 0)
      {
         wmfdef.hfile = hfile;
         wmfdef.pwi   = pwi;
         wmfdef.pMeta = (pMetaimg)newMetaObject();

         if (mp2)
         {
            wmfdef.xPos = SHORT1FROMMP(mp2);
            wmfdef.yPos = SHORT2FROMMP(mp2);
         }
         ThreadFlags = 0;                /* Indicate that the thread is to */
                                         /* be started immediately         */
         StackSize = PICTHREADSTACK;     /* Set the size for the new       */
                                         /* thread's stack                 */
         rc = DosCreateThread(&ThreadID,(PFNTHREAD)wmfCreateHmf,(ULONG)&wmfdef,
                              ThreadFlags,StackSize);

         if (rc)
         {
            /*
            ** Free memory, something went wrong...
            */
            if (gdistate.pObjects)
               free(gdistate.pObjects);
            gdistate.pObjects = NULL;
         }
      }
   }
   else
   {
       WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                         (PSZ)"Unknown file format",
                         (PSZ)"Sorry",0,
                         MB_OK | MB_APPLMODAL | MB_MOVEABLE |
                         MB_ICONEXCLAMATION);
       close(hfile);
       return FALSE;
   }
   return TRUE;
}
/*-----------------------------------------------[ public ]---------------*/
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT wmfLoad(WINDOWINFO *pwi)
{
   static         Loadinfo  li;
   int            hfile;

   li.dlgflags = FDS_OPEN_DIALOG;

   strcpy(li.szExtension,".WMF");

   if (!FileGetName(&li,pwi))
      return (MRESULT)0;

   hfile = open(li.szFileName,O_RDONLY | O_BINARY,S_IREAD);
   wmfRunLoader(pwi,hab,hfile,(MPARAM)0); /* Gives all the error message etc */
   return (MRESULT)0;
}
/*-----------------------------------------------[ public ]---------------*/
/* wmfLoadFile - Called after a WMF file is dropped on the drawing surface*/
/*------------------------------------------------------------------------*/
MRESULT wmfLoadFile(WINDOWINFO *pwi, char *pszFileName, MPARAM mp2)
{
   int  hfile;
   hfile = open(pszFileName,O_RDONLY | O_BINARY,S_IREAD);
   wmfRunLoader(pwi,hab,hfile,mp2); /* Gives all the error message etc */
   return (MRESULT)0;
}
