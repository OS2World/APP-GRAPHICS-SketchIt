/*------------------------------------------------------------------------*/
/*           1ÚÄÄÄÄÄÄÄ¿2                                                  */
/*            ³       ³                                                   */
/*           4ÀÄÄÄÄÄÄÄÙ3                                                  */
/*--ch--------date--------------description-------------------------------*/
/*  1       170598    Added the m_bClose.                                 */
/*  3       301098    Added the arrow structure for version 2.9           */
/*------------------------------------------------------------------------*/

typedef struct _cir {
                       OBJECT obj;
                       USHORT ustype;        /* type of object */
                       ULONG  Multiplier;
                       ULONG  StartAngle;
                       ULONG  SweepAngle;
                       Line   CirLine;       /* Used line def in circle */
                       ULONG  fColor;        /* filling color           */
                       USHORT Rotate;
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlPosn;      /* Center coordinates      */
                       ULONG  lPattern;      /* Filling pattern         */
                       GRADIENT CirGrad;     /* gradient Filling        */
                       ARCFPARAMS arcpParms;
                       POINTLF ptlfCenter;  /* Rotation center.        */
                       } *pCir24,Cir24;


typedef struct _Cir27
{                      OBJECT   obj;
                       USHORT   ustype;        /* type of object */
                       ULONG    Multiplier;
                       ULONG    StartAngle;
                       ULONG    SweepAngle;
                       Line     line;          /* Used line def in circle */
                       ULONG    fColor;        /* filling color           */
                       USHORT   Rotate;
                       USHORT   uslayer;       /* layer it belongs to     */
                       POINTLF  ptlPosn;      /* Center coordinates      */
                       ULONG    lPattern;      /* Filling pattern         */
                       GRADIENT gradient;     /* gradient Filling        */
                       ARCFPARAMS arcpParms;
                       POINTLF  ptlfCenter;  /* Rotation center.        */
                       FOUNTAIN fountain;
                       } *PCIR27,CIR27;

typedef struct _Cir28  {
                       OBJECT obj;
                       USHORT ustype;        /* type of object */
//@ch2                       ULONG  Multiplier;
                       BOOL   m_bClose;
                       ULONG  StartAngle;
                       ULONG  SweepAngle;
                       Line   line;          /* Used line def in circle */
                       ULONG  fColor;        /* filling color           */
                       USHORT Rotate;
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlPosn;      /* Center coordinates      */
                       ULONG  lPattern;      /* Filling pattern         */
                       GRADIENT gradient;    /* gradient Filling        */
                       ARCFPARAMS arcpParms; /* Floating point version..*/
                       POINTLF ptlfCenter;   /* Rotation center.        */
                       FOUNTAIN fountain;
                       }  *PCIR28,CIR28;

typedef struct _Cir29  {
                       OBJECT obj;
                       USHORT ustype;        /* type of object */
//@ch2                       ULONG  Multiplier;
                       BOOL   m_bClose;
                       ULONG  StartAngle;
                       ULONG  SweepAngle;
                       Line   line;          /* Used line def in circle */
                       ULONG  fColor;        /* filling color           */
                       USHORT Rotate;
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlPosn;      /* Center coordinates      */
                       ULONG  lPattern;      /* Filling pattern         */
                       GRADIENT gradient;    /* gradient Filling        */
                       ARCFPARAMS arcpParms; /* Floating point version..*/
                       POINTLF ptlfCenter;   /* Rotation center.        */
                       FOUNTAIN fountain;
                       ARROW   Arrow;
                       }  *PCIR29,CIR29;

typedef struct _Cir31 {
                       OBJECT obj;
                       USHORT ustype;        /* type of object */
                       BATTR31    bt;        /* Base attributes*/
                       BOOL       m_bClose;
                       ULONG      StartAngle;
                       ULONG      SweepAngle;
                       USHORT     Rotate;
                       POINTLF    ptlPosn;      /* Center coordinates      */
                       ARCFPARAMS arcpParms; /* Floating point version..*/
                       } *PCIR31,CIR31;


typedef struct _Circle {
                       OBJECT obj;
                       USHORT ustype;        /* type of object */
                       BATTR  bt;            /* Base attributes*/
                       BOOL       m_bClose;
                       ULONG      StartAngle;
                       ULONG      SweepAngle;
                       USHORT     Rotate;
                       POINTLF    ptlPosn;      /* Center coordinates      */
                       ARCFPARAMS arcpParms; /* Floating point version..*/
                       } *pCircle,Circle,CIRCLE,*PCIRCLE;

pCircle OpenCircleSegment(WINDOWINFO *pwi);
void DrawCircleSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj,RECTL *rcl);
void MoveCirSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
void CircleInvArea(POBJECT, RECTL *,WINDOWINFO *, BOOL);
extern pCircle pCircleBase,pCirStart;
void CircleOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc);
POBJECT CloseCircleSegment(pCircle pCir,POINTL ptl,POINTL ptlEnd, WINDOWINFO *pwi);
VOID * CircleSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void CirScale(POBJECT pObj,float dx, float dy, POINTL *ptlsh,WINDOWINFO *pwi);
VOID CirMoveOutLine (POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy);
VOID CirShowHandles(POBJECT pObj, WINDOWINFO *pwi);
BOOL CirDragHandle(POBJECT,WINDOWINFO *,POINTL,ULONG,MPARAM);
void CirPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void CirRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void CirStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg);

BOOL CirGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL CirPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse);
BOOL CirSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
void CirDrawRotate(WINDOWINFO *,POBJECT,double,ULONG,POINTL *);
BOOL CirNewPattern(WINDOWINFO *pwi, POBJECT pObj, ULONG ulPattern, BOOL bd);
BOOL cirCheck(POBJECT pObj);
void cirDrawOutline(POBJECT pObj, WINDOWINFO *pwi,BOOL bIgnoreLayer);
BOOL bCirHasPattern(POBJECT pObj);

MRESULT circleCommand(USHORT usCommand);
VOID createCircle(WINDOWINFO *pwi,POINTL ptMouse,ULONG ulMsg, USHORT sShift);
void circleDetail( HWND hOwner, WINDOWINFO *pwi);
void registerCircle(HAB hab);
void circleSetLineEnd(POBJECT pObj, WINDOWINFO *pwi);
ULONG cirPattern  (POBJECT pObj);
ULONG cirLineType (POBJECT pObj);
ULONG cirLineJoin (POBJECT pObj);
ULONG cirLineWidth(POBJECT pObj);
void cirGetRealCenter(WINDOWINFO *pwi,POBJECT pObj, POINTL *ptlCenter);
void cirRestoreCenter(WINDOWINFO *pwi,POBJECT pObj);



