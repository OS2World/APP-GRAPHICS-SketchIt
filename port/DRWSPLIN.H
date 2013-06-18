/*------------------------------------------------------------------------*/
/* FILE : drwspline.h                                                     */
/*                                                                        */
/*------------------------------------------------------------------------*/

typedef struct _spl {
                       OBJECT obj;
                       USHORT ustype;      /* type of object          */
                       Line   SplLine;     /* Used line def in square */
                       USHORT nrpts;       /* Number of points used   */
                       ULONG  lPattern;    /* Filling pattern         */
                       GRADIENT SplGrad;   /* gradient Filling        */
                       ULONG  fColor;      /* Filling color.          */
                       USHORT uslayer;     /* layer it belongs to     */
                       BOOL   LinDeleted;
                       BOOL   LinMultiSelected;
                       ULONG  ulState;
                       PPOINTLF pptl;       /* Corner coordinates      */
                       POINTLF ptlfCenter;  /* Rotation center.        */
                       } *pSpl24,Spl24;

typedef struct _Spl27  {
                       OBJECT obj;
                       USHORT ustype;      /* type of object          */
                       Line   line;        /* Used line def in square */
                       USHORT nrpts;       /* Number of points used   */
                       ULONG  lPattern;    /* Filling pattern         */
                       GRADIENT gradient;  /* gradient Filling        */
                       ULONG  fColor;      /* Filling color.          */
                       USHORT uslayer;     /* layer it belongs to     */
                       BOOL   LinDeleted;
                       BOOL   LinMultiSelected;
                       ULONG  ulState;
                       PPOINTLF pptl;       /* Corner coordinates      */
                       POINTLF ptlfCenter;  /* Rotation center.        */
                       FOUNTAIN fountain;
                       } *pSpl27,SPL27;


typedef struct _Spl29  {
                       OBJECT obj;
                       USHORT ustype;      /* type of object          */
                       Line   line;        /* Used line def in square */
                       USHORT nrpts;       /* Number of points used   */
                       ULONG  lPattern;    /* Filling pattern         */
                       GRADIENT gradient;  /* gradient Filling        */
                       ULONG  fColor;      /* Filling color.          */
                       USHORT uslayer;     /* layer it belongs to     */
                       BOOL   LinDeleted;
                       BOOL   LinMultiSelected;
                       ULONG  ulState;
                       PPOINTLF pptl;       /* Corner coordinates      */
                       POINTLF ptlfCenter;  /* Rotation center.        */
                       FOUNTAIN fountain;
                       ARROW  Arrow;
                       } *pSpl29,SPL29;


typedef struct _Spl31 {
                       OBJECT   obj;
                       USHORT   ustype;      /* type of object          */
                       BATTR31  bt;          /* Base attributes.        */
                       USHORT nrpts;       /* Number of points used   */
                       BOOL   LinDeleted;
                       BOOL   LinMultiSelected;
                       ULONG  ulState;
                       PPOINTLF pptl;       /* Corner coordinates      */
} *pSpl31,SPL31;


typedef struct _Spline {
                       OBJECT   obj;
                       USHORT   ustype;      /* type of object          */
                       BATTR    bt;          /* Base attributes.        */
                       USHORT nrpts;       /* Number of points used   */
                       BOOL   LinDeleted;
                       BOOL   LinMultiSelected;
                       ULONG  ulState;
                       PPOINTLF pptl;       /* Corner coordinates      */
} *pSpline,Spline,*PSPLINE,SPLINE;

#define SPL_CLOSED 0x0001
#define SPL_FILLET 0x0002
#define SPL_SPLINE 0x0004
/*
** creation
*/
MRESULT CreateSpline(WINDOWINFO *pwi, POINTL *ptl,ULONG ulMsg);
void SplineSetup(ULONG ultype);
void DelLastPoint(WINDOWINFO *pwi,USHORT usAction);


pSpline CloseSpline(WINDOWINFO *pwi);
void SplineOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc);
void SplineInvArea(POBJECT pSpl, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc);
VOID * SplineSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void DrawSplineSegment(HPS hps,WINDOWINFO *pwi,POBJECT pObj,RECTL *prcl);
VOID SplMoveSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
pSpline OpenSpline(WINDOWINFO *pwi);
void SplinGetBoxCorners(pSpline pSpl, PPOINTL ptlStart,USHORT cnr, WINDOWINFO *pwi);
void ScaleFactor(pSpline pSpl,LONG dx, LONG dy, float *y, float *x, WINDOWINFO *pwi);
POINTL CenterPoint(POINTL ptl1, POINTL ptl2);
void ScaleSpline(pSpline pSpl, POINTL ptl1,float xfactor, float yfactor, WINDOWINFO *pwi);
void SplShowSecondSel(pSpline pSpl,WINDOWINFO *pwi);
int FilePutSpline(pLoadinfo pli, pSpline pSpl );
int FileGetSpline(pLoadinfo pli, pSpline pSpl);
VOID SplineCopy(pSpline pCopy, pSpline pOrg,ULONG ObjectSize);
MRESULT EXPENTRY PolyLineDlgProc(HWND,USHORT,MPARAM,MPARAM);
VOID SplinMoveOutLine (POBJECT pObj,WINDOWINFO *pwi, SHORT dx, SHORT dy);
BOOL SplinHitHandle(POBJECT pObj,WINDOWINFO *pwi, POINTL ptlMouse);
VOID SplinShowHandles(POBJECT pObj,WINDOWINFO *pwi);
MRESULT RegularPolyCreate(ULONG,POINTL,WINDOWINFO *,ULONG);
void StartupPolyDlg(WINDOWINFO *pwi);
void SplRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void SplPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
/*
** Form change functions
*/
void SplStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg);
BOOL SplinDragHandle(POBJECT,WINDOWINFO *,POINTL,ULONG,MPARAM);
BOOL SplinDelHandle(POBJECT);
void splDrawOutline(POBJECT pObj,WINDOWINFO *pwi,BOOL b);
BOOL splineMoveHandle(POBJECT pObj, WINDOWINFO *pwi, long ldx,long ldy);
/*
** Rotation functions.
*/
BOOL SplPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptl);
BOOL SplGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL SplSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
void SplDrawRotate(WINDOWINFO *,POBJECT, double,ULONG,POINTL *);
BOOL SplNewPattern(WINDOWINFO *pwi, POBJECT pObj, ULONG ulPattern,BOOL bd);
BOOL bSplinHasPattern(POBJECT pObj);
void splineSetLineEnd(POBJECT pObj, WINDOWINFO *pwi);

void * splMakeClipPath(POBJECT,WINDOWINFO *,RECTL,ULONG *,ULONG *);
ULONG splinePattern (POBJECT pObj);
ULONG splineLineType(POBJECT pObj);
ULONG splineLineJoin(POBJECT pObj);
ULONG splineLineWidth(POBJECT pObj);

POBJECT closeSqrSegment(POBJECT pObj, POINTL ptlEnd, WINDOWINFO *pwi);
POBJECT openSqrSegment(POINTL ptl, WINDOWINFO *pwi);
POBJECT copySpline(POBJECT pOrg);
