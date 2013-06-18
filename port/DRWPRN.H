#ifdef __IBMC__
#define DRW_CALLING _System
#else
#define DRW_CALLING
#endif

BOOL PrintOpen(WINDOWINFO *);
void ClosePrinter(void);
void DRW_CALLING PrintWrite(WINDOWINFO *pwi);
MRESULT EXPENTRY PrintPrevDlgProc( HWND, USHORT, MPARAM, MPARAM);
