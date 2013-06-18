/*------------------------------------------------------------------------*/
/*  Name: dlg_file.c                                                      */
/*                                                                        */
/*  Description : Functions for file handling and title text of the app   */
/*                window.                                                 */
/*                                                                        */
/*  Functions  :                                                          */
/*    delTitle : Deletes the filename from the application title.         */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
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
#include "dlg_file.h"
#include "drwutl.h"
#include "drwcanv.h"
#include "drwfile.h"
#include "resource.h"

#define MESSAGELEN        150    /* maximum length for messages */
#define TITLESEPARATORLEN   4

#define DRWEXTENSION        ".JSP"

/*
** Both string must have the same size!!!
*/
#define FILESIGNE           "JASPER DE KEIJZERV24"
#define FILESIGNE2          "--DRAWIT VERSION25--"
#define FILESIGNE3          "--DRAWIT VERSION28--"
#define FILESIGNE4          "--DRAWIT VERSION29--"
#define FILESIGNE5          "--DRAWIT VERSION30--"
#define FILESIGNE6          "--DRAWIT VERSION32--"

CHAR szFullPath[CCHMAXPATH] = "";
extern HWND hwndMFrame;                 /*!!!!!!!!!!!!!!!!MUST BE IN DLG.H!!!*/

#ifdef DEMOVERSION
BOOL demoMessage(WINDOWINFO *pwi)
{
 WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
              (PSZ)"Sorry this is a demo version.\n"\
              "Data can only be saved as \nOS/2 Metafile\n"\
              "Register this version now!\n\n"\
              "http://www.bmtmicro.com",
              (PSZ)"Demo version! <Unregistered>", 0,
              MB_OK | MB_APPLMODAL |
              MB_MOVEABLE |
              MB_ICONEXCLAMATION);
  return FALSE;
}
/* end of demo code */
#endif
/*-----------------------------------------------------------------------*/
BOOL FileNew(HWND hwnd, WINDOWINFO *pwi)
{
    USHORT usResponse;
    char   szBuf[MESSAGELEN];
    char   szWarning[50];


    /*
    ** If there are no objects to save, don't bother user
    ** with messagebox.
    */
    if (!ObjectsAvail()) /* see drwutl.c*/
        return TRUE;



    WinLoadString(hab, (HMODULE)0, IDS_ASKFORSAVE,MESSAGELEN,(PSZ)szBuf);
    WinLoadString(hab, (HMODULE)0, IDS_WARNING,50,(PSZ)szWarning);

    usResponse = WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                               (PSZ)szBuf,(PSZ)szWarning, 0,
                               MB_YESNOCANCEL | MB_APPLMODAL |
                               MB_MOVEABLE |
                               MB_ICONQUESTION);
   if (usResponse == MBID_YES)
      WriteDrwfile(pwi);
   else if (usResponse == MBID_CANCEL)
      return FALSE; /* no action just escape..*/

    /* clear file name and reset the titlebar text */
   szFullPath[0] = '\0';
   UpdateTitleText(hwndMFrame);
   return TRUE;
}   /* End of FileNew   */
/*-----------------------------------------------------------------------*/
/*  Name:   FileGet()                                                    */
/*                                                                       */
/*  Purpose: Processes the File menu's Open item.                        */
/*                                                                       */
/*  Usage:  called whenever New from the File menu is selected           */
/*                                                                       */
/*  Method:  calls the standard file open dialog to get the              */
/*          file name.                                                   */
/*                                                                       */
/*  Uses : A pointer to the Loadinfo struct which must be filled         */
/*         in with the extension at least (.MET or .BMP etc).            */
/*         Returns the full path and file name in the struct filename    */
/*         element.                                                      */
/*                                                                       */
/*                                                                       */
/*  Returns: TRUE   on success + full filename + path in load            */
/*                  info struct.                                         */
/*           FALSE  on error                                             */
/*-----------------------------------------------------------------------*/
BOOL FileGetName(pLoadinfo pli, WINDOWINFO *pwi )
{
   PSZ apszITL[]={"DrawIt", (PSZ)NULL};
   struct stat bf;
   char     buf[CCHMAXPATH];
   char     szExtension[20]; /* for open files */
   USHORT   usResponse;
   BOOL     bDone=FALSE;
   FILEDLG  fdg;
   char     TitleBuf[50];
   char     szTitle[100]; /* title for the messagebox loaded from resource */
   char  *p;


#ifdef DEMOVERSION

/* Start of democode!!!!*/

 if ((pli->dlgflags & FDS_SAVEAS_DIALOG) ||
     !(pli->dlgflags & FDS_OPEN_DIALOG))
 {
   return demoMessage(pwi);
 }
/* end of demo code */
#endif


   memset(&fdg, 0,sizeof(FILEDLG));
   /*
   ** set up structure to pass to standard file dialog
   */
   fdg.cbSize = sizeof(FILEDLG);
   fdg.fl = pli->dlgflags | FDS_CENTER;

   strcpy(TitleBuf,"DrawIt - Default extension = ");
   strcat(TitleBuf,pli->szExtension);

   strcpy(szExtension,"*"); /* to make later *.net,*.jsp etc or *.* */

   fdg.pszTitle = TitleBuf;

   fdg.pszOKButton = "OK";
   fdg.ulUser = 0L;
   fdg.pfnDlgProc = (PFNWP)NULL;
   fdg.lReturn = 0L;
   fdg.lSRC = 0;
   fdg.hMod = 0;
   fdg.usDlgId = 0;
   fdg.x = 0;
   fdg.y = 0;
   fdg.pszIType = (PSZ)NULL;
   fdg.papszITypeList = (PAPSZ)NULL;
   fdg.pszIDrive = (PSZ)NULL;
   fdg.papszIDriveList = (PAPSZ)NULL;
   fdg.sEAType = (SHORT)0;
   fdg.papszFQFilename = (PAPSZ)NULL;
   fdg.ulFQFCount = 0L;
   fdg.szFullFile[0]=0;

   /*
   ** if dialog has been previously invoked, then ensure the
   ** dialog is brought with the last user-selected directory
   ** start with FDS_OPEN_DIALOG overwrite if not this....
   */

   if (pli->dlgflags & FDS_OPEN_DIALOG)
   {
      if (!strlen(pli->szdefdir))
      {
         pli->szdefdir[0]=0;
         strcat(szExtension,pli->szExtension);
         strcpy(fdg.szFullFile,szExtension);
      }
      else
      {
         strcpy(fdg.szFullFile,pli->szdefdir);
         strcat(fdg.szFullFile,"\\");
         strcat(szExtension,pli->szExtension);
         strcat(fdg.szFullFile,szExtension);
      }
   }

   if (pli->dlgflags & FDS_SAVEAS_DIALOG)
   {
      if (pli->szFileName[0])
         strcpy(fdg.szFullFile, pli->szFileName);
   }
   fdg.sEAType = -1;
   fdg.papszFQFilename = 0;
   fdg.ulFQFCount = 0;

   /*
   ** invoke the standard file dialog and get a filename
   ** If we are getting a filename for saving data we check
   ** if the file already exists. And if true we ask the
   ** if it is ok to use this filename.
   */

   if (pli->dlgflags & FDS_SAVEAS_DIALOG)
   {
      do
      {
         if (!WinFileDlg(HWND_DESKTOP,hwndMFrame,(PFILEDLG)&fdg))
            return FALSE;
         /*
         ** Don't touch the pli->szFileName if the cancel button
         ** is pressed. So quit.
         */
         if (fdg.lReturn == DID_CANCEL)
            return FALSE;

         strcpy(pli->szFileName, fdg.szFullFile);
         if (!(p = strchr(pli->szFileName,'.')) )
            strcat(pli->szFileName,pli->szExtension);
         else if (stricmp(p,pli->szExtension))
         {
            /*
            ** If the extension is not as it should be
            ** we simply overwrite it with the default.
            */
            *p = 0;
            strcat(pli->szFileName,pli->szExtension);
         }

         bDone = FALSE;
         if (_stat(pli->szFileName,&bf) == 0)
         {
            WinLoadString(hab, (HMODULE)0, IDS_WARNING,100,(PSZ)szTitle);

            sprintf(buf,"File: %s \nalready exists!\n"\
                    "Do you want to overwrite it?",pli->szFileName);

            usResponse = WinMessageBox(HWND_DESKTOP,pwi->hwndClient,
                                       (PSZ)buf,(PSZ)szTitle, 0,
                                       MB_YESNOCANCEL | MB_APPLMODAL |
                                       MB_MOVEABLE |
                                       MB_ICONQUESTION);
            bDone = TRUE;
         }
     } while (usResponse == MBID_NO && bDone);

     if (usResponse == MBID_CANCEL)
        return FALSE;

     if (usResponse == MBID_YES)
     {
        /*
        ** Try to remove the old file.
        */
        remove(pli->szFileName);
     }

   }
   else
   {
      /*
      ** OPEN FILEDIALOG...
      */
      if (!WinFileDlg(HWND_DESKTOP,hwndMFrame,(PFILEDLG)&fdg))
         return FALSE;

      if (fdg.lReturn == DID_CANCEL)
         return FALSE;
   }

   /*
   **  Upon sucessful return of a filename in the struct *loadinfo struct.
   */

   if (fdg.lReturn == DID_OK)
   {
      strcpy(pli->szFileName, fdg.szFullFile);

      if (pli->dlgflags & FDS_SAVEAS_DIALOG)
      {
         if (!(p = strchr(pli->szFileName,'.')) )
            strcat(pli->szFileName,pli->szExtension);
         else if (stricmp(p,pli->szExtension))
         {
         /*
         ** If the extension is not as it should be
         ** we simply overwrite it with the default.
         */
         *p = 0;
         strcat(pli->szFileName,pli->szExtension);
      }
   }
   else
   {
      /*
      ** Help the user who forgot the extension for opening
      ** the file.
      */
      if (!(p = strchr(pli->szFileName,'.')) )
         strcat(pli->szFileName,pli->szExtension);
   }
   p = strrchr(fdg.szFullFile,'\\');
   /*
   ** Finally save the directory in the loadinfo struct.
   ** So whenever the user comes back he/she would not
   ** have to travel all the way back to the directory.
   */
   if (p)
   {
      *p=0;
      strcpy(pli->szdefdir,fdg.szFullFile);
   }
   return TRUE;
 }
 else
    return FALSE;

}   /* end FileGet() */
/*------------------------------------------------------------------------*/
void ReadDrwfile(char *filename, WINDOWINFO *pwi)
{
   static Loadinfo  li;
   PVOID  pFileBegin;          /* beginning of file data */
   char   szBuf[MESSAGELEN];   /* Buffer for loading an error message */
   char   szError[100];        /* Buffer for loading an error message */
   char   buf[50];

   pFileBegin = (PVOID)buf;
   li.dlgflags = FDS_OPEN_DIALOG;

   strcpy(li.szExtension,".JSP");        /* Set default extension */

   if (!filename)
   {
      if (!FileGetName(&li,pwi))
         return;

      /* First of all, check the extension. */

      if (!strchr(li.szFileName,'.'))
         strcat(li.szFileName,".JSP");


      /* copy file name into file name buffer */


      strcpy(szFullPath, li.szFileName);
      li.handle = open(li.szFileName,O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
   }
   else
   {
       if (filename[1] != ':')
       {
          getcwd(szFullPath,CCHMAXPATH);
          strcat(szFullPath,"\\");
       }
       strcat(szFullPath,filename);

       if (!strchr(szFullPath,'.'))
          strcat(szFullPath,".JSP");

       strcpy(li.szFileName,szFullPath);
       li.handle = open(li.szFileName,O_RDONLY | O_BINARY, S_IREAD | S_IWRITE);
   }

   if (li.handle == -1 )
   {
      szFullPath[0]=0;
      WinLoadString(hab, (HMODULE)0, IDS_CANNOTOPENFILE,MESSAGELEN,(PSZ)szBuf);
      WinLoadString(hab, (HMODULE)0, IDS_FILEOPENERROR,100,(PSZ)szError);
      WinMessageBox(HWND_DESKTOP,hwndClient,(PSZ)szBuf,(PSZ)szError, 0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE | MB_ICONEXCLAMATION);

      UpdateTitleText(hwndMFrame);
      return;
   }
   read(li.handle,pFileBegin,strlen(FILESIGNE2));

   /*
   ** Since pFileBegin points to the buf[50]...
   */
   pwi->lFileFormat = FILE_DRAWIT30; /* By default....*/

   if (!strncmp(buf,FILESIGNE,strlen(FILESIGNE)))
      pwi->lFileFormat  = FILE_DRAWIT24;
   else if (!strncmp(buf,FILESIGNE2,strlen(FILESIGNE2)))
      pwi->lFileFormat  = FILE_DRAWIT25;
   else if (!strncmp(buf,FILESIGNE3,strlen(FILESIGNE3)))
      pwi->lFileFormat  = FILE_DRAWIT28;
   else if (!strncmp(buf,FILESIGNE4,strlen(FILESIGNE4)))
      pwi->lFileFormat  = FILE_DRAWIT29;
   else if (!strncmp(buf,FILESIGNE5,strlen(FILESIGNE5)))
      pwi->lFileFormat  = FILE_DRAWIT30;
   else if (!strncmp(buf,FILESIGNE6,strlen(FILESIGNE6)))
      pwi->lFileFormat  = FILE_DRAWIT32;
   else
   {
      /*
      ** It is not the old format or the new one, so notify
      ** user and quit.
      */
      szFullPath[0]=0;
      WinLoadString(hab, (HMODULE)0, IDS_WRONGFORMAT,MESSAGELEN,(PSZ)szBuf);
      WinLoadString(hab, (HMODULE)0, IDS_NOTADRAWITFILE,100,(PSZ)szError);
      WinMessageBox(HWND_DESKTOP,hwndClient,(PSZ)szBuf,(PSZ)szError,0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE | MB_ICONEXCLAMATION);
      UpdateTitleText(hwndMFrame);
      close( li.handle);
      return;
   }

   UpdateTitleText(hwndMFrame);
   /*
   ** Restore view before loading any file....
   */
   PercentageZoom(pwi,100);

   if (pwi->lFileFormat == FILE_DRAWIT24)
      return;
   else
   {
      LoadDrwFile(&li,pwi);
   }
}
/*------------------------------------------------------------------------*/

void WriteDrwfile(WINDOWINFO *pwi)
{
   static Loadinfo  li;

   li.dlgflags = FDS_SAVEAS_DIALOG;

   if (szFullPath[0])
      strcpy(li.szFileName, szFullPath);

   strcpy(li.szExtension,DRWEXTENSION);

   if (!FileGetName(&li,pwi))
      return;

   strcpy(szFullPath,li.szFileName);
   li.handle = open(li.szFileName,O_WRONLY | O_BINARY | O_CREAT,S_IREAD | S_IWRITE);
   _write(li.handle,(PVOID)FILESIGNE6,(ULONG)strlen(FILESIGNE6));
   ObjPutFile(&li,(POBJECT)0,pwi);
   pwi->bFileHasChanged = FALSE;
   close( li.handle);
   UpdateTitleText(hwndMFrame);
}
/*------------------------------------------------------------------------*/
/*                                                                        */
/*  Name       : FileSave()                                               */
/*                                                                        */
/*  Description: Processes the File menu's Save item                      */
/*                                                                        */
/*  Concepts:  Called whenever SAVE from the FILE menu is selected        */
/*                                                                        */
/*             Routine opens the file for output, calls the               */
/*             application's save routine, and closes the file.           */
/*                                                                        */
/*  Return     :  [none]                                                  */
/*                                                                        */
/*------------------------------------------------------------------------*/
VOID FileSave(WINDOWINFO *pwi)
{
   Loadinfo  li;
   char szBuf[MESSAGELEN];
   char szTitle[100];
   /*
   ** If there is no name jet go via the SaveAs way....
   */
   if(szFullPath[0] == '\0')
   {
      WriteDrwfile(pwi);
      return;
   }
   li.handle = open(szFullPath,O_WRONLY | O_BINARY | O_CREAT,S_IREAD | S_IWRITE);
   if (li.handle == -1 )
   {
      szFullPath[0]=0;
      WinLoadString(hab, (HMODULE)0, IDS_CANNOTOPENFILE,MESSAGELEN,(PSZ)szBuf);
      WinLoadString(hab, (HMODULE)0, IDS_FILEOPENERROR,100,(PSZ)szTitle);
      WinMessageBox(HWND_DESKTOP,hwndClient,(PSZ)szBuf,(PSZ)szTitle,0,
                    MB_OK | MB_APPLMODAL | MB_MOVEABLE | MB_ICONEXCLAMATION);

      UpdateTitleText(hwndMFrame);
      return;
   }
   _write(li.handle,(PVOID)FILESIGNE6,(ULONG)strlen(FILESIGNE6));
   ObjPutFile(&li,(POBJECT)0,pwi);
   pwi->bFileHasChanged = FALSE;
   close( li.handle);
}   /* End of FileSave   */
/*-------------------------------------------------------------------*/
VOID UpdateTitleText(HWND hwnd)
{
   CHAR szBuf[MAXNAMEL+TITLESEPARATORLEN+CCHMAXPATH];
   CHAR szSeparator[TITLESEPARATORLEN+1];

   PSZ pszT;

   WinLoadString(hab, (HMODULE)0, IDS_APPNAME, MAXNAMEL,(PSZ)szBuf);
   WinLoadString(hab,
                 (HMODULE)0,
                 IDS_TITLEBARSEPARATOR,
                 4,
                 (PSZ)szSeparator);

   strcat(szBuf, szSeparator);
   if(szFullPath[0] == '\0')
      pszT ="UNTITLED";
   else
      pszT = szFullPath;

   strcat(szBuf, pszT);

   WinSetWindowText(hwnd,szBuf);
}   /* End of UpdateTitleText   */
/*------------------------------------------------------------------------*/
/*  Name: delTitle                                                        */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
void delTitle(HWND hwnd, WINDOWINFO *pwi)
{
   szFullPath[0] = 0;
   UpdateTitleText(hwnd);
}
