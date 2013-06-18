/*------------------------------------------------------------------------*/
/*  Name: drwbox.h                                                        */
/*                                                                        */
/*  Description : Box function prototypes.                                */
/*                                                                        */
/*------------------------------------------------------------------------*/

typedef struct _box {
                       OBJECT obj;
                       USHORT ustype;        /* type of object          */
                       Line   line;          /* Used line def in square */
                       RECTLF rclf;          /* Corner coordinates      */
                       LONG  lVRound;        /* Corner-rounding control */
                       LONG  lHRound;        /* Corner-rounding control */
                       LONG  lRotate;        /* box rotation            */
                       LONG  lShear;
                       ULONG  lPattern;      /* Filling pattern         */
                       ULONG  fColor;        /* Filling color.          */
                       GRADIENT gradient;    /* gradient Filling        */
                       FOUNTAIN fountain;    /* fountain fill.          */
                       USHORT uslayer;       /* layer it belongs to     */
                       POINTLF ptlfCenter;   /* Rotation center.        */
                       } *PBOX,BOX;

POBJECT boxCreate(WINDOWINFO *pwi, POINTL ptlStart);
void boxMoveOutLine(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy);
void boxDraw(HPS hps, WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl);
void boxDrawOutline(POBJECT pObj, WINDOWINFO *pwi, BOOL bIgnorelayer);
void boxOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc);
void boxInvArea(POBJECT pObj, RECTL *prcl,WINDOWINFO *pwi, BOOL bInc);
void boxPutInGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void boxRemFromGroup(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi);
void boxStretch(POBJECT pObj,RECTL *prcl,WINDOWINFO *pwi,ULONG ulMsg);
VOID * boxSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
BOOL boxGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL boxSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL boxPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse);
BOOL boxNewPattern(WINDOWINFO *pwi, POBJECT pObj, ULONG ulPattern,BOOL bDialog);
VOID boxMake(POBJECT pObj, WINDOWINFO *pwi, POINTL ptle, POINTL ptls, ULONG msg);
void boxMoveSegment(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
void boxDrawRotate(WINDOWINFO *pwi, POBJECT pObj, double lRotate,ULONG ulMsg,POINTL *pt);
void RegisterBox(HAB hab);
void boxDetails(WINDOWINFO *pwi,HWND hOwner,POBJECT pObj);
MRESULT boxRounding(WINDOWINFO *pwi,HWND hOwner);

