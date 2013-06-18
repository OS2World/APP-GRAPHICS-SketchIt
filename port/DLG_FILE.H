
BOOL FileGetName(pLoadinfo pli, WINDOWINFO *pwi );
void ReadDrwfile(char *,WINDOWINFO *);
void WriteDrwfile(WINDOWINFO *pwi);
BOOL FileNew(HWND hwnd,WINDOWINFO *pwi);
VOID FileSave(WINDOWINFO *pwi);
VOID UpdateTitleText(HWND hwnd);
VOID delTitle(HWND,WINDOWINFO *pwi);

/*
** drfrm24.c
*/
BOOL ReadOldFile(pLoadinfo pli,WINDOWINFO *pwi);


/*
** Just for the demoversion of drawit.
*/
BOOL demoMessage(WINDOWINFO *pwi);

