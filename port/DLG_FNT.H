BOOL FontDlg(WINDOWINFO *,PFATTRS,BOOL bObj);
void setFont(WINDOWINFO *,PFATTRS,SIZEF );
void FontInit(HPS hps, PSZ facename, USHORT reason, SIZEF * pSizefx);
LONG getFontSetID(HPS hps);
BOOL getFontDlgSizfx( SIZEF *pSizef);
/*
** global vars
**/
extern char   FontFaceName[]; /*Contains the current active font name */
extern FATTRS  defaultfattr;
extern FIXED   deffxpointsize;
