/*------------------------------------------------------------------------*/
/*  Name: drwform.c                                                       */
/*                                                                        */
/*  Description : Handles the formsize dialog with the multicolumn        */
/*                listbox.                                                */
/*                                                                        */
/*  Functions  :                                                          */
/*     Size2Name : Converts the formsize to the formname.                 */
/*     initForm  : Called during startup to load the default form from ini*/
/*     showForm  : Called just before the program starts handling the q   */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define  INCL_BASE
#define  INCL_WIN
#define  INCL_DOS
#define  INCL_WINSTDSPIN
#define  INCL_GPI
#define  INCL_WINSHELLDATA
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "drwtypes.h"
#include "dlg.h"
#include "dlg_hlp.h"
#include "drwform.h"               // MCLB definitions
#include "drwcanv.h"
#include "resource.h"

/* General Dialog Helper Macros */

#define CONTROLHWND(ID)         WinWindowFromID(hwnd,ID)

HWND MCLBHwnd;                  // Handle of MCLB control
SHORT Item;                     // Item index

MRESULT EXPENTRY FormDlgProc (HWND,ULONG,MPARAM,MPARAM);

#define FRMNAMELEN 255
#define INIBUFFER  500

static char tmpBuf[INIBUFFER];


typedef struct _drwform
{
   char *szFormtitle;
   USHORT usFormWidth;           /* in 0.1 mm units!! */
   USHORT usFormHeight;          /* in 0.1 mm units!! */
   BOOL   bPortrait;
   BOOL   bPixel;
} drwform;

static drwform form[] = {{"A3\t11.69 * 16.54\t29,69 * 42,00",2969,4200,FALSE},
             {"A4\t8.27 * 11.79\t21,00 * 29,96",2100,2969,FALSE},
             {"A5\t5.83 *  8.27\t14,80 * 21,00",1480,2100,FALSE},
             {"B4\t9.84 * 13.94\t25,00 * 35,40",2500,3540,FALSE},
             {"B5\t7.17 * 10.12\t18,20 * 25,70",1820,2570,FALSE},
             {"C Sheet\t17 * 22\t43,10 * 55.88",4310,5588,FALSE},
             {"C3 envelope\t12.76 * 18.03\t32,40 * 45,80",3240,4580,FALSE},
             {"C4 envelope\t9.02 * 12.76\t22,90 * 32,40",2290,3240,FALSE},
             {"C5 envelope\t6.38 * 9.01\t16,20 * 22,89",1620,2289,FALSE},
             {"C6 envelope\t4.48 * 6.38\t11,40 * 16,20",1140,1620,FALSE},
             {"CD Cover\t4,72 * 4,72\t12 * 12",1200,1200,FALSE},
             {"Folio\t8.5 * 13\t21,60 * 33,02", 2160,3302,FALSE},
             {"Letter\t8.5 * 11\t21,59 * 27,94",2159,2794,FALSE},
             {"Legal\t8.5 * 14\t21,59 * 35,56" ,2159,3556,FALSE},
             {"Ledger\t17 * 11\t43,18 * 27,94" ,4318,2794,FALSE},
             {"Executive\t7.25 * 10.5\t18,42 * 26,67",1842,2667,FALSE},
             {"D Sheet\t22 * 34\t55.88 * 86.36",5588,8636,FALSE},
             {"E Sheet\t34 * 44\t86.36 * 111.76",8636,11176,FALSE},
             {"DL envelope\t4.33 * 8.66\t11,00 * 22,00",1100,2200,FALSE},
             {"COM-10 evelope\t4.12 * 9.50\t10,47 * 24,13",  1047,2413,FALSE},
             {"Monarch envelope\t3.87 * 7.50\t9,84 * 19,05",  984,1905,FALSE},
             {"Quarto\t8.46 * 10.83\t21,50 * 27,50",2150,2750,FALSE},
             {"Statement\t5.5 * 8.5\t13,97 * 21,59",1397,2159,FALSE},
             {"Tabloid\t11 * 17\t27,94 * 43,18",2794,4318,FALSE},
             {"",0,0}};

#if 0
typedef struct _frm
{
   char   szformname[FRMNAMELEN];
   USHORT usFormWidth;
   USHORT usFormHeight;
   BOOL   bDeleted;
   struct _frm *next;
   struct _frm *previous;
} frm,*pfrm;

static frm *pFormBase,*pFormEnd;


/*
** AddForm
*/
static frm * NewForm(void)
{
   frm *pTmp;

   if (!pFormBase)
   {
      pFormBase = (frm *)calloc(sizeof(frm),1);
      pFormEnd  = pFormBase;
      pFormEnd->next     = NULL;
      pFormEnd->previous = NULL;
   }
   else
   {
      pFormEnd->next = (frm *)calloc(sizeof(frm),1);
      pTmp = pFormEnd;
      pFormEnd = pFormEnd->next;
      pFormEnd->previous = pTmp;
   }
   return pFormEnd;
}
/*
** DelForm
*/
static void DelForm(frm *pForm)
{
   pForm->bDeleted = TRUE;
}
/*
** Load forms from the inifile.
*/
static void loadFormsFromIni( void )
{
   HINI hini;

}
#endif
/*-----------------------------------------------[ private ]-----------------*/
/* Name        : saveAsDefault.                                              */
/*                                                                           */
/* Description : Save the given form as the program default form.            */
/*               The data is used at startup of the program.                 */
/*                                                                           */
/* Returns     : BOOL - TRUE on SUCCESS.                                     */
/*---------------------------------------------------------------------------*/
static BOOL saveAsDefault(WINDOWINFO *pwi,drwform *pfrm)
{
  HINI   hini;             /* initialization-file handle           */
  BOOL   bLandScape;
  
  if (!createIni(pwi))
     return FALSE;

  hini = PrfOpenProfile((HAB)0,pwi->szIniFile);

  if (pwi->usFormWidth > pwi->usFormHeight)
     bLandScape = TRUE;
  else
     bLandScape = FALSE;

  if (!hini)
     return FALSE;
  sprintf(tmpBuf,"%s;%d,%d,%d",pfrm->szFormtitle,pfrm->usFormWidth,pfrm->usFormHeight,bLandScape);

  PrfWriteProfileString(hini,
                        "FORM",
                        "DEFAULT",
                        tmpBuf);
  PrfCloseProfile(hini);
  return TRUE;
}
/*-----------------------------------------------[ private ]-----------------*/
static BOOL loadDefaultForm(WINDOWINFO *pwi)
{
   HINI  hini;             /* initialization-file handle           */
   ULONG ulLen;
   char  *p,*c;
   BOOL  bLandScape = FALSE;
   USHORT usTmp;
   struct stat bf;

  if (_stat(pwi->szIniFile,&bf) != 0)
     return FALSE; /* file does not exist */

   hini = PrfOpenProfile((HAB)0,pwi->szIniFile);

   if (!hini)
     return FALSE;

   ulLen = PrfQueryProfileString(hini,
                              "FORM",
                              "DEFAULT",
                              "",
                              tmpBuf,
                              INIBUFFER);
   PrfCloseProfile(hini);

   if (!ulLen)
      return FALSE;

   p = strchr(tmpBuf,';');

   if (!p)
      return FALSE;

   p++; /* jump over ; */

   c = p;  /* c points to first size */

   p = strchr(p,',');
   /*
   ** Again test ....
   */
   if (!p)
      return FALSE;

   *p = 0; /* Close string */

   pwi->usFormWidth    = (USHORT)atoi(c); /* Get form width */

   p++; /* jump over zero.... */
   c = p;
   p = strchr(p,',');

   if (!p)
      return FALSE;

   *p = 0; /* Close string */

   pwi->usFormHeight   = (USHORT)atoi(c); /* Get form width */

   p++; /* Jump over , */


   if (*p != '0')
      bLandScape = TRUE;
   else
      bLandScape = FALSE;

   if (bLandScape)
   {
      /*
      ** Check...
      */
      if (pwi->usFormHeight > pwi->usFormWidth)
      {
         /*
         ** OOPS this is portrait...
         */
         usTmp = pwi->usFormHeight;
         pwi->usFormHeight = pwi->usFormWidth;
         pwi->usFormWidth  = usTmp;
      }
   }
   pwi->usHeight = pwi->usFormHeight;
   pwi->usWidth  = pwi->usFormWidth;
   return TRUE;
}
/*-----------------------------------------------[ public ]---------------*/
/*  Function : Size2Name.                                                 */
/*                                                                        */
/*  Description : Finds the formname as result of the given sizes.        */
/*                When no form is found the function returns              */
/*                "User defined".                                         */
/*                                                                        */
/*  called from :drwprn.c. The print preview dialog shows the formname.   */
/*                                                                        */
/*  Parameters  :WINDOWINFO *pwi - pointer to the WINDOWINFO struct.      */
/*               char *frmname   - pointer to a buffer which contains     */
/*               the formname on return.                                  */
/*               int  iLen       - sizeof the frmname buffer.             */
/*                                                                        */
/*  returns     : BOOL TRUE on success.                                   */
/*------------------------------------------------------------------------*/
BOOL Size2Name(WINDOWINFO *pwi, char *frmname, int ilen)
{
   BOOL bFound = FALSE;
   char cName[FRMNAMELEN];
   char *p;
   int i = 0;

   if (ilen <= 40 || !frmname)
      return FALSE;

   if (pwi->usHeight >= pwi->usWidth)
   {
      /*
      ** Portrait
      */
      do
      {
         if (pwi->usWidth  == form[i].usFormWidth &&
             pwi->usHeight == form[i].usFormHeight )
         {
            bFound = TRUE;
         }
         i++;
      }while (!bFound && form[i].usFormHeight);
   }
   else
   {
      /*
      ** Landscape orientation.
      */
      do
      {
         if (pwi->usHeight  == form[i].usFormWidth &&
             pwi->usWidth   == form[i].usFormHeight )
         {
            bFound = TRUE;
         }
         i++;
      }while (!bFound && form[i].usFormHeight);
   }

   if (bFound)
   {
      i--;
      strcpy(cName,form[i].szFormtitle);
   }
   else
   {
      i = 0;
      strcpy(cName,form[i].szFormtitle);

   }
   p = strchr(cName,'\t');
   if (!p)
      return FALSE;
   else
      *p=0;
   strcpy(frmname,cName);
   return TRUE;
}
/*-----------------------------------------------[private]----------------*/
/*  Function    : SelectCurrForm                                          */
/*                                                                        */
/*  Description : Selects the current form in the multicolumnlistbox.     */
/*                Runs through the default list with the current settings.*/
/*                                                                        */
/*  Parameters  : HWND hList - Window handle of the multicolumn listbox   */
/*                WINDOWINFO * - Pointer to the application context.      */
/*                BOOL       - Portrait or landscape flag.                */
/*                                                                        */
/*  Returns     : SHORT - selected item in the list. Zero based offset.   */
/*------------------------------------------------------------------------*/
static SHORT SelectCurrForm(HWND hList, WINDOWINFO *pwi, BOOL bPortrait)
{
   SHORT i=0;
   BOOL  bFound=FALSE;

   if (bPortrait)
   {
      do
      {
         if (pwi->usWidth  == form[i].usFormWidth &&
             pwi->usHeight == form[i].usFormHeight )
         {
            bFound = TRUE;
         }
         i++;
      }while (!bFound && form[i].usFormHeight);
   }
   else
   {
      /*
      ** Landscape orientation.
      */
      do
      {
         if (pwi->usHeight  == form[i].usFormWidth &&
             pwi->usWidth   == form[i].usFormHeight )
         {
            bFound = TRUE;
         }
         i++;
      }while (!bFound && form[i].usFormHeight);
   }

   if (bFound)
   {
      i--;
      WinSendMsg(hList,LM_SELECTITEM,MPFROMSHORT(i),MPFROMSHORT(TRUE));
   }
   else
   {
      i = 0;
      WinSendMsg(hList,LM_SELECTITEM,MPFROMSHORT(0),MPFROMSHORT(TRUE));
   }
   return i;
}


MRESULT EXPENTRY FormDlgProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   SWP    swp;
   SHORT  sItem;
   static WINDOWINFO *pwi;
   static SHORT sCheck;
   static bPortrait = TRUE;
   static bOldOrient;
   static bSaveSelection;

  switch (msg)
  {

    case WM_INITDLG:
    {
      /* Create the MCLB before the dialog window is displayed.  We don't */
      /* have to worry about sizing it now since we will get and process  */
      /* a WM_WINDOWPOSCHANGED message before the window becomes visible. */

      MCLBINFO InitData;
      LONG InitSizeList[3]= {1L, 1L, 1L};  // Make col 1 as big as 2 and 3
      int i;

       /* Centre dialog	on the screen			*/
       WinQueryWindowPos(hwnd, (PSWP)&swp);
       WinSetWindowPos(hwnd, HWND_TOP,
                       ((WinQuerySysValue(HWND_DESKTOP,SV_CXSCREEN) - swp.cx) / 2),
                       ((WinQuerySysValue(HWND_DESKTOP,SV_CYSCREEN) - swp.cy) / 2),
                       0, 0, SWP_MOVE);

       pwi = (WINDOWINFO *)mp2;

       /* Initialize MCLB create data structure */

       memset(&InitData, 0x00, sizeof(MCLBINFO));
       // These are the only required initialization values:
       InitData.Size = sizeof(MCLBINFO);
       InitData.Cols = 3;
       InitData.TabChar = '\t';
       InitData.Titles = "Form\tInch\tCM";
       InitData.InitSizes= InitSizeList;  // Initial sizes (proportions)

       // Play with these for colors and fonts:
       // InitData.TitleBColor = 0x00FFFF00;
       // InitData.TitleFColor = 0x00000000;
       // InitData.ListBColor  = 0x0000FFFF;
       // InitData.ListFColor  = 0x00000000;
       // strcpy(InitData.ListFont, "8.Helv");
       // strcpy(InitData.TitleFont, "10.Helvetica Bold Italic");

       /* Now create the MCLB.  The dialog window is the parent (so it */
       /* draws on the dialog), and owner (so this dialog proc will    */
       /* get messages from it).                                       */

       MCLBHwnd = MCLBCreateWindow(
                  hwnd,                    // Parent window
                  hwnd,                    // Owner to recv messages
                  WS_VISIBLE|              // Styles: Make it visible
                  WS_TABSTOP|              // Let user TAB to it
                  MCLBS_SIZEMETHOD_PROP|   // Proportional sizing when window is sized
                  LS_HORZSCROLL,           // Give each column a horizontal scroll bar
                  0,0,100,100,             // Will set size later, but must have large horz size now
                  HWND_TOP,                // Put on top of any sibling windows
                  ID_FRMDIALOG,            // Window ID
                  &InitData);              // MCLB create structure

       /* Insert data into the MCLB.  Each LM_INSERTITEM inserts a single */
       /* row.  Columns are separated with the TabChar supplied above     */
       /* (an "\t" for our sample here).                                  */

       i = 0;
       do
       {
          WinSendMsg(MCLBHwnd, LM_INSERTITEM, MPFROM2SHORT(LIT_END, 0), MPFROMP(form[i++].szFormtitle));
       } while (form[i].szFormtitle[0]);



       if (pwi->usFormHeight > pwi->usFormWidth)
       {
          /*
          ** Set radiobutton for portrait orientation.
          */
          bOldOrient = bPortrait = TRUE;
          WinSendDlgItemMsg(hwnd,ID_RADPORTRAIT,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
       }
       else
       {
          bOldOrient = bPortrait = FALSE;
          WinSendDlgItemMsg(hwnd,ID_RADLANDSCAPE,BM_SETCHECK,(MPARAM)1,(MPARAM)0);
       }
       /*
       ** Selects the current form in the multicolumn listbox.
       */
       sCheck = SelectCurrForm(MCLBHwnd,pwi,bPortrait);
       bSaveSelection = FALSE;
       return FALSE;
       } // end of WM_INITDLG

     case WM_COMMAND:
       switch (SHORT1FROMMP(mp1))
       {
         case DID_OK: /*----------~OK (PUSHBUTTON)----------*/
           /* Since our main window is a dialog, don't let default dialog */
           /* proc dismiss us or focus will not always return to the      */
           /* proper application.  Instead, destroy ourselves.  This is   */
           /* a trick to properly using a dialog for a main window.       */
           sItem = SHORT1FROMMR(WinSendMsg(MCLBHwnd, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), MPVOID));
           if ((sItem != sCheck ) || (bOldOrient != bPortrait))
           {
              if (bPortrait)
              {
                 PaperSetSize(pwi,
                              form[sItem].usFormWidth,
                              form[sItem].usFormHeight);
              }
              else
              {
                 /*
                 ** Make it a landscape orientation.
                 */
                 PaperSetSize(pwi,
                              form[sItem].usFormHeight,
                              form[sItem].usFormWidth);
              }
           }
           WinPostMsg(pwi->hwndClient,UM_ENDDIALOG,(MPARAM)0,(MPARAM)0);
           WinDestroyWindow(MCLBHwnd);
           WinDismissDlg(hwnd,DID_OK);

           if (bSaveSelection)
              saveAsDefault(pwi,&form[sItem]);
          return 0;
       case DID_CANCEL:
          WinDestroyWindow(MCLBHwnd);
          WinDismissDlg(hwnd,DID_CANCEL);
          return 0;
       case DID_HELP:
          ShowDlgHelp(hwnd);
          return 0;

      }
      break;

    case WM_CONTROL:
      switch SHORT1FROMMP(mp1)
      {
         case ID_RADPORTRAIT:
            bPortrait = TRUE;
            break;

         case ID_RADLANDSCAPE:
            bPortrait = FALSE;
            break;
         case ID_CHKASDEFAULT:
            bSaveSelection = !bSaveSelection;
            break;

        case ID_MCLB:
          /* Process control messages from the MCLB.  Most of them are */
          /* standard listbox messages with the same meaning as usual. */
          /* There are also a few MCLB-specific control messages.      */

          switch SHORT2FROMMP(mp1)
          {

            case LN_SELECT:

              /* User selected or unselected an entry in the MCLB.  Our   */
              /* sample uses a single-select listbox.                     */

              Item = SHORT1FROMMR(WinSendMsg(MCLBHwnd, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), MPVOID));
              return 0;

            case LN_ENTER:

              /* User double-clicked or pressed ENTER in the MCLB. */

              Item = SHORT1FROMMR(WinSendMsg(MCLBHwnd, LM_QUERYSELECTION, MPFROMSHORT(LIT_FIRST), MPVOID));
              return 0;
          } // switch on MCLB notify code
          break;

      }
      break; /* End of WM_CONTROL */

    case WM_WINDOWPOSCHANGED:
      {
      /* Dialog was resized, so we resize/move the dialog controls as needed. */
      /* When we resize the MCLB, it will adjust it's column widths according */
      /* the the MCLBS_SIZEMETHOD_* algorithm we specified in the style bits. */
      /* Resizing the control does NOT cause a MCLBN_COLSIZED control message.*/

      SWP  Pos1,Pos2,Pos3;

      /* Let def dlg proc set frame controls */
      WinDefDlgProc(hwnd, msg, mp1, mp2);

      /* Size/place MCLB above OK button, centered in dialog.  Note that the */
      /* dialog window is the frame, so we must account for frame controls.  */

      WinQueryWindowPos(CONTROLHWND(DID_OK), &Pos1);
      WinQueryWindowPos(hwnd, &Pos2);
      Pos2.cy = Pos2.cy - WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR) - WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER);
      Pos2.cx = Pos2.cx - (2 * Pos1.x);

      WinQueryWindowPos(CONTROLHWND(ID_GRPFORM), &Pos3);
      Pos2.cx -= Pos3.cx;


      WinSetWindowPos(MCLBHwnd, HWND_TOP,
         Pos1.x,
         Pos1.y+Pos1.cy+5,
         Pos2.cx,
         Pos2.cy - (Pos1.y+Pos1.cy+7+Pos1.y),
         SWP_MOVE|SWP_SIZE);

      return 0;  // Already called default proc, so just return.
      }

  } /* End of (msg) switch */

  return WinDefDlgProc(hwnd, msg, mp1, mp2);

} /* End dialog procedure */
/*-----------------------------------------------[ public  ]-----------------*/
/* Name        : FormDlg.                                                    */
/*---------------------------------------------------------------------------*/
void FormDlg(WINDOWINFO *pwi)
{
   WinDlgBox(HWND_DESKTOP,pwi->hwndClient,(PFNWP)FormDlgProc,NULLHANDLE,
             ID_FRMDIALOG,(void *)pwi);
   return;
}
/*-----------------------------------------------[ public  ]-----------------*/
/* Name        : initForm.                                                   */
/*                                                                           */
/* Description : Called during the startup of the application.  Loads the    */
/*               default form from the inifile.                              */
/*                                                                           */
/* Returns     : BOOL - TRUE on SUCCESS.                                     */
/*---------------------------------------------------------------------------*/
void initForm(WINDOWINFO *pwi)
{
   if (!loadDefaultForm(pwi))
   {
      pwi->usFormHeight = 2969;
      pwi->usFormWidth  = 2100;
      /*
      ** Keep the original values in mind.
      */
      pwi->usHeight     = 2969;
      pwi->usWidth      = 2100;
   }
}
/*-----------------------------------------------[ public  ]-----------------*/
/* Name        : showForm.                                                   */
/*                                                                           */
/* Description : Called from the main() function just before the program     */
/*               starts handling the message queue.                          */
/*                                                                           */
/* Parameters  : WINDOWINFO * - Pointer to the windowinfo struct (dlg.h)     */
/*               HWND hFrame  - Frame windowhandle of the drawing window.    */
/*               HWND hCanv   - Frame windowhandle of the canvas window which*/
/*                              holds the scrollbars.                        */
/*                                                                           */
/* Returns     : NONE.                                                       */
/*---------------------------------------------------------------------------*/
void showForm(WINDOWINFO *pwi, HWND hFrame, HWND hCanv)
{
   LONG   x,y;
   SWP    swp;
   ULONG  ulHeight,ulWidth;
   /*
   ** If the xPixels & yPixels (pixels/meter) do contain
   ** a value, try to calculate the number of pixels
   ** nessecary to show a kind of A4 form on the screen.
   ** So this overwrites our formsizes unless one of the
   ** values is zero.
   */
   ulHeight = pwi->usHeight * pwi->yPixels;
   ulWidth  = pwi->usWidth  * pwi->xPixels;

   ulHeight /= 10000;
   ulWidth  /= 10000;

   swp.cx = swp.cy = 0;
   x = 0;
   y = 0;
   WinQueryWindowPos(hCanv, (PSWP)&swp);
   if (swp.cx && swp.cy)
   {
     if (swp.cx > ulWidth) /* if possible center drawing window */
        x = (swp.cx - ulWidth)/2;

     if (swp.cy > ulHeight)
        y = (swp.cy - ulHeight)/2;
     else
        y = swp.cy - ulHeight;
   }
   WinSetWindowPos(hFrame,HWND_TOP,x,y,
                   ulWidth,ulHeight,
                   SWP_SHOW | SWP_SIZE | SWP_MOVE | SWP_ZORDER );
}
