#define DRAG_START 0x100
#define DRAG_MOVE  0x102

extern MRESULT EXPENTRY ObjectDlgProc(HWND, ULONG, MPARAM, MPARAM );
extern MRESULT EXPENTRY ObjectWndProc(HWND, ULONG, MPARAM, MPARAM );
VOID DragBox(HPS hps, POINTL *pptlStart, POINTL *pptlEnd, SHORT dx, SHORT dy, USHORT mode);
VOID SelectBox(HPS hps, POINTL *pptlStart, POINTL *pptlEnd, SHORT dx, SHORT dy, USHORT mode);
void ConvPts2Rect(POINTL ptlStart, POINTL ptlEnd, RECTL *rcl);
int mapSelTypeOpmode(int iClass);
MRESULT changeMode(WINDOWINFO *pwi,USHORT usMode, ULONG ulExtra);
BOOL isSingleSelection(WINDOWINFO *pwi);
