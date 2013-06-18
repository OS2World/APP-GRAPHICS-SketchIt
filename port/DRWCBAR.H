/*---------------------------------------------------------------------------*/
/*  Name: drwcbar.h                                                          */
/*                                                                           */
/*  Description : Implements the colorbar at the bottom of the main window.  */
/*                                                                           */
/*---------------------------------------------------------------------------*/
BOOL RegisterColorBar(HAB hab);
HWND cbar(HWND hParent);
MRESULT setCurBarColor(HWND hBar,ULONG ulNewColor);
