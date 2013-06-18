/*---------------------------------------------------------------------------*/
/*  Name: drwcreat.c                                                         */
/*                                                                           */
/*  Description : Initializes the windowinfo structure.                      */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*--public functions---------------------------------------------------------*/
/*                                                                           */
/* DrawCreate          : Create the complete drawing.                        */
/* iniSaveSettings     : Save the settings in the drawit.ini.                */
/*                                                                           */
/*-sc---date-----------------------------------------------------------------*/
/* 1    060898   JdK   Added the inifilename stuff.                          */
/*---------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSNLS
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_fnt.h"
#include "dlg_lin.h"
#include "drwform.h" /* to load the default form size */
#include "resource.h"

#define  INISTRING 50

static WINDOWINFO wiCopy; /* Used to check for setting changes. .ini etc*/

extern HWND hwndMFrame;
/*-----------------------------------------------[ private ]-----------------*/
BOOL checkCodePage( void )
{
 ULONG  CpList[8];
 ULONG  CpSize;
 APIRET rc;                             /* Return code */
 
   rc = DosQueryCp(sizeof(CpList),       /* Length of list */
                   CpList,               /* List */
                   &CpSize);             /* Length of returned list */
 
  if (rc)
     return FALSE;

  if ( CpList[0] == 437 || CpList[0] == 850 )
     return FALSE;
  return TRUE;
}
/*-----------------------------------------------[ public ]------------------*/
/* Name        : creatIni.                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
BOOL createIni( WINDOWINFO *pwi )
{
  struct stat bf;
  char   szMsgBuf[255];
  USHORT usResponse;

  if (_stat(pwi->szIniFile,&bf) != 0)
  {
      sprintf(szMsgBuf,"DrawIt is about to create DRAWIT.INI\n"\
              "in the OS2 directory\n\n"\
              "Do you want to save the status?");

       usResponse = WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                                  (PSZ)szMsgBuf,(PSZ)"Warning", 0,
                                  MB_YESNO | MB_APPLMODAL |
                                  MB_MOVEABLE |
                                  MB_ICONQUESTION);

       if (usResponse == MBID_NO)
          return FALSE;
  }
  return TRUE;
}

/*------------------------------------------------------------------------*/
static void iniGetPaper(WINDOWINFO *pwi, HINI hini)
{
   char szPaper[INISTRING];

   if (!hini)
      pwi->paper = IDM_MM;
   else
   {
      PrfQueryProfileString(hini,"APPLIC","METRIC","YES",
                            szPaper,INISTRING);
   
      if (szPaper[0] == 'N')
         pwi->paper = IDM_INCH;
      else
         pwi->paper = IDM_MM;
   }
   WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
              MM_SETITEMATTR,
              MPFROM2SHORT(pwi->paper, TRUE),
              MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
}
/*------------------------------------------------------------------------*/
static void iniGetSelection(WINDOWINFO *pwi, HINI hini)
{
   char szSelection[INISTRING];

   if (!hini)
   {
      pwi->bOnArea = TRUE;
      WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                 MM_SETITEMATTR,
                 MPFROM2SHORT(IDM_ONAREA, TRUE),
                 MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
      return;
   }

   PrfQueryProfileString(hini,"APPLIC","SELECT","YES",
                         szSelection,INISTRING);
   
   if (szSelection[0] == 'N')
   {
      pwi->bOnArea = FALSE;
      WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                 MM_SETITEMATTR,
                 MPFROM2SHORT(IDM_ONLINE, TRUE),
                 MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));

   }
   else
   {
      pwi->bOnArea = TRUE;
      WinSendMsg(WinWindowFromID(hwndMFrame, FID_MENU),
                 MM_SETITEMATTR,
                 MPFROM2SHORT(IDM_ONAREA, TRUE),
                 MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
   }
}
/*------------------------------------------------------------------------*/
static void iniSaveSelection(HINI hini,BOOL bOnArea)
{
  if (bOnArea)
     PrfWriteProfileString(hini,"APPLIC","SELECT","YES");
  else
     PrfWriteProfileString(hini,"APPLIC","SELECT","NO");
}
/*------------------------------------------------------------------------*/
static void iniSaveInch(HINI hini, USHORT usPaper)
{
  if (usPaper == IDM_INCH)
     PrfWriteProfileString(hini,"APPLIC","METRIC","NO");
  else
     PrfWriteProfileString(hini,"APPLIC","METRIC","YES");
}
/*------------------------------------------------------------------------*/
void iniSaveSettings(WINDOWINFO *pwi)
{
   BOOL bSel  = FALSE;
   BOOL bInch = FALSE;
   HINI hini;
   /*
   ** First check whether we need to save some states.
   */
   bSel  = (BOOL)(pwi->bOnArea != wiCopy.bOnArea);

   bInch = (BOOL)(pwi->paper   != wiCopy.paper  );

   if (!bInch && !bSel)
      return;

   if (!createIni( pwi )) /* see drwform */
      return;

   if ( !(hini = PrfOpenProfile((HAB)0,pwi->szIniFile)) )
      return;
   
   if (bSel)
      iniSaveSelection(hini,pwi->bOnArea);
   
   if (bInch)
      iniSaveInch(hini,pwi->paper);

   PrfCloseProfile(hini);
   return;
}
/*------------------------------------------------------------------------*/
static void loadSettings(WINDOWINFO *pwi)
{
  struct stat bf;
  HINI hini;

  if (_stat(pwi->szIniFile,&bf) != 0)
     hini = (HINI)0;
  else
     hini = PrfOpenProfile((HAB)0,pwi->szIniFile);
 
  iniGetSelection(pwi,hini);
  iniGetPaper(pwi,hini);

  if (hini)
     PrfCloseProfile(hini);
  return;
}
/*------------------------------------------------------------------------*/
MRESULT DrawCreate(HWND hwnd, WINDOWINFO *pwi)
{
   SIZEL     sizl;                      /* max client area size       */
   ULONG     cbBuf  = CCHMAXPATH - 3;   /* Used for drive:\dir info   */
   PULONG    pcbBuf = &cbBuf;                /* Used for drive:\dir info   */
   ULONG     ulDiskNum;
   ULONG     ulLogicalDisk;
   HWND      hMenu;
   ULONG     StartIndex;
   ULONG     LastIndex;
   UCHAR     DataBuf[50];
   APIRET    rc;          /*  Return Code. */
   float     f1,f2;

   pwi->bDBCS = checkCodePage(); /* Running on double byte system?? */

   /*
   ** Setup the inifile name to be use.
   */
   StartIndex = 5;
   LastIndex  = 5;
   memset(DataBuf,0,50);
   rc = DosQuerySysInfo (StartIndex,LastIndex,DataBuf,50);
   if (rc == 0 )
   {
      pwi->szIniFile[0] = (CHAR)('A' - 1 + DataBuf[0]);
      pwi->szIniFile[1] = ':';
      pwi->szIniFile[2] = '\\';
      pwi->szIniFile[3] = 0;
      strcat(pwi->szIniFile,"OS2\\DRAWIT.INI");
   }
   else
      strcpy(pwi->szIniFile,"DRAWIT.INI");

   /* A4 = 2970 * 0.1 mm by 2100 * 0.1 mm ?? See drwform.c */

   initForm(pwi);
   pwi->fOffx        = 0.0;
   pwi->fOffy        = 0.0;

   pwi->uXfactor     = 1.0;
   pwi->uYfactor     = 1.0;
   pwi->ulUnits      = PU_LOMETRIC;
   pwi->uslayer     = MINLAYER;
   pwi->fxPointsize = MAKEFIXED(12,0);
   /*
   ** Store all important windowhandles!
   */
   pwi->hwndClient = hwnd;
   pwi->hwndVscroll = WinWindowFromID(WinQueryWindow(pwi->hwndClient, QW_PARENT),
                                              FID_VERTSCROLL);

   pwi->hwndHscroll = WinWindowFromID(WinQueryWindow(pwi->hwndClient, QW_PARENT),
                                                   FID_HORZSCROLL);

   /*
   ** Change the menufont to make it a bit smaller.
   */
   hMenu = WinWindowFromID(WinQueryWindow(pwi->hwndMain,QW_PARENT),
                                          FID_MENU);

   WinSetPresParam(hMenu,PP_FONTNAMESIZE,(ULONG)strlen("8.Helv")+1,(void *)"8.Helv");
   /*
   ** Change the font of the system menu.
   */
   hMenu = WinWindowFromID(WinQueryWindow(pwi->hwndMain,QW_PARENT),
                                          FID_SYSMENU);

   WinSetPresParam(hMenu,PP_FONTNAMESIZE,(ULONG)strlen("8.Helv")+1,(void *)"8.Helv");

   /* create a normal screen ps for the screen dc in twips          */
   /* 1440 twips == 1 inch; 20 twips == 1 point; 72 points per inch */

   sizl.cx = pwi->usFormWidth;
   sizl.cy = pwi->usFormHeight;

   /* Query hab */
   pwi->hab = hab; //WinQueryAnchorBlock( hwnd );

   pwi->hdcClient = WinOpenWindowDC(hwnd);

   pwi->hps = GpiCreatePS(hab,pwi->hdcClient,
                          &sizl,
                          PU_LOMETRIC  |      /* 0.1 mm precision */
                          GPIF_DEFAULT |
                          GPIT_NORMAL  |
                          GPIA_ASSOC);

   pwi->hpsScreen = pwi->hps;

   pwi->bPrinter  = FALSE;
   /*
   ** get the number of pixels per meter
   */
   DevQueryCaps(pwi->hdcClient,CAPS_VERTICAL_RESOLUTION,
                1L,&pwi->yPixels);
   DevQueryCaps(pwi->hdcClient,CAPS_HORIZONTAL_RESOLUTION,
                1L,&pwi->xPixels);

   /*
   ** If the xPixels & yPixels (pixels/meter) do contain
   ** a value, try to calculate the number of pixels
   ** nessecary to show a kind of A4 form on the screen.
   ** So this overwrites our formsizes unless one of the
   ** values is zero.
   */
   if (!pwi->yPixels || !pwi->xPixels)
   {
      /*
      ** Avoid a devide by zero!!!
      */
      pwi->yPixels = 1;
      pwi->xPixels = 1;
   }
//   FontInit(pwi->hps,(char *)0,FONT_INIT,&pwi->sizfx); /*see dlg_fnt.c*/
//   GpiSetCharBox(pwi->hps,&pwi->sizfx);

   GpiQueryModelTransformMatrix(pwi->hps,9L,&pwi->matOrg);


   GpiCreateLogColorTable ( pwi->hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
   GpiSetColor(pwi->hps,0x000000ff);

   /*
   ** Setup the aperture size used when picking objects with
   ** the mouse. For the moment only simple lines are selected
   ** with this method.
   ** Make it 1 mm big.
   */
   sizl.cx = 10L;  sizl.cy = 10L;

   GpiSetPickApertureSize(pwi->hps,PICKAP_REC, &sizl);


   /*Clientwindow background color is white by default*/

   pwi->lBackClr = 0x00ffffff;
   pwi->ColorPattern = PATSYM_DEFAULT;   /*by default create open figures */
   pwi->lLntype    = LINETYPE_SOLID;
   pwi->lLnWidth   = 1;                  /* One unit line width.          */
   pwi->lLnJoin    = LINEJOIN_DEFAULT;
   pwi->lLnEnd     = LINEEND_FLAT;
   pwi->ulColor    = 0x00000000;      /* black is default filling color*/
   pwi->ulOutLineColor = 0x00000000;  /* black is default outline color*/
   pwi->uslayer    = MINLAYER;
   pwi->paper      = IDM_MM;       /* Metric measurements by default?    */
   pwi->bSuppress  = FALSE;        /* No gradientfill suppress by def    */
   pwi->ulgridcx   = 50;           /* 50 * 0.1 mm gridsize.  x-direction */
   pwi->ulgridcy   = 50;           /* 50 * 0.1 mm gridsize.  y-direction */
   pwi->ulgriddisp = 1;            /* Grid display interval 1            */


   /* Setup the gradient fill stuff..*/

   pwi->Gradient.ulStart = 0;         /* starting point 0 degrees   */
   pwi->Gradient.ulSweep = 360;       /* sweeping angle 360 degrees */
   pwi->Gradient.ulSaturation = 100;  /* 100 % Color saturation.    */
   /*
   ** Fountain fill stuff....
   */
   pwi->fountain.ulStartColor = 0x00FF0000;
   pwi->fountain.ulEndColor   = 0x000000FF;
   pwi->fountain.lHorzOffset  = 0;
   pwi->fountain.lVertOffset  = 0;
   pwi->fountain.ulFountType  = 0;
   pwi->fountain.lAngle       = 0;

   WinPostMsg(pwi->hwndMain,UM_LAYERHASCHANGED,(MPARAM)0,(MPARAM)0);
   /*
   ** Initialize string for default directory
   ** Used in drag and drop situations.
   */
   DosQueryCurrentDisk(&ulDiskNum, &ulLogicalDisk);
   pwi->szCurrentDir[0] = (CHAR)('A' - 1 + ulDiskNum);
   pwi->szCurrentDir[1] = ':';
   pwi->szCurrentDir[2] = '\\';
   DosQueryCurrentDir(0L, &pwi->szCurrentDir[3], pcbBuf);
   if(pwi->szCurrentDir[3] == '\\')
      pwi->szCurrentDir[3] = '\0';    /*  account for the root, if current */

   /*
   ** Load popupmenu's
   */
   pwi->hwndImgMenu = WinLoadMenu(pwi->hwndMain,0,IDM_IMAGE);
   pwi->hwndOptMenu = WinLoadMenu(hwnd,0,IDM_LINE);
   pwi->hwndTxtMenu = WinLoadMenu(hwnd,0,IDM_FONT);
   pwi->hwndClipMenu= WinLoadMenu(hwnd,0,IDM_CLIPPATH);
   pwi->hwndBlockMenu = WinLoadMenu(hwnd,0,IDM_BLOCKTEXT);
   pwi->hwndMetaMenu  = WinLoadMenu(hwnd,0,IDM_METAPOPUP);
   /*
   ** Set fontname and layer, because our status line wants them.
   */
   memset(&pwi->fattrs,0,sizeof(FATTRS));
   pwi->fattrs.usRecordLength = sizeof(FATTRS);        /* Length of record  */
   pwi->fattrs.usCodePage     = GpiQueryCp(pwi->hps);  /* Code page         */
   pwi->fattrs.fsFontUse      = FATTR_FONTUSE_OUTLINE; /* Outline fonts only*/
   strcpy(pwi->fattrs.szFacename,"Courier");
   pwi->sizfx.cx              = MAKEFIXED(12,0);
   pwi->sizfx.cy              = MAKEFIXED(12,0);
   pwi->lcid                  = 3L;

   pwi->colorPalette.nColors  = 0;
   pwi->colorPalette.prgb2    = NULL;
   /*
   ** The program supports selection on the figure line as well
   ** as selection in the figure area.
   ** By default the user can  click on the object area to
   ** get it in the selected state.
   */
   pwi->bOnArea               = TRUE;

   /*
   ** Arrow definitions.
   */
   pwi->arrow.lSize  = DEF_ARROWSIZE;
   pwi->arrow.lEnd   = DEF_LINEEND;
   pwi->arrow.lStart = DEF_LINESTART;

   loadSettings(pwi);         /* Load settings from inifile. */
   /*
   ** Set default shading.
   */
   pwi->Shade.lShadeType  = SHADE_NONE;
   pwi->Shade.lShadeColor = 0x00c0c0c0; /* Gray default shade color?*/
   pwi->Shade.lUnits      = SHADOWMED;
   strcpy (pwi->fontname,"12.Courier"); /* also in dlf_fnt.c*/
   setFont(pwi,&pwi->fattrs,pwi->sizfx);
   /*
   ** Set font in status line.
   */
   WinPostMsg(pwi->hwndMain, UM_FNTHASCHANGED,
             (MPARAM)&pwi->fontname,(MPARAM)0);

   memcpy(&wiCopy,pwi,sizeof(WINDOWINFO));

   return (MRESULT)0;
}
