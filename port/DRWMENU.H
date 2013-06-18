/*-----------------------------------------------------------------------*/
/*  Name: DRWMENU.H                                                      */
/*                                                                       */
/*  Description : Menu processing.                                       */
/*-----------------------------------------------------------------------*/
VOID InitMenu(WINDOWINFO *pwi,MPARAM mp1, MPARAM mp2, BOOL fHelpEnabled, void *pObj);
VOID EnableMenuItem( HWND hwndMenu, SHORT sIditem, BOOL bEnable);
void SetTextMenu(HWND hwndMenu,BOOL bEnable);
void SetImgMenu(HWND hwndMenu, BOOL bEnable);
void SetButtonState(HWND hwnd , USHORT ButtonId, BOOL bEnable);
MRESULT DrawMenuItem(MPARAM mp2);
MRESULT MeasureItem(MPARAM mp2);
MRESULT MenuFillPattern(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi);
MRESULT MenuLineSize(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi);
MRESULT MenuLineType(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi);
MRESULT MenuLineJoin(USHORT MenuId,HWND hwndMenu, WINDOWINFO *pwi);
MRESULT ShowPopupMenu(WINDOWINFO *pwi,HWND hOwner,ULONG msg,POINTL ptl);
void Menuhelptext(HWND hwnd, HAB hab, MPARAM mp1);
MRESULT MenuZoom(WINDOWINFO *pwi,HWND hMenu,USHORT usMenuId, BOOL bMenu);
BOOL handleMenuCommand(WINDOWINFO *,HWND,MPARAM);
MRESULT measureColorItem( MPARAM mp2);
MRESULT drawColorItem ( MPARAM mp2);

