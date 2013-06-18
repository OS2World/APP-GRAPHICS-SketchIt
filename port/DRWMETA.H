typedef struct _Metaimg {
                       OBJECT obj;
                       USHORT ustype;        /* type of object          */
                       USHORT uslayer;       /* layer it belongs to     */
                       RECTLF rclf;          /* Our object area.        */
                       CHAR   szMetaFileName[CCHMAXPATH];
                       HMF    hmf;           /* metafile handle.        */
                       LONG   cBytes;        /* metafile length         */
                       PBYTE  pbBuffer;      /* metafile data buffer    */
                       } *pMetaimg,Metaimg;

BOOL ScaleMetaFile(HPS, HMF, RECTL *);
HMF GetMetaFile(pMetaimg pMeta,char *filename,WINDOWINFO *pwi);
POBJECT OpenMetaSegment(POINTL ptl,char *filename,WINDOWINFO *pwi, HMF hmf);
VOID DrawMetaSegment(HPS hps,WINDOWINFO *pwi,POBJECT pObj, RECTL *prcl);
BOOL RecordMetaFile(HAB hab,WINDOWINFO *pwi,RECTL *prcl);
void MetaPictOutLine(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi);
void MetaPictInvArea(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi, BOOL bInc);
VOID * MetaPictSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj);
void MetaPictMove(POBJECT pObj, SHORT dx, SHORT dy, WINDOWINFO *pwi);
void MetaStretch(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi,ULONG ulMsg);
BOOL FilePutMetaData(pLoadinfo pli, POBJECT pObj);
BOOL FileGetMetaData(pLoadinfo pli, POBJECT pObj);
void MetaDetail(HWND hwnd,POBJECT pObj );
void SetMetaSizes(pMetaimg pMeta, WINDOWINFO *pwi);
POBJECT copyMetaFileObject( POBJECT pObj);
void MetaMoveOutline(POBJECT pObj, WINDOWINFO *pwi, SHORT dx, SHORT dy);
void setMetaPosition(WINDOWINFO *pwi,POBJECT pObj, LONG x, LONG y);
void saveMetaPicture(POBJECT pObj,WINDOWINFO *pwi);
POBJECT newMetaObject(void);