#define TXT_DROPSHADOW 0x0001
#define TXT_CIRCULAR   0x0002
#define TXT_SHADOW     0x0004
#define TXT_OUTLINE    0x0008

/*
** Circular struct contains all the nesecarry bits for
** circular text.
*/
typedef struct _CircularStruct
              {
                 ULONG    ulStart;
                 ULONG    ulSweep;
                 ULONG    ulDirection;
                 RECTLF   rclf; /* Circular txt box */
                 BOOL     bMirror;
               }*PCIRCULAR,CIRCULAR;


typedef struct _txtstruct
	   {
	     OBJECT  obj;
             USHORT  ustype;      /* type of object */
	     char Str[120];       /*String element */
	     POINTLF ptl;         /*String place   */
	     LONG    lShadeX;     /* nr of Units in x dir    */
	     LONG    lShadeY;     /* nr of Units in y dir    */
             USHORT  uslayer;     /* layer it belongs to     */
             ULONG   lPattern;    /* filling pattern of text */
             GRADIENT TxtGrad;    /* gradient Filling        */
    	     char    facename[50];
	     ULONG   TxtColor;    /*Text Color     */
	     ULONG   ShadeColor;  /*shade color    */
	     ULONG   TxtOutlineColor;
	     ULONG   TxtBackGroundColor;
	     USHORT  TxtShear;    /*Char shear     */
	     USHORT  ShadeShear;  /*Shadow shear   */
	     ULONG   ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
	     CIRCULAR TxtCircular; /*see above definition */
             SIZEFF  sizfx;
             LONG    LineWidth;   /* Outline width    */
             POINTLF ptlfCenter;  /* Rotation point   */
             GRADIENTL  gradl;    /* Rotation vector. */
             float   fRotate;     /* pi rad rotated.  */
	   }*ptxt24,Text24;

/*
** New text structure for version later than 2.4
*/
typedef struct _Txt27
   {
             OBJECT   obj;
             USHORT  ustype;      /* type of object */
             USHORT  usSize;      /* structure size */
	     char Str[120];       /*String element  */
	     POINTLF ptl;         /*String place    */
	     LONG    lShadeX;     /* nr of Units in x dir    */
	     LONG    lShadeY;     /* nr of Units in y dir    */
             USHORT  uslayer;     /* layer it belongs to     */
             ULONG   lPattern;    /* filling pattern of text */
             GRADIENT gradient;   /* gradient Filling        */
    	     char    facename[50];/* used font.              */
	     ULONG   TxtColor;    /* Text Color     */
	     ULONG   ShadeColor;  /* shade color    */
	     ULONG   TxtOutlineColor;
	     ULONG   TxtBackGroundColor;
	     USHORT  TxtShear;    /*Char shear     */
	     USHORT  ShadeShear;  /*Shadow shear   */
	     ULONG   ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
	     CIRCULAR TxtCircular; /*see above definition */
             SIZEFF  sizfx;
             LONG    LineWidth;   /* Outline width    */
             POINTLF ptlfCenter;  /* Rotation point   */
             GRADIENTL  gradl;    /* Rotation vector. */
             float   fRotate;     /* pi rad rotated.  */
             FOUNTAIN fountain;   /* fountain fill    */
}*PTXT27,TXT27;


typedef struct _Txt29
   {
     OBJECT  obj;
     USHORT  ustype;       /* type of object */
     USHORT  usSize;       /* structure size */
     char Str[120];        /*String element  */
     POINTLF  ptl;         /*String place    */
     LONG     lShadeX;     /* nr of Units in x dir    */
     LONG     lShadeY;     /* nr of Units in y dir    */
     USHORT   uslayer;     /* layer it belongs to     */
     ULONG    lPattern;    /* filling pattern of text */
     GRADIENT gradient;    /* gradient Filling        */
     ULONG    TxtColor;    /* Text Color     */
     ULONG    ShadeColor;  /* shade color    */
     ULONG    TxtOutlineColor;
     ULONG    TxtBackGroundColor;
     USHORT   TxtShear;    /*Char shear     */
     USHORT   ShadeShear;  /*Shadow shear   */
     ULONG    ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
     CIRCULAR TxtCircular; /*see above definition */
     SIZEFF   sizfx;
     LONG     LineWidth;   /* Outline width    */
     POINTLF  ptlfCenter;  /* Rotation point   */
     GRADIENTL gradl;      /* Rotation vector. */
     float     fRotate;    /* pi rad rotated.  */
     FOUNTAIN  fountain;   /* fountain fill    */
     FATTRS    fattrs;
}*PTXT29,TXT29;


typedef struct _Txt31
   {
     OBJECT  obj;
     USHORT  ustype;       /* type of object */
     BATTR31 bt;
     USHORT  usSize;       /* structure size */
     char Str[120];        /*String element  */
     POINTLF  ptl;         /*String place    */
     LONG     lShadeX;     /* nr of Units in x dir    */
     LONG     lShadeY;     /* nr of Units in y dir    */
     ULONG    TxtOutlineColor;
     ULONG    TxtBackGroundColor;
     USHORT   TxtShear;    /*Char shear     */
     USHORT   ShadeShear;  /*Shadow shear   */
     ULONG    ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
     CIRCULAR TxtCircular; /*see above definition */
     SIZEFF   sizfx;
     LONG     LineWidth;   /* Outline width    */
     GRADIENTL gradl;      /* Rotation vector. */
     float     fRotate;    /* pi rad rotated.  */
     FATTRS    fattrs;
}*PTXT31,TXT31;

typedef struct _Textstruct
   {
     OBJECT  obj;
     USHORT  ustype;       /* type of object */
     BATTR   bt;
     USHORT  usSize;       /* structure size */
     char Str[120];        /*String element  */
     POINTLF  ptl;         /*String place    */
     LONG     lShadeX;     /* nr of Units in x dir    */
     LONG     lShadeY;     /* nr of Units in y dir    */
     ULONG    TxtOutlineColor;
     ULONG    TxtBackGroundColor;
     USHORT   TxtShear;    /*Char shear     */
     USHORT   ShadeShear;  /*Shadow shear   */
     ULONG    ulState;     /*TXT_DROPSHADOW | TXT_CIRCULAR etc */
     CIRCULAR TxtCircular; /*see above definition */
     SIZEFF   sizfx;
     LONG     LineWidth;   /* Outline width    */
     GRADIENTL gradl;      /* Rotation vector. */
     float     fRotate;    /* pi rad rotated.  */
     FATTRS    fattrs;
}*Textstruct,Text,*PTEXT,TEXT;

extern void * TextSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void DrawTextSegment(HPS hps, WINDOWINFO *pwi, POBJECT pObj,RECTL *prcl);
void MoveTextSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
Textstruct OpenTextSegment(POINTL ptlstart,WINDOWINFO *pwi);      /* dlg_txt.c*/
MRESULT EXPENTRY TextDlgProc  (HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2 );

void TextInvArea(POBJECT pTxt, RECTL * prcl, WINDOWINFO *pwi, BOOL bInc);
void TextOutLine(POBJECT,RECTL * ,WINDOWINFO *,BOOL);
void TxtFlipHoriz(POBJECT pObj, WINDOWINFO *pwi);
void TxtFlipVert(POBJECT pObj, WINDOWINFO *pwi);
BOOL TxtNewPattern(WINDOWINFO *pwi, POBJECT pObj, ULONG ulPattern, BOOL bd);

Textstruct CloseTextSegment(Textstruct pText,char *InpString, WINDOWINFO *pwi);

VOID TxtMoveOutLine (POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy);
void TxtRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void TxtPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
BOOL TxtGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL TxtSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
void TextStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg);
void TxtDrawRotate(WINDOWINFO *, POBJECT, double,ULONG, POINTL *);
BOOL TxtPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse);
BOOL isOutlineSet(POBJECT pObj);
void setTxtOutline(POBJECT pObj, BOOL bAction);
void RegisterTxt(HAB hab);
ULONG textPattern(POBJECT pObj);

/*
** Variables
*/
extern ULONG DefValclrtab[];
extern ULONG DlgColorTab[];
/*
** Function prototypes for DRWTXT.H
*/
MRESULT EXPENTRY TxtCirWndProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY TxtCirDlgProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY ChangeTextDlgProc(HWND,ULONG,MPARAM,MPARAM);
VOID DisplayRotate (HPS,WINDOWINFO *,Textstruct, BOOL);
void txtDrawOutline(POBJECT pObj, WINDOWINFO *pw,BOOL bIgnorelayer);
BOOL txtSetFont(POBJECT pObj, WINDOWINFO *pwi);
void showTxtFont(POBJECT pObj,WINDOWINFO *pwi); /* Set font name in status line */
void setFontSquare(POBJECT pObj, WINDOWINFO *pwi);
void textDetail( HWND hOwner, WINDOWINFO *pwi);
ULONG textLineWidth(POBJECT pObj);
BOOL isTxtOutlineSet(POBJECT pObj);
BOOL txtEdit(POBJECT, WINDOWINFO *);
void drawCharString(WINDOWINFO *,LONG, char *,LONG *,FIXED,int );

