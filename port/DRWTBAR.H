/*---------------------------------------------------------------------------*/
/*  Name: drwtbar.h                                                          */
/*                                                                           */
/*  Description : Toolbar definitions.                                       */
/*                                                                           */
/*                                                                           */
/*-sc---date-----------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define CYBUTTON      26
#define CXBUTTON      26

BOOL createToolBar(HWND hOwner, HWND hParent );
BOOL destroyToolBar(void);
void posToolBar(long x, long y);
BOOL registerToolBar(HAB hab);
long getToolBarHeight( void );
void commandToolBar(long lCommand);

