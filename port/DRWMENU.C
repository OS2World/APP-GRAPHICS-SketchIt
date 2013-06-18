/*-----------------------------------------------------------------------*/
/*  Name: DRWMENU.C                                                      */
/*                                                                       */
/*  Description : Menu processing.                                       */
/*                                                                       */
/* Functions:                                                            */
/*   MeasureItem    : Called whenever a WM_MEASURE item comes in.        */
/*   DrawMenuItem   : Draws the ownerdraw menuitems.                     */
/*   MenuFillPattern: Handles the selections in the filling pattern menu.*/
/*   MenuLineType   : Linetype submenu.                                  */
/*   Menuhelptext   : Shows hint text in application titlebar.           */
/*   ShowPopupMenu  : Shows popupmenu                                    */
/*   InitMenu       : Processes the WM_INITMENU from the main window.    */
/*   EnableMenuItem : Enables/ disables menu item.                       */
/*   MenuZoom       : Handles the zoom submenu.                          */
/*   measureColorItem: Color popup menu item size calculation            */
/*                                                                       */
/*-Private Functions-----------------------------------------------------*/
/*   menuSetFillPatternChecks  - Set the check marks in fill pattern menu*/
/*   menuSetLineTypeChecks     - Set the check marks in linetype menu    */
/*   menuSetLineJoinChecks     - Set the check marks in linejoin menu    */
/*   menuSetLineWidthChecks    - Set the check marks in linewidth menu   */
/*   menuSetBlockTxtAlignChecks- Set the check marks in alignment menu   */
/*-----------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS

#include <os2.h>
#include <string.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwmenu.h"
#include "resource.h"
#include "dlg_clr.h"
#include "drwfount.h"  /* fountain fill prototypes */
#include "dlg_img.h"     /* clipboard checking ... */
#include "dlg_txt.h"     /* oultine checking       */
#include "dlg_file.h"    /* Updatetitletext        */
#include "drwcanv.h"     /* for zooming            */
#include "drwutl.h"
#include "dlg_sel.h"     /* isSingleSelection(?)   */
#include "drwbtext.h"
#include "drwmeta.h"

#define DB_RAISED    0x0400
#define DB_DEPRESSED 0x0800

#define LINE_WIDTH_FACTOR 3
extern ULONG    ColorTab[];       /* SEE dlg_val.c                    */

static USHORT  ColorPat[]={ PATSYM_HALFTONE, PATSYM_SOLID, PATSYM_DENSE1,
                     PATSYM_DENSE2,   PATSYM_DENSE3,PATSYM_VERT,
                     PATSYM_HORIZ,    PATSYM_DIAG1, PATSYM_DIAG2,
                     PATSYM_DIAG3,    PATSYM_DEFAULT,
		     PATSYM_GRADIENTFILL, PATSYM_FOUNTAINFILL };

extern HAB hab;
/*-----------------------------------------------[ public ]---------------*/
/* measureColorItem                                                       */
/*                                                                        */
/* Description  : Defines the size of the color items in the color popup  */
/*                Function is called for fountain fill and shadow color   */
/*                Also called in this module. The popup of the blocktext  */
/*                uses it as well.                                        */
/*------------------------------------------------------------------------*/
MRESULT measureColorItem( MPARAM mp2)
{
   POWNERITEM poi;
   poi = (POWNERITEM)mp2;
   poi->rclItem.xRight = 5;
   poi->rclItem.yTop   = 20;
   return (MRESULT)0;
}
/*-----------------------------------------------[ public ]---------------*/
/* drawColorItem                                                          */
/*                                                                        */
/* Description  : Draws the color item of the color popupmenu. Used for   */
/*                defining the fountain fill color, shadow color and the  */
/*                blocktext color.                                        */
/*------------------------------------------------------------------------*/
MRESULT drawColorItem ( MPARAM mp2)
{
   LONG  lVertSideWidth,lHorizSideWidth;
   ULONG flCmd;             /* draw flags */
   RECTL rcl;
   POINTL ptl;
   POWNERITEM poi;

   poi = (POWNERITEM)mp2;
   if (!poi->fsAttribute &&
       (poi->fsAttributeOld & MIA_HILITED))
   {
      poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
   }
   /*
   ** Fill the total item first with PALEGRAY, hilited or not.
   ** If hilited the OS/2 takes care of it and will display
   ** it inverted.
   */
   WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);
   /*
   ** Make the color 5 pels smaller then the menuitem size
   ** So we get a nice gray border around the color
   */
   lVertSideWidth =2;
   lHorizSideWidth=2;
   flCmd = DB_DEPRESSED;
   GpiSetPattern(poi->hps,PATSYM_SOLID);
   WinDrawBorder(poi->hps, &poi->rclItem,lVertSideWidth,lHorizSideWidth,
                 0L, 0L, flCmd);

   rcl.xLeft = poi->rclItem.xLeft + 5;
   rcl.yBottom = poi->rclItem.yBottom + 5;
   rcl.yTop = poi->rclItem.yTop - 5;
   rcl.xRight = poi->rclItem.xRight - 5;
   GpiCreateLogColorTable ( poi->hps, LCOL_RESET, LCOLF_RGB, 0, 0, NULL );
   GpiSetColor(poi->hps,ColorTab[poi->idItem - IDM_CLR1]);
   ptl.x = rcl.xLeft;
   ptl.y = rcl.yBottom;
   GpiMove(poi->hps,&ptl);
   ptl.x = rcl.xRight;
   ptl.y = rcl.yTop;
   GpiBox(poi->hps,DRO_FILL,&ptl,0L,0L);
   return (MRESULT)0;
}

/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetLineWidthChecks.                                        */
/*-----------------------------------------------------------------------*/
static void menuSetLineWidthChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulLineWidth;
   USHORT usMenuId;

   if (pwi->pvCurrent && pwi->pvCurrent->usClass == CLS_TXT)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_NOOUTLINE,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));

   for (usMenuId = IDM_LNNORM; usMenuId <= IDM_LNEND; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));
   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulLineWidth = ObjGetLineWidth(pwi, pwi->pvCurrent);

   if ( ulLineWidth > 1 )
   {
      ulLineWidth /= LINE_WIDTH_FACTOR;
      usMenuId = IDM_LNNORM  + ulLineWidth;
      if (usMenuId > IDM_LNEND)
         usMenuId = IDM_LNEND;
   }
   else
   {
      usMenuId = IDM_LNNORM;
   }
   if (pwi->pvCurrent && pwi->pvCurrent->usClass == CLS_TXT)
   { 
      if (usMenuId == IDM_LNNORM && !isTxtOutlineSet(pwi->pvCurrent))
         usMenuId = IDM_NOOUTLINE;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetLineJoinChecks.                                         */
/*-----------------------------------------------------------------------*/
static void menuSetLineJoinChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulLineJoin;
   USHORT usMenuId;

   for (usMenuId = IDM_LNDEFAULT; usMenuId <= IDM_LNMITRE; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));
   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulLineJoin = ObjGetLineJoin(pwi, pwi->pvCurrent);

   switch(ulLineJoin)
   {
   case LINEJOIN_BEVEL:
      usMenuId = IDM_LNBEVEL;
      break;
   case LINEJOIN_ROUND:
      usMenuId = IDM_LNROUND;
      break;
   case LINEJOIN_MITRE:
      usMenuId = IDM_LNMITRE;
      break;
   default:                         /*LINEJOIN_DEFAULT*/
      usMenuId = IDM_LNDEFAULT;
      break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetLineTypeChecks.                                         */
/*                                                                       */
/*  Description : During the WM_INITMENU of the lintype         menu     */
/*                this function is called to set the appropriate check-  */
/*                marks. It uses the current selection to find out       */
/*                which selection should be set.                         */
/*                                                                       */
/*  Parameters :  WINDOWINFO * - pointer to the applictaion info.        */
/*                HWND hMenu   - Window handle of the submenu.           */
/*                                                                       */
/*  Returns: Nome                                                        */
/*-----------------------------------------------------------------------*/
static void menuSetLineTypeChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulLineType;
   USHORT usMenuId;

   for (usMenuId = IDM_LINEDEFAULT; usMenuId <= IDM_LINEALTERNATE; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));

   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulLineType = ObjGetLineType(pwi, pwi->pvCurrent);

   switch(ulLineType)
   {
   case LINETYPE_DEFAULT:
      usMenuId = IDM_LINEDEFAULT;
      break;
   case LINETYPE_DOT:
      usMenuId = IDM_LINEDOT;
      break;
   case LINETYPE_SHORTDASH:
      usMenuId = IDM_LINESHORTDASH;
      break;
   case LINETYPE_DASHDOT:
      usMenuId = IDM_LINEDASHDOT;
      break;
   case LINETYPE_DOUBLEDOT:
      usMenuId = IDM_LINEDOUBLEDOT;
      break;
   case LINETYPE_LONGDASH:
      usMenuId = IDM_LINELONGDASH;
      break;
   case LINETYPE_INVISIBLE:
      usMenuId = IDM_LINEINVISIBLE;
      break;
   case LINETYPE_ALTERNATE:
      usMenuId = IDM_LINEALTERNATE;
      break;
   case LINETYPE_DASHDOUBLEDOT:
      usMenuId = IDM_LINEDASHDOUBLEDOT;
      break;
   default:
      usMenuId = IDM_LINESOLID;
      break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetFillPatternChecks.                                      */
/*                                                                       */
/*  Description : During the WM_INITMENU of the filling pattern menu     */
/*                this function is called to set the appropriate check-  */
/*                marks. It uses the current selection to find out       */
/*                which selection should be set.                         */
/*                                                                       */
/*  Parameters :  WINDOWINFO * - pointer to the applictaion info.        */
/*                HWND hMenu   - Window handle of the submenu.           */
/*                                                                       */
/*  Returns: Nome                                                        */
/*-----------------------------------------------------------------------*/
static void menuSetFillPatternChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulPattern;
   USHORT usMenuId;

   for (usMenuId = IDM_HALFTONE; usMenuId <= IDM_FOUNTAIN; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));
   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulPattern = ObjGetFillType(pwi, pwi->pvCurrent);

   switch(ulPattern)
   {
      case PATSYM_HALFTONE:
         usMenuId = IDM_HALFTONE;
         break;
      case PATSYM_SOLID:
         usMenuId = IDM_SOLID;
         break;
      case PATSYM_DENSE1:
         usMenuId = IDM_DENSE1;
         break;
      case PATSYM_DENSE2:
         usMenuId = IDM_DENSE2;
         break;
      case PATSYM_VERT:
         usMenuId = IDM_VERT;
         break;
      case PATSYM_HORIZ:
         usMenuId = IDM_HORIZ;
         break;
      case PATSYM_DIAG1:
         usMenuId = IDM_DIAG1;
         break;
      case PATSYM_DIAG2:
         usMenuId = IDM_DIAG2;
         break;
      case PATSYM_DIAG3:
         usMenuId = IDM_DIAG3;
         break;
      case PATSYM_DEFAULT:
         usMenuId = IDM_NOPATTERN;
         break;
      case PATSYM_GRADIENTFILL:
         usMenuId = IDM_GRADIENT;
         break;
     case PATSYM_FOUNTAINFILL:
         usMenuId = IDM_FOUNTAIN;
         break;
     default:
         usMenuId = IDM_NOPATTERN;
         break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetShadeTypeChecks.                                        */
/*-----------------------------------------------------------------------*/
static void menuSetShadeTypeChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulShadeType;
   USHORT usMenuId;

   for (usMenuId = IDM_LBSHADE; usMenuId <= IDM_SHADENONE;  usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));

   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulShadeType = ObjGetShadeType(pwi, pwi->pvCurrent);

   switch(ulShadeType)
   {
   case SHADE_LEFTBOTTOM:
      usMenuId = IDM_LBSHADE;
      break;
   case SHADE_LEFTTOP:
      usMenuId = IDM_LTSHADE;
      break;
   case SHADE_RIGHTBOTTOM:
      usMenuId = IDM_RBSHADE;
      break;
   case SHADE_RIGHTTOP:
      usMenuId = IDM_RTSHADE;
      break;
   default:
      usMenuId = IDM_SHADENONE;
      break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetBlockTxtAlignChecks                                     */
/*                                                                       */
/*  Description : During the WM_INITMENU of the blocktext allignment menu*/
/*                this function is called to set the appropriate check-  */
/*                marks. It uses the current selection to find out       */
/*                which selection should be set.                         */
/*                                                                       */
/*  Parameters :  WINDOWINFO * - pointer to the applictaion info.        */
/*                HWND hMenu   - Window handle of the submenu.           */
/*                                                                       */
/*  Returns: Nome                                                        */
/*-----------------------------------------------------------------------*/
static void menuSetBlockTxtAlignChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulAlignment;
   USHORT usMenuId;

   for (usMenuId = IDM_BTEXTLEFT; usMenuId <= IDM_BTEXTCENTER; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));
   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulAlignment = ObjGetAlignment(pwi, pwi->pvCurrent);

   switch(ulAlignment)
   {
      case ALIGN_LEFT:
         usMenuId = IDM_BTEXTLEFT;
         break;
      case ALIGN_RIGHT:
         usMenuId = IDM_BTEXTRIGHT;
         break;
      case ALIGN_CENTER:
         usMenuId = IDM_BTEXTCENTER;
         break;
      case ALIGN_JUST:
         usMenuId = IDM_BTEXTJUST;
         break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------[ private ]-------------*/
/*  Name: menuSetBlockTxtSpaceChecks                                     */
/*                                                                       */
/*  Description : During the WM_INITMENU of the blocktext allignment menu*/
/*                this function is called to set the appropriate check-  */
/*                marks. It uses the current selection to find out       */
/*                which selection should be set.                         */
/*                                                                       */
/*  Parameters :  WINDOWINFO * - pointer to the applictaion info.        */
/*                HWND hMenu   - Window handle of the submenu.           */
/*                                                                       */
/*  Returns: Nome                                                        */
/*-----------------------------------------------------------------------*/
static void menuSetBlockTxtSpaceChecks(WINDOWINFO *pwi, HWND hMenu)
{
   ULONG  ulAlignment;
   USHORT usMenuId;

   for (usMenuId = IDM_BTEXTSPACENONE; usMenuId <= IDM_BTEXTSPACEDOUBLE; usMenuId++)
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
                 MPFROM2SHORT(MIA_CHECKED,0));
   /*
   ** Do not check any menu item when the user has made a multiple selection.
   */
   if (!isSingleSelection(pwi)) /*See dlg_sel.c */
      return;

   ulAlignment = ObjGetLineSpace(pwi, pwi->pvCurrent);

   switch(ulAlignment)
   {
      case SPACE_NONE:
         usMenuId = IDM_BTEXTSPACENONE;
         break;
      case SPACE_SINGLE:
         usMenuId = IDM_BTEXTSPACESINGLE;
         break;
      case SPACE_HALF:
         usMenuId = IDM_BTEXTSPACEHALF;
         break;
      case SPACE_DOUBLE:
         usMenuId = IDM_BTEXTSPACEDOUBLE;
         break;
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usMenuId,TRUE),
              MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
}
/*-----------------------------------------------------------------------*/
/*  Name: InitMenu                                                       */
/*                                                                       */
/*  Description : Processes the WM_INITMENU message for the main         */
/*                window, disabling any menus that are not active        */
/*                                                                       */
/*  Concepts : Routine is called each time a menu is selected.  A        */
/*             switch statement branches control based upon the          */
/*             id of the menu which is being displayed.                  */
/*                                                                       */
/*  API's : WinSendMsg                                                   */
/*          WinOpenClipbrd                                               */
/*          WinQueryClipbrdFmtInfo                                       */
/*          WinCloseClipbrd                                              */
/*                                                                       */
/*  Parameters :  mp1 - First message parameter                          */
/*                mp2 - Second message parameter                         */
/*                                                                       */
/*  Returns: Nome                                                        */
/*-----------------------------------------------------------------------*/
VOID InitMenu(WINDOWINFO *pwi,MPARAM mp1, MPARAM mp2,
              BOOL fHelpEnabled, VOID *pObj)
{
   BOOL    bEnable= FALSE;
   BOOL    bText  = FALSE;
   BOOL    bPal   = FALSE;
   BOOL    bClip  = FALSE; /* clippath on image???*/
   POBJECT p=NULL;

   if (pObj)
      p = (POBJECT)pObj;



   switch(SHORT1FROMMP(mp1))
   {
      case IDM_FILEMENU:
        bEnable = ObjectsAvail(); /* see drwutl.c */
        EnableMenuItem(HWNDFROMMP(mp2), IDM_SAVE, bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_SAVA,bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_PRINTAS,bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_PRINTPREVIEW,bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_METAEXP,bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_BITMAPEXP,bEnable);
        break;
      case IDM_HELP:
      /*
       * Enable or disable the Help menu depending upon whether the
       * help manager has been enabled
       */
         EnableMenuItem(HWNDFROMMP(mp2), IDM_HELPUSINGHELP, fHelpEnabled);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_HELPGENERAL, fHelpEnabled);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_HELPKEYS, fHelpEnabled);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_HELPINDEX, fHelpEnabled);
         break;
      case IDM_FONT:       /* I mean text... */
        if (p && p->usClass == CLS_TXT)
        {
           bEnable = TRUE;
        }

        if (p && (p->usClass == CLS_TXT || p->usClass == CLS_BLOCKTEXT))
        {
           bText = TRUE;
        }
        EnableMenuItem(HWNDFROMMP(mp2), IDM_SELTEXT, bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_EDITTEXT,bText);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_TXTCIRC, bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_CHARBOX, bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_FLIP,    bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_ROT,     bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_LNSIZE,  bEnable);
        EnableMenuItem(HWNDFROMMP(mp2), IDM_INSTEXT, 
                      (BOOL)(pwi->op_mode == TEXTINPUT));
        break;
      case IDM_IMAGE:
         if (p && p->usClass == CLS_IMG)
         {
           bEnable = TRUE;
           bClip   = imgHasClipPath(p);
           bPal    = (BOOL)p->pDrwPal;
           imgSetMenuCheckMarks(HWNDFROMMP(mp2),p);
         }
         EnableMenuItem(HWNDFROMMP(mp2), IDM_SELBMP, bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_FLIPIMG,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_CROP,   bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_CIRBMP, bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_NORBMP, bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_ROTATION,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_IMGOTHERFORMATS,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_GRAYSCALE,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_BLACKWHITE,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_IMGINVERT ,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_IMGBRIGHT ,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_COLORFILTER,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_IMGROT,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_RESTASPECT ,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_RESTORESIZE,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_RESTOREIMG,  bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_UNDOPALETTE,(bEnable && bPal));

         EnableMenuItem(HWNDFROMMP(mp2), IDM_COLORCOPY,(bEnable && bPal));
         EnableMenuItem(HWNDFROMMP(mp2), IDM_COLORSMENU,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_COLORPASTE,
                        (bEnable && bPal && pwi->colorPalette.prgb2));

         EnableMenuItem(HWNDFROMMP(mp2), IDM_SETCLIPPATH,ObjIsClipPathSelected());
         EnableMenuItem(HWNDFROMMP(mp2), IDM_DELCLIPPATH,(bClip && bEnable));
         EnableMenuItem(HWNDFROMMP(mp2), IDM_ROPCODE,bEnable);
         break;
      case IDM_RESTOREIMG:
         if (p && p->usClass == CLS_IMG)
         {
           bEnable = TRUE;
           bPal    = (BOOL)p->pDrwPal;
           EnableMenuItem(HWNDFROMMP(mp2), IDM_UNDOPALETTE,(bEnable && bPal));
         }
         break;
      case IDM_EDIT:
         if (DataOnClipBoard(hab))
            EnableMenuItem(HWNDFROMMP(mp2), IDM_PASTE,TRUE);
         else
            EnableMenuItem(HWNDFROMMP(mp2), IDM_PASTE,FALSE);

         if (p && (p->usClass == CLS_IMG || p->usClass == CLS_META) )
            bEnable = TRUE;
         EnableMenuItem(HWNDFROMMP(mp2), IDM_COPY,bEnable);
         EnableMenuItem(HWNDFROMMP(mp2), IDM_CUT,bEnable);

         if (p && (p->usClass == CLS_GROUP))
            EnableMenuItem(HWNDFROMMP(mp2), IDM_UNGROUP,TRUE);
         else
            EnableMenuItem(HWNDFROMMP(mp2), IDM_UNGROUP,FALSE);

         if (!p)  /* Delete option in edit menu */
            EnableMenuItem(HWNDFROMMP(mp2),IDM_CLEAR,FALSE);
         else
            EnableMenuItem(HWNDFROMMP(mp2),IDM_CLEAR,TRUE);
         break;

      case IDM_ROPCODE:
         if (p && p->usClass == CLS_IMG)
           imgSetMenuCheckMarks(HWNDFROMMP(mp2),p);
         break;
      case IDM_CST:                        /* Filling patterns */
         menuSetFillPatternChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_LINETYPE:
         menuSetLineTypeChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_LNJOIN:
         menuSetLineJoinChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_LNSIZE:
         menuSetLineWidthChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_BTEXTALIGN: /* Blocktext alignment*/
         menuSetBlockTxtAlignChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_BTEXTSPACE:
         menuSetBlockTxtSpaceChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_OBJSHADE:
         menuSetShadeTypeChecks(pwi,HWNDFROMMP(mp2));  /* See this module  */
         break;
      case IDM_ATTR:
         /*
         ** Copy attributes from selected object into application context.
         */
         EnableMenuItem(HWNDFROMMP(mp2),IDM_COPYATTR,
                        (BOOL)(isSingleSelection(pwi) && pObj));
         /*
         ** Copy attributes from application to selected object(s)
         */
         EnableMenuItem(HWNDFROMMP(mp2),IDM_COPYATTR2OBJ,
                        (BOOL)(pwi->op_mode == MULTISELECT  || pObj));
         break;
      default:
         break;
   }
}
/*********************************************************************
 *  Name: EnableMenuItem
 *
 *  Description : Enables or disables menu item.
 *
 *  Concepts : Called whenever a menu item is to be enabled or
 *             disabled.  Sends a MM_SETITEMATTR to the menu with
 *             the given item id.  Sets the MIA_DISABLED
 *             attribute flag if the item is to be disabled,
 *             clears the flag if enabling.
 *
 *  API's : WinSendMsg
 *
 *  Parameters   : hwnd - Window handle of the menu
 *                 sIditem  - Id of the menu item.
 *                 bEnable - Enable/Disable flag
 *
 *  Returns: Void
 *
 ****************************************************************/
VOID EnableMenuItem( HWND hwndMenu, SHORT sIditem, BOOL bEnable)
{
  SHORT sFlag;

  if(bEnable)
    sFlag = 0;
  else
    sFlag = MIA_DISABLED;

  WinSendMsg(hwndMenu, MM_SETITEMATTR, MPFROM2SHORT(sIditem, TRUE),
               MPFROM2SHORT(MIA_DISABLED, sFlag));

}   /*         End of EnableMenuItem()                                  */

/*-----------------------------------------------------------------------*/
/*  Name: SetButtonState                                                 */
/*                                                                       */
/*  Description : Enables/Disables the image menu. Depending of image    */
/*                selection.                                             */
/*-----------------------------------------------------------------------*/
void SetButtonState(HWND hwnd , USHORT ButtonId, BOOL bEnable)
{
   static USHORT prev;           /* Previous disabled button */

   WinEnableWindow(WinWindowFromID(hwnd, ButtonId),bEnable);

  if (prev && !bEnable && prev != ButtonId)
  {
     WinEnableWindow(WinWindowFromID(hwnd, prev),TRUE);
  }
  prev = ButtonId;
}
/*------------------------------------------------------------------------*/
/*  Name: MeasureItem.                                                    */
/*                                                                        */
/*  Description : Initializes the size of the ownerdrawn menuitems.       */
/*                The submenu with the filling patterns.                  */
/*                                                                        */
/*                                                                        */
/*  Parameters : MPARAM mp2.                                              */
/*                                                                        */
/*  Returns:  MRESULT 0.                                                  */
/*------------------------------------------------------------------------*/
MRESULT MeasureItem(MPARAM mp2)
{
   POWNERITEM poi;

   poi = (POWNERITEM)mp2;

   if (poi->idItem >= IDM_HALFTONE && poi->idItem <= IDM_DIAG3)
   {
      poi->rclItem.xRight = 30;
      poi->rclItem.yTop   = 30;
   }
   else if (poi->idItem >= IDM_ALIGNHORZ && poi->idItem <= IDM_ALNVERTR)
   {
      poi->rclItem.xRight = 40;
      poi->rclItem.yTop   = 26;
   }
   else if (poi->idItem >= IDM_LNNORM && poi->idItem <= IDM_LNEND)
   {
      poi->rclItem.xRight = 60;
      poi->rclItem.yTop   = 12 + (poi->idItem - IDM_LNNORM);
   }
   else if (poi->idItem >= IDM_LINEDOT && poi->idItem <= IDM_LINEALTERNATE)
   {
      poi->rclItem.xRight = 60;
      poi->rclItem.yTop   = 12;
   }
   else if (poi->idItem >= IDM_LBSHADE && poi->idItem <= IDM_RBSHADE)
   {
      poi->rclItem.xRight = 20;
      poi->rclItem.yTop   = 36;
   }
   else if (poi->idItem >= IDM_CLR1 && poi->idItem <= IDM_CLR40)
      measureColorItem(mp2);
   else if (poi->idItem >= IDM_ARROW1 && poi->idItem <= IDM_ARROW12)
   {
      poi->rclItem.xRight = 18;
      poi->rclItem.yTop   = 36;
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/* Name: DrawArrowMenu                                                    */
/*------------------------------------------------------------------------*/
static MRESULT drawArrowMenu(MPARAM mp2)
{
   POWNERITEM poi;
   HBITMAP hbm;
   ULONG   ulflBmp;

   poi = (POWNERITEM)mp2;

      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }

      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);


      if (poi->fsAttribute & MIA_HILITED)
         ulflBmp = DBM_STRETCH | DBM_INVERT;
      else
         ulflBmp = DBM_STRETCH;

      switch(poi->idItem)
      {
         case IDM_ARROW1:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW1,0L,0L);
            break;
         case IDM_ARROW2:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW2,0L,0L);
            break;
         case IDM_ARROW3:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW3,0L,0L);
            break;
         case IDM_ARROW4:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW4,0L,0L);
            break;
         case IDM_ARROW5:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW5,0L,0L);
            break;
         case IDM_ARROW6:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW6,0L,0L);
            break;
         case IDM_ARROW7:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW7,0L,0L);
            break;
         case IDM_ARROW8:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW8,0L,0L);
            break;
         case IDM_ARROW9:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW9,0L,0L);
            break;
         case IDM_ARROW10:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW10,0L,0L);
            break;
         case IDM_ARROW11:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW11,0L,0L);
            break;
         case IDM_ARROW12:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ARROW12,0L,0L);
            break;
      }
      poi->rclItem.xLeft   += 4;
      poi->rclItem.xRight  -= 4;
      poi->rclItem.yBottom += 4;
      poi->rclItem.yTop    -= 4;
      if (hbm)
      {
         WinDrawBitmap(poi->hps,
                       hbm,
                       NULL,
                       (PPOINTL)&poi->rclItem,  /* bit-map destination  */
                       0L,
                       0L,
                       ulflBmp);
      }
      return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: DrawMenuItem.                                                   */
/*                                                                        */
/*  Description : Draws the ownerdraw menuitems. In this case the fill    */
/*                pattern options.                                        */
/*                                                                        */
/*                                                                        */
/*  Parameters : MPARAM mp2.                                              */
/*                                                                        */
/*  Returns:  MRESULT 0.                                                  */
/*------------------------------------------------------------------------*/
MRESULT  DrawMenuItem(MPARAM mp2)
{
   POWNERITEM poi;
   POINTL  ptl;
   HBITMAP hbm;
   ULONG   ulflBmp;
   poi = (POWNERITEM)mp2;

   if (poi->idItem >= IDM_HALFTONE && poi->idItem <= IDM_NOPATTERN)
   {
      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }
      /*
      ** Fill the total item first with PALEGRAY, hilited or not.
      ** If hilited the OS/2 takes care of it and will display
      ** it inverted.
      */
      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);
      /*
      ** Make the color 5 pels smaller then the menuitem size
      ** So we get a nice gray border around the color.
      */
      GpiSetPattern(poi->hps,ColorPat[poi->idItem-IDM_HALFTONE]);
      ptl.x = poi->rclItem.xLeft + 15;
      ptl.y = poi->rclItem.yTop - 5;
      GpiMove(poi->hps,&ptl);
      ptl.y = poi->rclItem.yBottom + 5;
      ptl.x = poi->rclItem.xRight - 10;
      GpiSetColor(poi->hps,CLR_BLUE);
      GpiBox(poi->hps,DRO_FILL,&ptl,0,0);
      GpiSetColor(poi->hps,CLR_BLACK);
   }
   else if (poi->idItem >= IDM_LNNORM && poi->idItem <= IDM_LNEND)
   {
      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }
      /*
      ** Fill the total item first with PALEGRAY, hilited or not.
      ** If hilited the OS/2 takes care of it and will display
      ** it inverted.
      */
      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);
      GpiSetLineType(poi->hps,LINETYPE_SOLID);
      ptl.x = poi->rclItem.xLeft   + 5;
      ptl.y = poi->rclItem.yBottom + 5;
      GpiMove(poi->hps,&ptl);
      ptl.x = poi->rclItem.xRight  - 5;
      ptl.y = poi->rclItem.yBottom + 6 + (poi->idItem - IDM_LNNORM);
      GpiSetColor(poi->hps,CLR_BLUE);
      GpiBox(poi->hps,DRO_FILL,&ptl,0,0);
   }
   else if (poi->idItem >= IDM_ALIGNHORZ && poi->idItem <= IDM_ALNVERTR)
   {
      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }

      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);


      if (poi->fsAttribute & MIA_HILITED)
         ulflBmp = DBM_STRETCH | DBM_INVERT;
      else
         ulflBmp = DBM_STRETCH;

      switch(poi->idItem)
      {
         case IDM_ALIGNHORZ:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALIGNHORZ,0L,0L);
            break;
         case IDM_ALNHORZB:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALNHORZB,0L,0L);
            break;
         case IDM_ALNHORZT:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALNHORZT,0L,0L);
            break;
         case IDM_ALIGNVERT:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALIGNVERT,0L,0L);
            break;
         case IDM_ALNVERTL:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALNVERTL,0L,0L);
            break;
         case IDM_ALNVERTR:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_ALNVERTR,0L,0L);
            break;
      }
      poi->rclItem.xLeft   += 4;
      poi->rclItem.xRight  -= 4;
      poi->rclItem.yBottom += 4;
      poi->rclItem.yTop    -= 4;
      if (hbm)
      {
         WinDrawBitmap(poi->hps,
                       hbm,
                       NULL,
                       (PPOINTL)&poi->rclItem,  /* bit-map destination  */
                       0L,
                       0L,
                       ulflBmp);
      }
   }
   else if (poi->idItem >= IDM_LINEDOT && poi->idItem <= IDM_LINEALTERNATE)
   {

      ULONG ulLinetype = (poi->idItem - IDM_LINEDEFAULT);

      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }
      /*
      ** Fill the total item first with PALEGRAY, hilited or not.
      ** If hilited the OS/2 takes care of it and will display
      ** it inverted.
      */
      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);
      GpiSetLineType(poi->hps,ulLinetype);
      ptl.x = poi->rclItem.xLeft   + 5;
      ptl.y = poi->rclItem.yBottom + 6;
      GpiMove(poi->hps,&ptl);
      ptl.x = poi->rclItem.xRight  - 5;
      GpiSetColor(poi->hps,CLR_BLACK);
      GpiLine(poi->hps,&ptl);
   }
   else if (poi->idItem >= IDM_LBSHADE && poi->idItem <= IDM_RBSHADE)
   {
      if (!poi->fsAttribute &&
          (poi->fsAttributeOld & MIA_HILITED))
      {
         poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
      }

      WinFillRect(poi->hps,&poi->rclItem,CLR_PALEGRAY);


      if (poi->fsAttribute & MIA_HILITED)
         ulflBmp = DBM_STRETCH | DBM_INVERT;
      else
         ulflBmp = DBM_STRETCH;

      switch(poi->idItem)
      {
         case IDM_LBSHADE:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_LBSHADE,0L,0L);
            break;
         case IDM_LTSHADE:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_LTSHADE,0L,0L);
            break;
         case IDM_RTSHADE:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_RTSHADE,0L,0L);
            break;
         case IDM_RBSHADE:
            hbm = GpiLoadBitmap(poi->hps,NULLHANDLE,IDB_RBSHADE,0L,0L);
            break;
      }
      poi->rclItem.xLeft   += 4;
      poi->rclItem.xRight  -= 4;
      poi->rclItem.yBottom += 4;
      poi->rclItem.yTop    -= 4;
      if (hbm)
      {
         WinDrawBitmap(poi->hps,
                       hbm,
                       NULL,
                       (PPOINTL)&poi->rclItem,  /* bit-map destination  */
                       0L,
                       0L,
                       ulflBmp);
      }
   }
   else if (poi->idItem >= IDM_CLR1 && poi->idItem <= IDM_CLR40)
      drawColorItem (mp2);
   else if (poi->idItem >= IDM_ARROW1 && poi->idItem <= IDM_ARROW12)
      drawArrowMenu(mp2);
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: MenuFillPattern.                                                */
/*                                                                        */
/*  Description : Handles the selections in the filling pattern submenu.  */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuFillPattern(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi)
{
   ULONG  ulColorPattern;
   RECTL  rcl;

   ulColorPattern = ColorPat[MenuId-IDM_HALFTONE];

   if (pwi->pvCurrent)
   {
      if (ObjPatternChange(ulColorPattern,pwi->pvCurrent,pwi,TRUE))
         ObjRefresh(pwi->pvCurrent,pwi);
   }
   else if (pwi->op_mode != MULTISELECT && !pwi->pvCurrent)
   {
      pwi->ColorPattern = ulColorPattern;
      if (pwi->ColorPattern == PATSYM_FOUNTAINFILL)
         WinLoadDlg(HWND_DESKTOP,pwi->hwndClient,(PFNWP)FountainDlgProc,0L,
                    ID_DLGFOUNTAIN,(PVOID)&pwi->fountain);
      else if (pwi->ColorPattern == PATSYM_GRADIENTFILL)
         WinLoadDlg(HWND_DESKTOP,pwi->hwndClient,(PFNWP)GradientDlgProc,
                          0L,ID_GRADIENT,(PVOID)&pwi->Gradient);
   }
   else  if (pwi->op_mode == MULTISELECT)
   {
      ObjBoundingRect(pwi,&rcl,TRUE);
      ObjMultiPatternChange(ulColorPattern,pwi);
      WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   }
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: MenuLineSize.                                                   */
/*                                                                        */
/*  Description : Handles the selections in the linesize submenu.         */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuLineSize(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi)
{
   ULONG  ulLineWidth;
   RECTL  rcl;

   ulLineWidth = (MenuId - IDM_LNNORM);
   if ( ulLineWidth > 1 )
      ulLineWidth *= LINE_WIDTH_FACTOR;

   if (pwi->pvCurrent)
   {
      ObjLnWidthChange(ulLineWidth,pwi->pvCurrent);
      if (pwi->pvCurrent->usClass == CLS_TXT)
         setTxtOutline(pwi->pvCurrent,TRUE);
      ObjRefresh(pwi->pvCurrent,pwi);
   }
   else  if (pwi->op_mode == MULTISELECT)
   {
      ObjBoundingRect(pwi,&rcl,TRUE);
      ObjMultiLnWidthChange(ulLineWidth);
      WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
    }
    else
      pwi->lLnWidth = ulLineWidth;

   return (MRESULT)0;

}
/*------------------------------------------------------------------------*/
/*  Name: MenuLineJoin.                                                   */
/*                                                                        */
/*  Description : Handles the selections in the linejoin submenu.         */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuLineJoin(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi)
{
   ULONG  ulLineJoin;
   RECTL  rcl;

   switch(MenuId)
   {
      case IDM_LNROUND:
         ulLineJoin = LINEJOIN_ROUND;
         break;
      case IDM_LNBEVEL:
         ulLineJoin = LINEJOIN_BEVEL;
         break;
      case IDM_LNMITRE:
         ulLineJoin = LINEJOIN_MITRE;
         break;
      default:
         ulLineJoin = LINEJOIN_DEFAULT;
         break;
   }

   if (pwi->pvCurrent)
   {
      ObjLnJoinChange(ulLineJoin,pwi->pvCurrent);
      ObjRefresh(pwi->pvCurrent,pwi);
   }
   else  if (pwi->op_mode == MULTISELECT)
   {
      ObjBoundingRect(pwi,&rcl,TRUE);
      ObjMltLnJoinChange(ulLineJoin);
      WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   }
   else
      pwi->lLnJoin = ulLineJoin;

   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: MenuLineType.                                                   */
/*                                                                        */
/*  Description : Handles the selections in the linetype submenu.         */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuLineType(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi)
{
   ULONG  ulLineType;
   RECTL  rcl;
   ulLineType   = MenuId - IDM_LINEDEFAULT;

   if (pwi->pvCurrent)
   {
      ObjLntypeChange(ulLineType,pwi->pvCurrent);
      ObjRefresh(pwi->pvCurrent,pwi);
   }
   else  if (pwi->op_mode == MULTISELECT)
   {
      ObjBoundingRect(pwi,&rcl,TRUE);
      ObjMultiLntypeChange(ulLineType);
      WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
   }
   else
      pwi->lLntype = ulLineType;
   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: MenuShadeType                                                   */
/*                                                                        */
/*  Description : Handles the selections in the shadetype submenu.        */
/*                                                                        */
/*  Parameters : None                                                     */
/*                                                                        */
/*  Returns:  None                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuShadeType(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi)
{
   ULONG ulShade;
   RECTL rcl;
   /*
   ** Just conventional code, the user selected a little shading
   ** picture from the menu.
   */
   switch ( MenuId)
   {
      case IDM_LBSHADE:
         ulShade = SHADE_LEFTBOTTOM;
         break;
      case IDM_LTSHADE:
         ulShade = SHADE_LEFTTOP;
         break;
      case IDM_RBSHADE:
         ulShade = SHADE_RIGHTBOTTOM;
         break;
      case IDM_RTSHADE:
         ulShade = SHADE_RIGHTTOP;
         break;
      case IDM_SHADECLR:
         /*
         ** Get the dialog to change the shading.....
         */
         if (pwi->pvCurrent)
         {
            if (ObjShadeChange(SHADE_CHANGE,pwi->pvCurrent,pwi,TRUE))
               ObjRefresh(pwi->pvCurrent,pwi);
         }
         else if (pwi->op_mode != MULTISELECT && !pwi->pvCurrent)
         {
            setupShading(pwi); /* Setup dialog */
         }
         else  if (pwi->op_mode == MULTISELECT)
         {
            setupShading(pwi); /* Setup dialog */
            ObjBoundingRect(pwi,&rcl,TRUE);
            ObjMultiShadeChange(SHADE_CHANGE,pwi);
            WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
         }
         return (MRESULT)0;

      default:
         ulShade = SHADE_NONE;
         break;
      }

      if (pwi->pvCurrent)
      {
         if (ObjShadeChange(ulShade,pwi->pvCurrent,pwi,FALSE))
            ObjRefresh(pwi->pvCurrent,pwi);
      }
      else  if (pwi->op_mode == MULTISELECT)
      {
         ObjBoundingRect(pwi,&rcl,TRUE);
         ObjMultiShadeChange(ulShade,pwi);
         WinInvalidateRect(pwi->hwndClient,&rcl,TRUE);
      }
      else
         pwi->Shade.lShadeType = ulShade;

   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: ShowPopupMenu                                                   */
/*                                                                        */
/*  Description : Shows the popupmenu with the handle hMenu and as owner  */
/*                hwnd.                                                   */
/*                                                                        */
/*  Parameters  : HWND - ownerwindow.                                     */
/*                WINDOWINFO *pwi (const)                                 */
/*                MPARAM - first message parameter used for position.     */
/*                                                                        */
/*  Returns     : MRESULT.                                                */
/*------------------------------------------------------------------------*/
MRESULT ShowPopupMenu(WINDOWINFO *pwi, HWND hwnd,ULONG msg,POINTL point)
{
   HWND hMenu = NULLHANDLE;

   if (pwi->op_mode == IMGSELECT)
      hMenu = pwi->hwndImgMenu;
   else if (pwi->op_mode == TEXTSELECT)
      hMenu = pwi->hwndTxtMenu;
   else if (ObjIsClipPathSelected())
      hMenu = pwi->hwndClipMenu;
   else if (pwi->op_mode == BLOCKTEXTSEL)
      hMenu = pwi->hwndBlockMenu;
   else if (pwi->op_mode == METAPICSELECT)
      hMenu = pwi->hwndMetaMenu;
   else
      hMenu = pwi->hwndOptMenu;

   if (!hMenu)
      return (MRESULT)0;
   /*
   ** Our mouse pointer position.
   */

   WinPopupMenu ( hwnd, hwnd, hMenu,
                  point.x,
                  point.y,
                  0,
                  PU_NONE |
                  PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 | PU_KEYBOARD |
                  PU_HCONSTRAIN | PU_VCONSTRAIN);

    return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
/*  Name: Menuhelptext                                                    */
/*                                                                        */
/*  Description : Shows an explanation of the menuitem in the titlebar    */
/*                of the application.                                     */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void Menuhelptext(HWND hwnd, HAB hab, MPARAM mp1)
{
   CHAR szBuf[MAXNAMEL];

   if ( WinLoadString(hab, (HMODULE)0, SHORT1FROMMP(mp1), MAXNAMEL, szBuf))
       WinSetWindowText(hwnd,szBuf);
   else
      UpdateTitleText(hwnd);

   return;
}
/*------------------------------------------------------------------------*/
/*  Name: MenuZoom                                                        */
/*                                                                        */
/*  Note        : BOOL bMenu is false when zooming is done by the         */
/*                toolpalette.                                            */
/*                                                                        */
/*------------------------------------------------------------------------*/
MRESULT MenuZoom(WINDOWINFO *pwi,HWND hMenu,USHORT usMenuId, BOOL bMenu)
{
   static USHORT usZoom;


   if (!usZoom)
   {
      /*
      ** First menu access.
      ** Initially the 100% is checked (see drawit.rc)
      ** So we can savely uncheck it here and check the
      ** new one.
      */
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(IDM_ZOOM10, TRUE),
                 MPFROM2SHORT(MIA_CHECKED, 0));

   }
   else
   {
      WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usZoom, TRUE),
                 MPFROM2SHORT(MIA_CHECKED, 0));
   }

   usZoom = usMenuId;

   if (bMenu)
   {
   switch(usMenuId)
   {
      case IDM_ZOOM2:
         PercentageZoom(pwi,25);
         break;
      case IDM_ZOOM5:
         PercentageZoom(pwi,50);
         break;
      case IDM_ZOOM10:
         PercentageZoom(pwi,100);
         break;
      case IDM_ZOOM15:
         PercentageZoom(pwi,150);
         break;
      case IDM_ZOOM20:
         PercentageZoom(pwi,200);
         break;
      case IDM_ZOOM25:
         PercentageZoom(pwi,250);
         break;
      case IDM_ZOOM30:
         PercentageZoom(pwi,300);
         break;
   }
   }
   WinSendMsg(hMenu,MM_SETITEMATTR,MPFROM2SHORT(usZoom, TRUE),
              MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));

   return (MRESULT)0;
}
/*------------------------------------------------------------------------*/
static void MenuRopCode(USHORT usCommand,USHORT usMenuId,WINDOWINFO * pwi)
{
   LONG lRop;

   if (!pwi->pvCurrent)
      return;

   if (pwi->pvCurrent->usClass != CLS_IMG)
      return;

   switch (usCommand)
   {
      case IDM_SRCCOPY:
         lRop = ROP_SRCCOPY;
         break;      
      case IDM_SRCPAINT:
         lRop = ROP_SRCPAINT;
         break;
      case IDM_SRCAND:
         lRop = ROP_SRCAND;
         break;
      case IDM_SRCINVERT:
         lRop = ROP_SRCINVERT;
         break;
      case IDM_SRCERACE:
         lRop = ROP_SRCERASE;
         break;
      case IDM_NOTSRCCOPY:
         lRop = ROP_NOTSRCCOPY;
         break;
      case IDM_NOTSRCERASE:
         lRop = ROP_NOTSRCERASE;
         break;
      case IDM_MERGECOPY:
         lRop = ROP_MERGECOPY;
         break;
      case IDM_PATCOPY:
         lRop = ROP_PATCOPY;
         break;
      case IDM_PATPAINT:
         lRop = ROP_PATPAINT;
         break;
      case IDM_PATINVERT:
         lRop = ROP_PATINVERT;
         break;
      case IDM_PATDSTINVERT:
         lRop = ROP_DSTINVERT;
         break;
      case IDM_PATZERO:
         lRop = ROP_ZERO;
         break;
      case IDM_PATONE:
         lRop = ROP_ONE;
         break;
      default:
         return;  /* error */
   }
   ObjChangeRopCode(pwi,pwi->pvCurrent,lRop);
}
/*------------------------------------------------------------------------*/
static void MenuBlockTextAlign(USHORT usCommand,USHORT usMenuId,WINDOWINFO * pwi)
{
   USHORT usAlign;

   if (!pwi->pvCurrent)
      return;

   if (pwi->pvCurrent->usClass != CLS_BLOCKTEXT)
      return;
   switch (usCommand)
   {
      case IDM_BTEXTLEFT:
         usAlign = ALIGN_LEFT;
         break;      
      case IDM_BTEXTRIGHT:
         usAlign = ALIGN_RIGHT;
         break;      
      case IDM_BTEXTCENTER:
         usAlign = ALIGN_CENTER;
         break;      
      case IDM_BTEXTJUST:
         usAlign = ALIGN_JUST;
         break;      
      default:
         return; /* ERROR */
   }

   if (ObjChangeAlignment(pwi,pwi->pvCurrent,usAlign))
      ObjRefresh(pwi->pvCurrent,pwi);
}
/*------------------------------------------------------------------------*/
static void MenuBlockTextSpacing(USHORT usCommand,USHORT usMenuId,WINDOWINFO * pwi)
{
   USHORT usAlign;

   if (!pwi->pvCurrent)
      return;

   if (pwi->pvCurrent->usClass != CLS_BLOCKTEXT)
      return;
   switch (usCommand)
   {
      case IDM_BTEXTSPACENONE:
         usAlign = SPACE_NONE;
         break;      
      case IDM_BTEXTSPACESINGLE:
         usAlign = SPACE_SINGLE;
         break;      
      case IDM_BTEXTSPACEHALF:
         usAlign = SPACE_HALF;
         break;      
      case IDM_BTEXTSPACEDOUBLE:
         usAlign = SPACE_DOUBLE;
         break;      
      default:
         return; /* ERROR */
   }

   if (ObjChangeSpacing(pwi,pwi->pvCurrent,usAlign))
      ObjRefresh(pwi->pvCurrent,pwi);
}
/*------------------------------------------------------------------------*/
/* handleMenuCommand.   Handles the WM_COMMAND.                           */
/*                                                                        */
/* Returns BOOL   TRUE when message is handled. FALSE otherwise.          */
/*------------------------------------------------------------------------*/
BOOL handleMenuCommand(WINDOWINFO *pwi,HWND hwndMFrame, MPARAM mp1)
{
   USHORT usCommand = LOUSHORT(mp1);
   /*
   ** FILLING PATTERNS....
   */
   if (usCommand >= IDM_HALFTONE && usCommand <= IDM_FOUNTAIN)
   {
      MenuFillPattern(usCommand,
                      WinWindowFromID(hwndMFrame, FID_MENU),
                      pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_LNNORM && usCommand <= IDM_LNEND)
   {
      MenuLineSize(usCommand,
                   WinWindowFromID(hwndMFrame, FID_MENU),
                   pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_LNDEFAULT && usCommand <= IDM_LNMITRE)
   {
      MenuLineJoin(usCommand,
                   WinWindowFromID(hwndMFrame,FID_MENU),
                   pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_LINEDOT &&
            usCommand <= IDM_LINEALTERNATE)
   {
      MenuLineType(usCommand,
                   WinWindowFromID(hwndMFrame,FID_MENU),
                   pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_SRCCOPY &&
            usCommand <= IDM_PATONE)
   {
     MenuRopCode(usCommand,
                 WinWindowFromID(hwndMFrame,FID_MENU),
                 pwi);
     return TRUE;
   }
   else if (usCommand >= IDM_BTEXTLEFT && usCommand <= IDM_BTEXTCENTER)
   {
      MenuBlockTextAlign(usCommand,
                   WinWindowFromID(hwndMFrame, FID_MENU),
                   pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_BTEXTSPACENONE && 
            usCommand <= IDM_BTEXTSPACEDOUBLE)
   {
      MenuBlockTextSpacing(usCommand,
                   WinWindowFromID(hwndMFrame, FID_MENU),
                   pwi);
      return TRUE;
   }
   else if ((usCommand >= IDM_LBSHADE &&
            usCommand <= IDM_SHADENONE ) || usCommand == IDM_SHADECLR)
   {
      MenuShadeType(usCommand,
                   WinWindowFromID(hwndMFrame, FID_MENU),
                   pwi);
      return TRUE;
   }
   else if (usCommand >= IDM_CLR1 && usCommand <= IDM_CLR40)
   {
      ULONG ulColor = ColorTab[ LOUSHORT(mp1) - IDM_CLR1 ];
      if (pwi->pvCurrent && pwi->pvCurrent->usClass == CLS_BLOCKTEXT)
      {
         setColumnColor(pwi->pvCurrent,ulColor);
         ObjRefresh(pwi->pvCurrent,pwi);
      }
      return TRUE;
   }
   else if (usCommand == IDM_SAVEMETA )
      saveMetaPicture(pwi->pvCurrent,pwi);
   else if (usCommand >= IDM_ARROW1 && usCommand <= IDM_ARROW12)
   {
      changeMode(pwi,INSERTPICTURE,0L);
      pwi->lFigure = (LONG)(usCommand - IDM_ARROW1);
      return TRUE;
   }
   return FALSE;
}
