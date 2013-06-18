/*------------------------------------------------------------------------*/
/*  drwtypes.h                                                            */
/*                                                                        */
/*  Description : Contains all types used in the drawing package.         */
/*                These are mostly floating point versions of already     */
/*                existing ones in the toolkit.                           */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define SHADE_NONE          0
#define SHADE_LEFTTOP       1
#define SHADE_LEFTBOTTOM    2
#define SHADE_RIGHTBOTTOM   3
#define SHADE_RIGHTTOP      4
#define SHADE_CHANGE       10

/* variation of SIZEF. We use floats. */
typedef struct _SIZEFF
{
   float fcx;
   float fcy;
} SIZEFF,*PSIZEFF;

/* floating point variation on rectl..*/
typedef struct _RECTLF          /* rcl */
{
   float  xLeft;
   float  yBottom;
   float  xRight;
   float  yTop;
} RECTLF, *PRECTLF;

typedef struct _groupsize
{
   USHORT width;
   USHORT heigth;
}GROUPSIZE,*PGROUPSIZE;

/*
** Line definitions
*/
typedef struct _LineStruct {
                        ULONG LineColor;
                        FIXED LineWidth;
                        ULONG LineType;
                        ULONG LineJoin;
                        ULONG LineEnd;
                }*pLine,Line;

/*
** floating point version of the normal arcparams.
*/
typedef struct _ARCFPARAMS       /* arcp */
{
   float lP;
   float lQ;
   float lR;
   float lS;
} ARCFPARAMS,*PARCFPARAMS;

/*
** floating point version of the normal pointl.
*/
typedef struct _POINTLF         /* ptl  */
{
   float x;
   float y;
} POINTLF,*PPOINTLF;


/*
** Gradient struct contains all the nesecarry bits for gradient filling
** of figure's.
*/
typedef struct _GradientStruct {
                 ULONG ulStart;
                 ULONG ulSweep;
                 ULONG ulSaturation;
                 ULONG ulDirection;
                 BOOL  bMirror;
                }*PGRADIENT,GRADIENT;
/*
** Fountain fill structure.
*/
typedef struct _fountain
{
   ULONG ulStartColor;
   ULONG ulEndColor;
   LONG  lHorzOffset;
   LONG  lVertOffset;
   ULONG ulFountType;
   LONG  lAngle;
} FOUNTAIN, *PFOUNTAIN;


typedef struct _arrow
{
    long lStart;
    long lEnd;
    long lSize;
} ARROW;
/*
** Floating point version of arrow...
*/
typedef struct _farrow
{
    long  lStart;
    long  lEnd;
    float fWidth;
    float fHeight;
} FARROW;
/*
** Shading....
*/
typedef struct _shade
{
   ULONG lShadeType;    /* NONE,TOPLEFT,BOTTOMLEFT,BOTTOMRIGHT,TOPRIGHT */
   ULONG lShadeColor;   /* Shading color.                               */
   ULONG lUnits;        /* Number of units. SHADOWMIN, SHADOWMAX etc see dlg.h */
} SHADE, *PSHADE;

typedef struct _baseattributes
{
   Line     line;        /* Used line def in square */
   ULONG    lPattern;    /* Filling pattern         */
   GRADIENT gradient;    /* gradient Filling        */
   FOUNTAIN fountain;    /* Fountain Filling        */
   ULONG    fColor;      /* Filling color.          */
   ULONG    ShadeColor;  /* shade color  NOT USED!! */
   USHORT   usLayer;     /* layer it belongs to     */
   ARROW    arrow;       /* Arrows for this object  */
   POINTLF  ptlfCenter;  /* Rotation center.        */
   ULONG    ulPage;
   SHADE    Shade;       /* Shading definitions.    */
} BATTR,*PBATTR;
/*
** Baseattributes as they are used in the older versions of drawit.
** defined here to be able to load the older drawit files.
*/
typedef struct _baseattr31
{
   Line     line;        /* Used line def in square */
   ULONG    lPattern;    /* Filling pattern         */
   GRADIENT gradient;    /* gradient Filling        */
   FOUNTAIN fountain;    /* Fountain Filling        */
   ULONG    fColor;      /* Filling color.          */
   ULONG    ShadeColor;  /* shade color  NOT USED!! */
   USHORT   usLayer;     /* layer it belongs to     */
   ARROW    arrow;       /* Arrows for this object  */
   POINTLF  ptlfCenter;  /* Rotation center.        */
   ULONG    ulPage;
} BATTR31,*PBATTR31;
