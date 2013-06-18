/*------------------------------------------------------------------------*/
/*  Name: drwtrian.c                                                      */
/*                                                                        */
/*  Description : Triangle functions.                                     */
/*                                                                        */
/*  Functions  :                                                          */
/*  TriScale      : Scale a given triangle by a given factor.             */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwutl.h"
#include "dlg_clr.h"
#include "drwtrian.h"

/*------------------------------------------------------------------------*/
pTriangle OpenTriangleSegment(POINTL ptlStart, WINDOWINFO *pwi)
{
   return (pTriangle)0;
}
/*------------------------------------------------------------------------*/
pTriangle CloseTriangleSegment(pTriangle pTri,POINTL ptlEnd, WINDOWINFO *pwi)
{
   return (pTriangle)0;
}
/*-------------------------------------------------------------------------*/
void TriMoveOutLine(pTriangle pTri ,WINDOWINFO *pwi,SHORT dx, SHORT dy)
{
}
/*-------------------------------------------------------------------------*/
void DrawTriangleSegment(HPS hps, WINDOWINFO *pwi, pTriangle pTri, RECTL *prcl)
{
}
/*------------------------------------------------------------------------*/
/* Move a Triangle segment.                                               */
/*------------------------------------------------------------------------*/
void MoveTriSegment(pTriangle pTri, SHORT dx, SHORT dy)
{
}
/*------------------------------------------------------------------------*/
VOID * TriangleSelect(POINTL ptl,WINDOWINFO *pwi,POBJECT pObj)
{
   return (void *)0;
}
/*------------------------------------------------------------------------*/
void TriangleOutLine(pTriangle pTri, RECTL *rcl,WINDOWINFO *pwi)
{
}
/*------------------------------------------------------------------------*/
void TriangleInvArea(pTriangle pTri, RECTL *rcl,WINDOWINFO *pwi)
{
}
/*-------------------------------------------------------------------------*/
/* Draws the triangle when at the time we draw it with the mouse.          */
/*-------------------------------------------------------------------------*/
VOID CreateTriangle(HPS hps, POINTL *pptlStart, POINTL *pptlEnd)
{
}
/*------------------------------------------------------------------------*/
/*  TriScale.                                                             */
/*                                                                        */
/*  Description : Scale the trian  with a given factor. Used when a group */
/*                is stretched.                                           */
/*                                                                        */
/*  Parameters : PTRI  - pointer to a triangle element.                   */
/*               WINDOWINFO * - pointer to our window structure.          */
/*               float dx - scaling factor in x direction.                */
/*               float dy - scaling factor in y direction.                */
/*               POINTL ptlshift - center shift.                          */
/*                                                                        */
/*------------------------------------------------------------------------*/
void TriScale(POBJECT pObj,float dx, float dy, POINTL *ptlsh,WINDOWINFO *pwi)
{
}
/*-----------------------------------------------------------------------*/
void TriStretch(pTriangle pTri, RECTL *rclNew, WINDOWINFO *pwi)
{
}
