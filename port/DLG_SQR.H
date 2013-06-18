/*------------------------------------------------------------------------*/
/* FILE : dlg_sql.h                                                       */
/*                                                                        */
/* Contains: Variable declaration & Function Prototypes for dlg_sqr.c     */
/*                                                                        */
/*           1ÚÄÄÄÄÄÄÄ¿2                                                  */
/*            ³       ³                                                   */
/*           4ÀÄÄÄÄÄÄÄÙ3                                                  */
/*------------------------------------------------------------------------*/

typedef struct _sqr {
                       OBJECT obj;
                       USHORT ustype;        /* type of object          */
                       Line   SqrLine;       /* Used line def in square */
                       POINTLF ptlf[4];      /* Corner coordinates      */
                       POINTLF ptlSqrCenter; /* Center for rotation     */
                       ULONG  lPattern;      /* Filling pattern         */
                       ULONG  fColor;        /* Filling color.          */
                       GRADIENT SqrGrad;     /* gradient Filling        */
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlfCenter;   /* Rotation center.        */
                       } *pSqr24,Sqr24;

typedef struct _sqr29 {
                       OBJECT obj;
                       USHORT ustype;        /* type of object          */
                       Line   line;          /* Used line def in square */
                       POINTLF ptlf[4];      /* Corner coordinates      */
                       POINTLF ptlSqrCenter; /* Center for rotation     */
                       ULONG  lPattern;      /* Filling pattern         */
                       ULONG  fColor;        /* Filling color.          */
                       GRADIENT gradient;    /* gradient Filling        */
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlfCenter;   /* Rotation center.        */
                       FOUNTAIN fountain;    /* fountain fill           */
                       } *pSqr29,SQR29;

typedef struct _Square {
                       OBJECT obj;
                       USHORT ustype;        /* type of object          */
                       BATTR  bt;
                       POINTLF ptlf[4];      /* Corner coordinates      */
                       POINTLF ptlSqrCenter; /* Center for rotation     */
                       } *pSquare,Square,*PSQUARE,SQUARE;

/*prototypes*/

POBJECT OpenSquareSegment(POINTL ptlStart, WINDOWINFO *pwi);
POBJECT CloseSquareSegment(POBJECT pObj,POINTL ptlEnd, WINDOWINFO *pwi);
void DrawSquareSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj, RECTL *rcl);
VOID * SquarSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void SquareInvArea(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc);
void SquareOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi);
void MoveSqrSegment(pSquare pSqr, SHORT dx, SHORT dy, WINDOWINFO *pwi);
void RotateSqrSegment(double Deg, POINTL ptlCenter, POINTL *ptls, USHORT points );
void SqrScale(POBJECT pObj,float dx, float dy, POINTL *ptlsh,WINDOWINFO *pwi);
void SqrMoveOutLine(pSquare pSqr,WINDOWINFO *pwi, SHORT dx, SHORT dy);
void SqrRotation(POBJECT pObj,WINDOWINFO *pwi, SHORT sDirection);
void SqrPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void SqrRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void SqrStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg);
BOOL SqrGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL SqrSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL SqrPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse);
void SqrDrawRotate(WINDOWINFO *,POBJECT,double,ULONG,POINTL *);
void sqrDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL b);
BOOL bSqrHasPattern(POBJECT pObj);
void sqrRestoreCenter(WINDOWINFO *pwi,POBJECT pObj);
void sqrGetRealCenter(WINDOWINFO *pwi,POBJECT pObj, POINTL *pptl);
