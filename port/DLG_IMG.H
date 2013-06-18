/*------------------------------------------------------------------------*/
/* Image formats plus imagestruct definitions.                            */
/*------------------------------------------------------------------------*/

#define IFMT_UNKNOWN  0
#define IFMT_DIB      1
#define IFMT_X11      2
#define IFMT_META     3
#define IFMT_GIF      4
#define IFMT_ICO      5
#define IFMT_BMP2     6      // OS/2 2.0 Bitmaps, Icons & Pointers
#define IFMT_TIFF     7
#define IFMT_AUTO     9

#define  IMG_OK               0
#define  IMG_OUTOFMEMORY      1
#define  IMG_WRONGFORMAT      2
#define  IMG_NOTIMPLEMENTED   3

#define  SZIMG_START          "<IMAGE>"
#define  SZIMG_END            "<END>"

#if 0 
typedef struct _image
{
    OBJECT   obj;
    USHORT   ustype;
    USHORT   nColors;
    USHORT   uslayer;
    ULONG    lPattern;      /* Filling pattern         */
    BOOL     ImgCir;
    float    cx;
    float    cy;
    float    cxdest;
    float    cydest;
    float    x;
    float    y;
    ULONG    ImageDataSize;
    ULONG    filesize;
    PBITMAPINFOHEADER2 pbmp2;
    unsigned char * ImgData;
    POINTLF  ptf;
} Image25, *PIMG25;
#endif

typedef struct _img28
{
    OBJECT   obj;
    USHORT   ustype;
    USHORT   nColors;
    USHORT   uslayer;
    ULONG    lPattern;      /* Filling pattern         */
    BOOL     ImgCir;
    float    cx;
    float    cy;
    float    cxdest;
    float    cydest;
    float    x;
    float    y;
    ULONG    ImageDataSize;
    ULONG    filesize;
    PBITMAPINFOHEADER2 pbmp2;
    unsigned char * ImgData;
    POINTLF  ptf;
} Image28, *PIMG28;


typedef struct _image
{
    OBJECT   obj;
    USHORT   ustype;
    USHORT   nColors;
    USHORT   uslayer;
    ULONG    lPattern;      /* Filling pattern         */
    BOOL     ImgCir;
    float    cx;
    float    cy;
    float    cxdest;
    float    cydest;
    float    x;
    float    y;
    ULONG    ImageDataSize;
    ULONG    filesize;
    PBITMAPINFOHEADER2 pbmp2;
    unsigned char * ImgData;
    POINTLF  ptf;
    LONG     lRopCode;       /* Ropcode to be used during blitting.      */
    ULONG    nrpts;          /* Number of points of clipping path        */
    ULONG    ulClipPathType; /* Type of clippath SPL_FILLET | SPL_SPLINE */
    POINTLF  *pptl;          /* Points to the chain of points...         */
} Image, *pImage,*pDrwImage;






typedef struct _IMG
{
   unsigned char * pFileData;
   PBITMAPINFOHEADER2 pbmp2;
   unsigned char * ImgData;
   ULONG ImageDataSize;
   ULONG filesize;
   BYTE  status;            /* Error handling while reading the data */
} IMG,*PIMAGE;


void _System CreateImgSegment(WINDOWINFO *pwi);
POBJECT ImgSetup(WINDOWINFO *pwi, char *pszFilename);

pImage SetupImgSegment(pImage pImg,WINDOWINFO *pwi);

void free_imgobject ( POBJECT pObj);

BOOL ReadBitmap(pLoadinfo pli, PIMAGE pImg);
VOID DrawImgSegment(HPS hps,WINDOWINFO *pwi, POBJECT pObj, RECTL *prcl);
BOOL CreateBitmapHdcHps( PHDC phdc, PHPS phps);
BOOL IsOnImageCorner( LONG x, LONG y, pImage pImg,WINDOWINFO *pwi, USHORT *cnr);
VOID * ImageSelect(POINTL ptl,WINDOWINFO *pwi, POBJECT pObj);
void ImageCrop(POINTL ptl1, POINTL ptl2, pImage pImg, WINDOWINFO *pwi);
VOID CloseImgHdcHps(WINDOWINFO *pwi);
MRESULT EXPENTRY ImgDlgProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY Export2ImgDlgProc(HWND,ULONG,MPARAM,MPARAM);
MRESULT EXPENTRY SaveSelImgDlgProc(HWND,ULONG,MPARAM,MPARAM);
BOOL ImageCircle(HBITMAP hbm, HPS hps,pImage pImg , WINDOWINFO *pwi);
void DeleteImage(pImage pImg);
void DeleteAllImages(void);
void FlipImgHorz(POBJECT pObj);
void FlipImgVert(POBJECT pObj);
extern HPS     hpsBitmapFile;
extern HDC     hdcBitmapFile;
int FilePutImageData(pLoadinfo pli,pImage pImg);
int FileGetImageData(WINDOWINFO *pwi, pLoadinfo pli,pImage pImg);
void ImgInvArea(POBJECT pObj, RECTL *rcl, WINDOWINFO *pwi, BOOL bInc);
void ImgOutLine(POBJECT pObj, RECTL *rcl, WINDOWINFO *pwi);
BOOL ImgRotateColor(pImage pImg, SHORT usmode);
BOOL SaveImage(pImage pImg,WINDOWINFO *pwi);
BOOL imgGrayScale(POBJECT pObj);
BOOL imgBlackWhite(POBJECT pObj);
BOOL imgUndoPalette( POBJECT pObj);
void ImgShowPalette(HWND hwnd,pImage pImg);
BOOL ExportSel2Bmp(WINDOWINFO *,RECTL *,char *, USHORT);
void ImageStretch(POBJECT pObj,PRECTL prclnew,WINDOWINFO *pwi,ULONG ulMsg);
void ImgMoveSegment(POBJECT pObject, SHORT dx, SHORT dy,WINDOWINFO *pwi);
void ImgSetDropPosition(POBJECT pObject, SHORT x, SHORT y, WINDOWINFO *pwi);
BOOL ImgRotate90(POBJECT pObject);
BOOL ImgSaveOtherFormats(HWND hwndClient, POBJECT pObj, WINDOWINFO *pwi);
BOOL Export2Bmp(WINDOWINFO *pwi );
BOOL ImgRotate(WINDOWINFO *pwi,POBJECT pObject, int degrees);
BOOL ImgRestoreSize(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi);
BOOL ImgRestoreAspect(POBJECT pObj, RECTL *rcl,WINDOWINFO *pwi);
void ImgLoad(WINDOWINFO *pwi,char *pszFilename); /*load image from file */
void ImgSetCircular(POBJECT pObj, BOOL bCirc);
BOOL ImgIsCircular(POBJECT pObj);
void ImgMoveOutLine(POBJECT pObj,WINDOWINFO *pwi, SHORT dx, SHORT dy);
/*
** Clippath handling
*/
void imgAddClipPath(POBJECT,POBJECT,WINDOWINFO *);
/*
** ROP Codes?
*/
void imgChangeRopCode(POBJECT pObj,LONG lRop);
void imgSetMenuCheckMarks(HWND hwndMenu,POBJECT pObj);
/*
** drwimg2.c
*/
BOOL DataOnClipBoard(HAB hab);
void CopyData(HAB hab,WINDOWINFO *pwi,POBJECT pObj);
VOID PasteData(HAB hab, WINDOWINFO *pwi);
VOID CutBitmap(HAB hab,pDrwImage pImg, WINDOWINFO *pwi);
MRESULT DragOver(HAB hab, PDRAGINFO pDraginfo, PSZ pCurrentDir);
MRESULT Drop(HAB hab, PDRAGINFO pDraginfo,WINDOWINFO *pwi);
void pastePalette(WINDOWINFO *pwi);
void copyPalette(WINDOWINFO *pwi);
POBJECT copyImageObject(POBJECT pObj);
/*
** drwjpg.c
*/
BOOL readjpeg(WINDOWINFO *pwi,pImage pImg);
void writejpegdata(char *pszFile,unsigned char *pImgData,BITMAPINFOHEADER2 *pbmp2);

BOOL imgHasClipPath(POBJECT p);
BOOL imgDelClipPath(POBJECT p);

#ifdef DRWIMG

typedef int MOD_ERR;
#define	MOD_ERR_OK	((MOD_ERR) 0)
#define	MOD_ERR_MEM	((MOD_ERR) 1)
#define	MOD_ERR_OPEN	((MOD_ERR) 2)
#define	MOD_ERR_CREATE	((MOD_ERR) 3)
#define	MOD_ERR_SUPPORT	((MOD_ERR) 4)
#define	MOD_ERR_HDC	((MOD_ERR) 5)
#define	MOD_ERR_HPS	((MOD_ERR) 6)
#define	MOD_ERR_HBITMAP	((MOD_ERR) 7)
#define	MOD_ERR_HMF	((MOD_ERR) 8)
#define	MOD_ERR_CLIP	((MOD_ERR) 9)
#define	MOD_ERR_GBM(rc)	((MOD_ERR) 0x8000+(rc))

typedef struct
	{
	GBM gbm;
	GBMRGB gbmrgb[0x100];
	BYTE *pbData;
	} MOD;

int ModMakeHBITMAP(MOD *mod, POBJECT pObj);
int ModCreateFromFile(CHAR *szFn, CHAR *szOpt,MOD *modNew);

/*...sbpp palette and algorithm mappings:0:*/
#define	CVT_BW		0
#define	CVT_VGA		1
#define	CVT_8		2
#define	CVT_4G		3
#define	CVT_784		4
#define	CVT_666		5
#define	CVT_8G		6
#define	CVT_TRIPEL	7
#define	CVT_RGB		8
#define	CVT_FREQ	9
#define	CVT_MCUT	10

#define	CVT_NEAREST	0
#define	CVT_ERRDIFF	1
#define	CVT_HALFTONE	2
/*...e*/

#endif
