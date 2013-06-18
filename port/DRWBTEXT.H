/*--------------------------------------------------------------------------*/
/*                                                                          */
/* File : drwbtext.h                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#define MAX_COLUMNS 20

#define ALIGN_LEFT       1
#define ALIGN_RIGHT      2
#define ALIGN_CENTER     3
#define ALIGN_JUST       4

#define SPACE_NONE       1
#define SPACE_SINGLE     2
#define SPACE_HALF       3
#define SPACE_DOUBLE     4


#define BEGIN_BLOCKTEXT  "@BLOCKTEXT@"    
#define EOF_BLOCKTEXT    "@/BLOCKTEXT@"

typedef struct _block31
{
   OBJECT obj;
   USHORT ustype;        /* type of object   CLS_BLOCKTEXT              */
   BATTR31  bt;            /* Base attributes         */
   USHORT usColumns;     /* Number of columns available.                */
   USHORT usPage;        /* PageNumber.                                 */
   USHORT nAlign;        /* Text alignment in column.                   */
   USHORT nSpace;        /* Spacing.                                    */
   ULONG  ulColColor;    /* Column color. In most cases transparent.    */
   FATTRS fattrs;        /* Font attribute structure.                   */
   SIZEFF sizfx;         /* Size of the fontbox floating point normalized*/
   RECTLF rclf[MAX_COLUMNS]; /* Column rectangles in a normalized rectangles*/
   ULONG  lTextlen;      /* Number of chars in textblock including \0   */
   char   *pszText;      /* Text to display                             */
} BLOCK31, *PBLOCK31;


typedef struct _blocktext
{
   OBJECT obj;
   USHORT ustype;        /* type of object   CLS_BLOCKTEXT              */
   BATTR  bt;            /* Base attributes         */
   USHORT usColumns;     /* Number of columns available.                */
   USHORT usPage;        /* PageNumber.                                 */
   USHORT nAlign;        /* Text alignment in column.                   */
   USHORT nSpace;        /* Spacing.                                    */
   ULONG  ulColColor;    /* Column color. In most cases transparent.    */
   FATTRS fattrs;        /* Font attribute structure.                   */
   SIZEFF sizfx;         /* Size of the fontbox floating point normalized*/
   RECTLF rclf[MAX_COLUMNS]; /* Column rectangles in a normalized rectangles*/
   ULONG  lTextlen;      /* Number of chars in textblock including \0   */
   char   *pszText;      /* Text to display                             */
} blocktext, *PBLOCKTEXT, BLOCKTEXT;
/*----------------------------------------------------------------------
** The following structure is used for printing. It is very hard in OS/2
** to get real WYSIWYG on the printer.
** So that's why this struct.
-----------------------------------------------------------------------*/
typedef struct _btextprint
{
   POBJECT  pObj;         /* Needed to get access to the char width table */
   POINTLF  ptlf;         /* Normalized position  */
   char    *pszString;    /* A single line of text*/
   FIXED    fxBreakExtra; /* Extra spaceing when justified allignment is set */
   int      iBreakCount;
   struct _btextprint *next;  /* Points to the next print block in chain*/
} BTEXTPRINT, *PBTEXTPRINT;

void free_BlockText(POBJECT pObj);
blocktext * newBlockText(POINTL ptlStart, WINDOWINFO *pwi);
void BlockTextMoveOutLine(POBJECT pObj,WINDOWINFO *pwi, SHORT dx, SHORT dy);
void drawBlockText(HPS hps,WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl);
void BlockTextInvArea(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi, BOOL bInc);
void BlockTextOutLine(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi);
VOID * BlockTextSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj);
POBJECT copyBlockTextObject( POBJECT pObj);
void BlockTextStretch(POBJECT pObj,PRECTL prclNew,WINDOWINFO *pwi,ULONG ulMsg);
void createBlockText(WINDOWINFO *pwi);
void BlockTextDetail( HWND hOwner, WINDOWINFO *pwi);
void MoveBlockText(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
void BlockTextFaceName(POBJECT pObj, char *pszFontname);
BOOL BlockTextSetColor(POBJECT pObj, ULONG lColor);
void createBlockText(WINDOWINFO *pwi);
BOOL BlockTextEdit(POBJECT, WINDOWINFO *);
BOOL BlockTextFontDlg(POBJECT pObj, WINDOWINFO *pwi);
int FileGetBlockText(pLoadinfo pli, POBJECT pObj);
int FilePutBlockText(pLoadinfo pli, POBJECT pObj);

ULONG BlockTextGetAlign(POBJECT pObj);
void  BlockTextSetAlign(POBJECT pObj,USHORT usAlign);

ULONG BlockTextGetSpacing(POBJECT pObj);
void  BlockTextSetSpacing(POBJECT pObj,USHORT usSpace);

BOOL BlockTextSelectable(POBJECT pObj, WINDOWINFO *pwi);
void BlockTextPrepPrint(POBJECT pObj, WINDOWINFO *pwi);
void showBlockTextFont(POBJECT  pObj, WINDOWINFO *pwi);
void BlockTextDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer);

void setColumnColor(POBJECT pObj, ULONG ulColor);
