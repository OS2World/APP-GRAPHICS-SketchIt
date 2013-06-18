void RotateSegment(USHORT Deg, POINTL ptlCenter, POINTL *ptls, USHORT points );
void ObjOutLine(POBJECT   pObject, RECTL *rcl,WINDOWINFO *pwi,BOOL bCalc);

void DrawMovePointHandle(HPS hps, LONG x, LONG y);
VOID DrawGrid(WINDOWINFO *pwi, RECTL *rcl);
void MoveObject(POBJECT,LONG,LONG, WINDOWINFO *); /* cursor keys..*/

BOOL ObjShowMovePoints(POBJECT pObj, WINDOWINFO *pwi);
BOOL ObjDragHandle(POBJECT,WINDOWINFO *,POINTL,ULONG,MPARAM);
BOOL ObjDelHandle(POBJECT pObj);

void ObjRotation(POBJECT pObj,WINDOWINFO *pwi, SHORT sDirection);
MRESULT EXPENTRY RotSnapDlgProc(HWND,USHORT,MPARAM,MPARAM);
void StartupSnapRotDlg(WINDOWINFO *pwi,BOOL bStartup);

void ObjInterChange(WINDOWINFO *pwi);

void ObjNotify(void);
POBJECT pObjCreate(WINDOWINFO *pwi, USHORT usClass);
POBJECT pObjNew(WINDOWINFO *pwi, USHORT usClass);
BOOL pObjAppend(POBJECT pObj);
BOOL AppendChain(POBJECT pObj);

void ObjDrawSegment(HPS hps, WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl);
void ObjDelete(POBJECT pObj);
void ObjExtract(POBJECT pObj);
void ObjDeleteAll(void);
POBJECT ObjCurrent(void);
POBJECT ObjectCopy(POBJECT pObjOrg);
POBJECT makeCopy(POBJECT pOrg);

void ObjInvArea(POBJECT pObject,RECTL *rcl,WINDOWINFO *pwi,BOOL bInc);
void ObjSetInvArea(WINDOWINFO *pwi);

void ObjRefresh(POBJECT pObj, WINDOWINFO *pwi);
void ObjMove(POBJECT pObj,SHORT dx,SHORT dy, WINDOWINFO *pwi);
void ObjMoveMultiOutline(WINDOWINFO *pwi,SHORT dx, SHORT dy);

BOOL ObjMultiSelect(POINTL ptlSt, POINTL ptlEnd, WINDOWINFO *pwi);
BOOL ObjMultiDelete(POBJECT pObj);
void ObjMultiUnSelect(void);
void ObjMultiClrChange(ULONG ulNewColor);

void ObjMultiLntypeChange(ULONG ulLntype);
void ObjMultiLnWidthChange(ULONG ulLnWidth);

void ObjLnJoinChange(ULONG ulLnJoin,POBJECT pObj);
void ObjMltLnJoinChange(ULONG ulLnJointype);
/*
** Font handling.
*/
void ObjMultiFontChange(char * FontName);
void ObjFontChange(char * FontName, POBJECT pObj);
BOOL ObjectFontDlg(POBJECT pObj, WINDOWINFO *pwi); /* Start font dialog */
/*
** Drawing outline of objects
*/
void ObjMultiDrawOutline(WINDOWINFO *pwi,POBJECT pObj,BOOL b);
void ObjDrawOutline(WINDOWINFO *pwi,POBJECT pObj);
/*
** Rop codes
*/
void ObjChangeRopCode(WINDOWINFO *pwi,POBJECT pObj,LONG lRop);
/*
** Color functions
*/
void ObjSetOutLineClr(ULONG ulNewColor, POBJECT pObj);
void ObjSetMltOutLineClr(ULONG ulNewColor);
BOOL ObjSetFillClr(ULONG ulNewColor, POBJECT pObj);
void ObjSetMltFillClr(ULONG ulNewColor);

BOOL ObjDropFillClr(WINDOWINFO *pwi, ULONG ulNewColor, POBJECT pObj);

void ObjLntypeChange(ULONG ulLntype,POBJECT pObj);
void ObjLnWidthChange(ULONG ulLnWidth, POBJECT pObj);

POBJECT ObjSelect(POINTL ptl,WINDOWINFO *pwi);
BOOL    ObjSelectAll(WINDOWINFO *pwi);

void ObjMultiMoveCopy(WINDOWINFO *pwi, LONG dx, LONG dy, USHORT op_mode);
void ObjPutFile(pLoadinfo pli,POBJECT pChain,WINDOWINFO *pwi);
void ObjGetFile(pLoadinfo pli,WINDOWINFO *pwi);
BOOL ObjIsOnCorner(WINDOWINFO *pwi,POBJECT pObj,POINTL ptl,RECTL *rcl, USHORT *cnr);
void ObjHorzAlign(WINDOWINFO *pwi);
void ObjAlnHorzB(WINDOWINFO *pwi);
void ObjAlnHorzT(WINDOWINFO *pwi);
void ObjVertAlign(WINDOWINFO *pwi);
void ObjAlnVertR(WINDOWINFO *pwi);
void ObjAlnVertL(WINDOWINFO *pwi);
void DrawHandle(HPS hps, LONG x, LONG y);
BOOL ObjGetBoxCorners(POBJECT pObj,PPOINTL ptl,USHORT cnr, WINDOWINFO *pwi);
BOOL ObjSecondSelect(POBJECT pObj,WINDOWINFO *pwi);
BOOL ObjStretch(POBJECT,PPOINTL,PPOINTL,WINDOWINFO *,ULONG);
BOOL ObjQuerySelect(void);
void ObjDrawSelected(POBJECT pObj,HPS hps,WINDOWINFO *pwi,RECTL *prcl,BOOL bMove);
void ObjDrawMetaSelected(POBJECT pObj, HPS hps,WINDOWINFO *pwi);


BOOL ObjPatternChange(ULONG, POBJECT, WINDOWINFO *,BOOL);
void ObjMultiPatternChange(ULONG,WINDOWINFO *);

POBJECT ObjGroup(WINDOWINFO *pwi);
void ObjUnGroup(POBJECT pObj, WINDOWINFO *pwi);
POBJECT ObjRemFromGroup(POBJECT, PRECTL, WINDOWINFO *);

LONG ObjGetSize(USHORT usClass);
void ObjGrpStretch(POBJECT pObj,PPOINTL ptlStart,PPOINTL ptlEnd,WINDOWINFO *pwi);
void ObjMoveOutLine(POBJECT pObj,WINDOWINFO *pwi,SHORT dx, SHORT dy);
void ObjShiftSelect(POINTL ptl, WINDOWINFO *pwi);
void ObjBoundingRect(WINDOWINFO *pwi, RECTL *rcl, BOOL bInclusive);

MRESULT EXPENTRY GridSizeDlgProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT ObjDrawRotLine(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl,ULONG ulMsg);
MRESULT ObjMoveCenter(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl,ULONG ulMsg);
void    ObjRestoreCenter(WINDOWINFO *pwi, POBJECT pObj);

BOOL ObjDrawRotCenter(WINDOWINFO *pwi,POBJECT pObj, BOOL bInvert);
void ObjRefreshRotCenter(WINDOWINFO * pwi, POBJECT pObj);
BOOL ObjPtrAboveCenter(WINDOWINFO *pwi, POBJECT pObj,POINTL ptl);
void ObjDrawRotate(WINDOWINFO *,POBJECT,double,ULONG,POINTL *);
BOOL ObjectsAvail(void); /* is there any object to save???? */
/*
** Box rounding.
*/
BOOL ObjSetMltRound(WINDOWINFO *pwi, RECTL *prcl);
void ObjSetRounding(POBJECT pObj, WINDOWINFO *pwi);
/*
** Line ends.
*/
void ObjMultiLineEndChange(WINDOWINFO *pwi);
BOOL ObjLineEndChange(POBJECT pObj, WINDOWINFO *pwi);
/*
** Clippath handling.
*/
BOOL ObjMakeImgClipPath( WINDOWINFO *pwi );
BOOL ObjIsClipPathSelected( void );

/*
** Getting some attribs for the popupmenu check marks
*/
ULONG ObjGetFillType(WINDOWINFO *pwi, POBJECT pObj);  /* Filling pattern */
ULONG ObjGetLineType(WINDOWINFO *pwi, POBJECT pObj);  /* Linetype        */
ULONG ObjGetLineJoin(WINDOWINFO *pwi, POBJECT pObj);  /* LineJoin        */
ULONG ObjGetLineWidth(WINDOWINFO *pwi, POBJECT pObj); /* LineWidth       */
ULONG ObjGetAlignment(WINDOWINFO *pwi, POBJECT pObj); /* Block txt alignment */
ULONG ObjGetLineSpace(WINDOWINFO *pwi, POBJECT pObj); /* Block txt spacing   */
/*
** Selection handle stuff.
*/
void remSelectHandles  ( WINDOWINFO *pwi );
VOID showSelectHandles (POBJECT pObject,WINDOWINFO *pwi);

/*
** Text editing
*/
BOOL ObjEditText(POBJECT pObj, WINDOWINFO *pwi);

BOOL ObjChangeAlignment(WINDOWINFO *pwi,POBJECT pObj,USHORT usAlign);
BOOL ObjChangeSpacing(WINDOWINFO *pwi,POBJECT pObj,USHORT usSpace);


void ObjPreparePrinting(WINDOWINFO *pwi); /* Prepare printing */
/*
** Copy attributes of the selected object to the application context.
*/
MRESULT copyAttributes(WINDOWINFO *pwi, POBJECT pObj);
MRESULT multiPasteAttribs(WINDOWINFO *pwi);
MRESULT pasteAttributes(WINDOWINFO *pwi, POBJECT pObj);

void ObjMultiPaintHandles(WINDOWINFO *pwi);

/*
** Shading!!
*/
ULONG ObjGetShadeType(WINDOWINFO *pwi, POBJECT pObj);
BOOL ObjShadeChange(ULONG ulShade, POBJECT pObj, WINDOWINFO *pwi, BOOL bDialog);
void ObjMultiShadeChange(ULONG ulShade, WINDOWINFO *pwi);

BOOL ObjMoveHandle(POBJECT pObj, WINDOWINFO *pwi, long ldx,long ldy);
