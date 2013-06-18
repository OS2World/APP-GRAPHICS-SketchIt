extern LONG PageFeed (void);
extern int PrintMode(void);
extern BOOL GetPrQueue(void);
extern BOOL PrintOpen(USHORT);
extern void ClosePrinter ( void );


#ifdef __IBMC__
#define DRW_CALLING _System
#else
#define DRW_CALLING
#endif

extern void DRW_CALLING PrintWrite(WINDOWINFO *pwi);
extern VOID ConvertPrinterFontSize(FIXED, PFATTRS);
extern BOOL CreatePrinterBmpHdcHps( PHDC phdc, PHPS phps);
MRESULT EXPENTRY PrintPrevDlgProc( HWND, USHORT, MPARAM, MPARAM);
