typedef struct _Triangle {
                       OBJECT obj;
                       USHORT  ustype;     /* type of object          */
                       Line   TriLine;     /* Used line def in square */
                       POINTLF ptlf[3];    /* Corner coordinates      */
                       POINTL ptlTriCenter;/* Center of rotation      */
                       ULONG  lPattern;    /* Filling pattern         */
                       GRADIENT TriGrad;   /* gradient Filling        */
                       ULONG  fColor;      /* filling color           */
                       USHORT Rotate;
                       USHORT uslayer;     /* layer it belongs to     */
                       BOOL   TriDeleted;
                       } *pTriangle,Triangle;

pTriangle OpenTriangleSegment(POINTL ptlStart, WINDOWINFO *pwi);
ULONG FileGetTri(pLoadinfo pli);
ULONG FilePutTri(pLoadinfo pli);
void DrawTriangleSegment(HPS hps, WINDOWINFO *pwi, pTriangle ps, RECTL *prcl);
void MoveTriSegment(pTriangle pTri, SHORT dx, SHORT dy);
void TriangleInvArea(pTriangle pTri, RECTL *rcl,WINDOWINFO *pwi);
VOID CreateTriangle(HPS hps, POINTL *pptlStart, POINTL *pptlEnd);
void TriangleOutLine(pTriangle pTri, RECTL *rcl,WINDOWINFO *pwi);
pTriangle CloseTriangleSegment(pTriangle pTri,POINTL ptlEnd, WINDOWINFO *pwi);
VOID * TriangleSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj);
void TriStretch(pTriangle pTri, RECTL *rclNew, WINDOWINFO *pwi);
void TriScale(POBJECT pObj,float dx, float dy, POINTL *ptlsh,WINDOWINFO *pwi);
void TriMoveOutLine(pTriangle pTri ,WINDOWINFO *pwi,SHORT dx, SHORT dy);