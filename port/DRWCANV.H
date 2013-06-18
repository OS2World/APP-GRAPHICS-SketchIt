/*------------------------------------------------------------------------*/
/*  Name: drwcanv.h                                                       */
/*                                                                        */
/*  Description : Contains the datatypes for the canvaswindow.            */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/


typedef struct _canvas{
                  HWND   hwndVscroll;        /* Vertical scrollbar   */
                  HWND   hwndHscroll;        /* HoriZontal scrollbar */
                  SHORT  sVscrollMax;        /* Scrollrange. vert    */
                  SHORT  sHscrollMax;        /* Scrollrange. horz    */
                  SHORT  sVscrollPos;        /* Scrollpos.           */
                  SHORT  sHscrollPos;        /* Scrollpos.           */
                  USHORT cyCanvas;
                  USHORT cxCanvas;
                  int    iZoom;
                  HWND   hwndClient;
} CANVAS,*PCANVAS;

/*
** forminfo contains the info which is stored with the drawing in
** the file. It is stored at the very begin of the file.
*/
typedef struct _forminfo{
                 int     cb;        /* sizeof this structure            */
                 USHORT  cxForm;    /* sizeof the form in 0.1 mm        */
                 USHORT  cyForm;    /* sizeof the form in 0.1 mm        */
}FORMINFO, *PFORMINFO;

/*
** Public function prototypes
*/
void RegCanv(HAB hab, WINDOWINFO *pwi);
void CanvScrollBar(WINDOWINFO *pwi, BOOL bSetForm);
void CanvZoom(WINDOWINFO *pwi);
void PercentageZoom(WINDOWINFO *pwi, int iZoom);
HWND CreateCanvas(HWND hParent, HWND *h );
void Snap2Grid(WINDOWINFO *pwi, PPOINTL pptl);
void PaperSetSize(WINDOWINFO *pwi,USHORT usWidth, USHORT usHeight);
void CalcPtrPosition(WINDOWINFO *pwi, PPOINTL ptl);
int  fileGetForm(pLoadinfo pli, WINDOWINFO *pwi,BOOL bDrawing);
void fileWrtForm(pLoadinfo pli, WINDOWINFO *pwi);
void checkScrollbars(WINDOWINFO *pwi, POINTL ptl);

