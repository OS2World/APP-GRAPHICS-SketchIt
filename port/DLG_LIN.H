/*------------------------------------------------------------------------*/
/* FILE : dlg_lin.h                                                       */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define MAX_ARROWSIZE 100  /* 100 * 0.1 mm == 1   cm    */
#define MIN_ARROWSIZE  30  /*  20 * 0.1 mm == 0.1 cm    */
#define DEF_ARROWSIZE  40  /*  20 * 0.1 mm default size */
#define DEF_LINEEND     1  /* Default line end          */
#define DEF_LINESTART   1  /* Default line start        */

typedef struct _lns27 {
                       OBJECT obj;
                       USHORT  ustype;     /* type of object          */
                       Line    line;       /* Used line def in square */
                       POINTLF ptl1,ptl2;  /* Corner coordinates      */
                       USHORT uslayer;     /* layer it belongs to     */
                       POINTLF ptlfCenter; /* Rotation center.        */
                       } *plns27,LNS27;


typedef struct _lns29  {
                       OBJECT obj;
                       USHORT  ustype;     /* type of object          */
                       Line    line;       /* Used line def in square */
                       POINTLF ptl1,ptl2;  /* Corner coordinates      */
                       USHORT  uslayer;    /* layer it belongs to     */
                       POINTLF ptlfCenter; /* Rotation center.        */
                       ARROW   Arrow;
                       } *pLns29,LNS29;

typedef struct _Lin31 {
                       OBJECT  obj;
                       USHORT  ustype;     /* type of object          */
                       BATTR31 bt;         /* Base attributes         */
                       POINTLF ptl1,ptl2;  /* Corner coordinates      */
                       } *pLns31,LNS31;

typedef struct _Lines {
                       OBJECT  obj;
                       USHORT  ustype;     /* type of object          */
                       BATTR   bt;         /* Base attributes         */
                       POINTLF ptl1,ptl2;  /* Corner coordinates      */
                       } *pLines,Lines,*PLINE;

/*prototypes*/
pLines CloseLineSegment(pLines pLin, POINTL ptlEnd, WINDOWINFO *pwi);
pLines OpenLineSegment(POINTL ptlStart, WINDOWINFO *pwi);
void DrawLineSegment(HPS hps, WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl);
void DeleteLinSegment( pLines pLin);
pLines CopyLineSegment(pLines pLin);
void DeleteAllLines(void);
VOID * LineSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void LineInvArea(POBJECT pLin, RECTL *rcl,WINDOWINFO *pwi, BOOL);
extern ULONG FilePutLin(pLoadinfo pli);
extern ULONG FileGetLin(pLoadinfo pli);
/*pointer to the chain of squares*/
BOOL IsOnLineEnd(POINTL ptl, pLines pL,WINDOWINFO *pwi, USHORT *cnr);
VOID LineMoving(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy);
void MoveLinSegment(POBJECT pObj, SHORT, SHORT,WINDOWINFO *);
void DeleteLineSegment( pLines pLin);
void LinRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void LinPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
BOOL LinGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL LinSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
void LinDrawRotate(WINDOWINFO *,POBJECT,double,ULONG,POINTL *);
VOID DrawLine(WINDOWINFO *pwi,POINTL ptlSt,POINTL ptlE, short mode, POBJECT pObj);
void linDrawOutline(POBJECT, WINDOWINFO *,BOOL);
void LineOutline(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi);
void lineSetLineEnd(POBJECT pObj,WINDOWINFO *pwi);

