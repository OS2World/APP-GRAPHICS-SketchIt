/*------------------------------------------------------------------------*/
/* FILE : drwgrp.h                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
typedef struct _Group {
                       OBJECT obj;
                       USHORT ustype;      /* type of object CLS_GROUP*/
                       RECTLF rclf;        /* Our outline             */
                       USHORT Rotate;      /* Is the whole group rotated?*/
                       BOOL   GrpMultiSelected;
                       USHORT uslayer;     /* layer it belongs to     */
                       POINTLF ptlfCenter; /* Rotation center.        */
                       } *pGroup,Group;

typedef struct _GroupEnd{
                       OBJECT obj;
                       USHORT ustype;      /* type of object CLS_GROUPEND*/
                       } *pGroupEnd,GroupEnd;


void GroupOutLine(pGroup pGrp, RECTL *prcl, WINDOWINFO *pwi);
void GroupMoveOutline(POBJECT pObj, WINDOWINFO *pwi,SHORT dx,SHORT dy);
void GroupInvArea(POBJECT pGrp, RECTL *rcl, WINDOWINFO *pwi, BOOL bInc);
VOID * GroupSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj);
void GroupMove(POBJECT pObj,SHORT dx, SHORT dy, WINDOWINFO *pwi);
void GroupDraw(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl, BOOL bSel);
void GrpDraw(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl);
POBJECT GroupCreate(POBJECT pBegin, WINDOWINFO *pwi);
void GroupReconstruct(POBJECT pChain);
void GroupDelete(POBJECT pObj);
void GroupCopy(POBJECT pObjOrg, POBJECT pBase, ULONG ulMode);
void GroupUnGroup(POBJECT pObj, POBJECT pBase, POBJECT pEnd, WINDOWINFO *pwi);
void GrpStretch(POBJECT pObj,RECTL *rcl,WINDOWINFO *pwi);

void GroupCalcOutline(POBJECT pObj, WINDOWINFO *pwi);

BOOL GrpGetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
void GrpSetCenterRel(WINDOWINFO *,POBJECT,float,float);

BOOL GrpSetCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL *ptl);
BOOL GrpPtrAboveCenter(WINDOWINFO *pwi,POBJECT pObj,POINTL ptlMouse);
void GrpDrawRotate(WINDOWINFO *, POBJECT, double,ULONG,POINTL *);
void grpDrawOutline(POBJECT ,WINDOWINFO *,BOOL);

#define GRP_APPEND  1
#define GRP_ADD     2

POBJECT copyGroup(POBJECT pOrg);

void    setGroupLayer( POBJECT pObj, USHORT usLayer );
USHORT  getGroupLayer( POBJECT pObj );
BOOL    canPackGroup(POBJECT pObj);



