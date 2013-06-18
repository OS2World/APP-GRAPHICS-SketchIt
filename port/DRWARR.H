/*------------------------------------------------------------------------*/
/*  Name: drwarr.h                                                        */
/*                                                                        */
/*  Description : Functions for handling arrow on lines ends.             */
/*------------------------------------------------------------------------*/
#define MAX_ARROWSIZE 100  /* 100 * 0.1 mm == 1   cm    */
#define MIN_ARROWSIZE  30  /*  20 * 0.1 mm == 0.1 cm    */
#define DEF_ARROWSIZE  40  /*  20 * 0.1 mm default size */
#define DEF_LINEEND     1  /* Default line end          */
#define DEF_LINESTART   1  /* Default line start        */

void arrowDetail(HWND,WINDOWINFO *);
void RegisterArrow(HAB);
void drwEndPt    (WINDOWINFO *,ARROW,POINTL,POINTL);
void drwStartPt  (WINDOWINFO *,ARROW,POINTL,POINTL);
void drwEndPoints(WINDOWINFO *,ARROW,POINTL,POINTL);
void drwStartPtAtAngle(WINDOWINFO *pwi,ARROW Arrow,LONG lAngle,POINTL ptl2);
void drwEndPtAtAngle(WINDOWINFO *pwi,ARROW Arrow,LONG lAngle,POINTL ptl2);
void arrowAreaExtra(ARROW Arrow, RECTL *prcl );
long angleFromPoints( const POINTL ptl1, const POINTL ptl2 );
