BOOL DrwTextInp ( HWND hwnd, MPARAM mp1, MPARAM mp2, WINDOWINFO *pwi);
void SetCursorStartPosition(HWND hwnd,WINDOWINFO *pwi,POINTL *ptl);
void DrawInputString(WINDOWINFO *pwi);
void UpdateInputString(WINDOWINFO *pwi, BOOL direct);
void Createcursor(WINDOWINFO *pwi);
void breakInputString( WINDOWINFO *pwi );

