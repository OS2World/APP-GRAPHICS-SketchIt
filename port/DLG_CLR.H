extern MRESULT EXPENTRY TrueClrDlgProc(HWND,USHORT,MPARAM,MPARAM);
extern MRESULT EXPENTRY RedGreenBlueWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 );
extern MRESULT EXPENTRY GradientWndProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 );
extern MRESULT EXPENTRY GradientDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 );
extern MRESULT EXPENTRY ShadingDlgProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2 );
void setupShading( WINDOWINFO *pwi);
void GradientFill(WINDOWINFO *pwi, HPS hps, RECTL *rcl,GRADIENT *pGrad);


MRESULT ColorPopupMenu(HWND hwnd,HWND hMenu, POINTL ptl);
void setShadingOffset( WINDOWINFO *,ULONG, ULONG, POINTL *, ULONG);

