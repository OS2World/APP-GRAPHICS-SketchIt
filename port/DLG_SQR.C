/*------------------------------------------------------------------------*/
/*  Name: dlg_sqr.c                                                       */
/*                                                                        */
/*  Description : Contains all functions for handling the sqaure.         */
/*                                                                        */
/*  Functions  :                                                          */
/*  SqrRemFromGroup : Remove the given SQR from the given group.          */
/*  SqrPutInGroup   : Put the Square in a group.                          */
/*------------------------------------------------------------------------*/
#define INCL_WINDIALOGS
#define INCL_WIN
#define INCL_GPI
#define INCL_GPIERRORS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "drwtypes.h"
#include "dlg.h"
#include "drwutl.h"

       float SINTAB[] = { 0.000000,0.017452,0.034899,0.052336,0.069756,
                          0.087156,0.104528,0.121869,0.139173,0.156434,
                          0.173648,0.190809,0.207912,0.224951,0.241922,
                          0.258819,0.275637,0.292372,0.309017,0.325568,
                          0.342020,0.358368,0.374607,0.390731,0.406737,
                          0.422618,0.438371,0.453990,0.469472,0.484810,
                          0.500000,0.515038,0.529919,0.544639,0.559193,
                          0.573576,0.587785,0.601815,0.615661,0.629320,
                          0.642788,0.656059,0.669131,0.681998,0.694658,
                          0.707107,0.719340,0.731354,0.743145,0.754709,
                          0.766044,0.777146,0.788011,0.798635,0.809017,
                          0.819152,0.829038,0.838670,0.848048,0.857167,
                          0.866025,0.874620,0.882948,0.891006,0.898794,
                          0.906308,0.913545,0.920505,0.927184,0.933580,
                          0.939693,0.945519,0.951056,0.956305,0.961262,
                          0.965926,0.970296,0.974370,0.978148,0.981627,
                          0.984808,0.987688,0.990268,0.992546,0.994522,
                          0.996195,0.997564,0.998630,0.999391,0.999848,
                          1.000000,0.999848,0.999391,0.998630,0.997564,
                          0.996195,0.994522,0.992546,0.990268,0.987688,
                          0.984808,0.981627,0.978148,0.974370,0.970296,
                          0.965926,0.961262,0.956305,0.951057,0.945519,
                          0.939693,0.933581,0.927184,0.920505,0.913546,
                          0.906308,0.898794,0.891007,0.882948,0.874620,
                          0.866026,0.857167,0.848048,0.838671,0.829038,
                          0.819152,0.809017,0.798636,0.788011,0.777146,
                          0.766045,0.754710,0.743145,0.731354,0.719340,
                          0.707107,0.694659,0.681999,0.669131,0.656059,
                          0.642788,0.629321,0.615662,0.601815,0.587786,
                          0.573577,0.559193,0.544639,0.529920,0.515038,
                          0.500000,0.484810,0.469472,0.453991,0.438372,
                          0.422619,0.406737,0.390732,0.374607,0.358368,
                          0.342020,0.325569,0.309017,0.292372,0.275638,
                          0.258819,0.241922,0.224952,0.207912,0.190809,
                          0.173649,0.156435,0.139174,0.121870,0.104529,
                          0.087156,0.069757,0.052336,0.034900,0.017453,
                          0.000000,-0.017452,-0.034899,-0.052336,-0.069756,
                          -0.087155,-0.104528,-0.121869,-0.139173,-0.156434,
                          -0.173648,-0.190808,-0.207911,-0.224951,-0.241921,
                          -0.258819,-0.275637,-0.292371,-0.309017,-0.325568,
                          -0.342020,-0.358367,-0.374606,-0.390731,-0.406736,
                          -0.422618,-0.438371,-0.453990,-0.469471,-0.484809,
                          -0.500000,-0.515038,-0.529919,-0.544639,-0.559192,
                          -0.573576,-0.587785,-0.601815,-0.615661,-0.629320,
                          -0.642787,-0.656059,-0.669130,-0.681998,-0.694658,
                          -0.707106,-0.719339,-0.731353,-0.743144,-0.754709,
                          -0.766044,-0.777146,-0.788010,-0.798635,-0.809017,
                          -0.819152,-0.829037,-0.838670,-0.848048,-0.857167,
                          -0.866025,-0.874619,-0.882947,-0.891006,-0.898794,
                          -0.906307,-0.913545,-0.920505,-0.927184,-0.933580,
                          -0.939692,-0.945518,-0.951056,-0.956305,-0.961261,
                          -0.965926,-0.970296,-0.974370,-0.978147,-0.981627,
                          -0.984808,-0.987688,-0.990268,-0.992546,-0.994522,
                          -0.996195,-0.997564,-0.998630,-0.999391,-0.999848,
                          -1.000000,-0.999848,-0.999391,-0.998630,-0.997564,
                          -0.996195,-0.994522,-0.992546,-0.990268,-0.987688,
                          -0.984808,-0.981627,-0.978148,-0.974370,-0.970296,
                          -0.965926,-0.961262,-0.956305,-0.951057,-0.945519,
                          -0.939693,-0.933581,-0.927184,-0.920505,-0.913546,
                          -0.906308,-0.898794,-0.891007,-0.882948,-0.874620,
                          -0.866026,-0.857168,-0.848049,-0.838671,-0.829038,
                          -0.819152,-0.809017,-0.798636,-0.788011,-0.777147,
                          -0.766045,-0.754710,-0.743145,-0.731354,-0.719340,
                          -0.707107,-0.694659,-0.681999,-0.669131,-0.656060,
                          -0.642788,-0.629321,-0.615662,-0.601816,-0.587786,
                          -0.573577,-0.559194,-0.544640,-0.529920,-0.515039,
                          -0.500001,-0.484810,-0.469472,-0.453991,-0.438372,
                          -0.422619,-0.406737,-0.390732,-0.374608,-0.358369,
                          -0.342021,-0.325569,-0.309018,-0.292373,-0.275638,
                          -0.258820,-0.241923,-0.224952,-0.207913,-0.190810,
                          -0.173649,-0.156435,-0.139174,-0.121870,-0.104529,
                          -0.087157,-0.069757,-0.052337,-0.034901,-0.017453};

       float COSTAB[] = { 1.000000,0.999848,0.999391,0.998630,0.997564,
                          0.996195,0.994522,0.992546,0.990268,0.987688,
                          0.984808,0.981627,0.978148,0.974370,0.970296,
                          0.965926,0.961262,0.956305,0.951057,0.945519,
                          0.939693,0.933580,0.927184,0.920505,0.913545,
                          0.906308,0.898794,0.891007,0.882948,0.874620,
                          0.866025,0.857167,0.848048,0.838671,0.829038,
                          0.819152,0.809017,0.798636,0.788011,0.777146,
                          0.766044,0.754710,0.743145,0.731354,0.719340,
                          0.707107,0.694658,0.681998,0.669131,0.656059,
                          0.642788,0.629321,0.615662,0.601815,0.587785,
                          0.573577,0.559193,0.544639,0.529919,0.515038,
                          0.500000,0.484810,0.469472,0.453991,0.438371,
                          0.422618,0.406737,0.390731,0.374607,0.358368,
                          0.342020,0.325568,0.309017,0.292372,0.275638,
                          0.258819,0.241922,0.224951,0.207912,0.190809,
                          0.173648,0.156435,0.139173,0.121870,0.104529,
                          0.087156,0.069757,0.052336,0.034900,0.017453,
                          0.000000,-0.017452,-0.034899,-0.052336,-0.069756,
                          -0.087156,-0.104528,-0.121869,-0.139173,-0.156434,
                          -0.173648,-0.190809,-0.207911,-0.224951,-0.241922,
                          -0.258819,-0.275637,-0.292371,-0.309017,-0.325568,
                          -0.342020,-0.358368,-0.374606,-0.390731,-0.406736,
                          -0.422618,-0.438371,-0.453990,-0.469471,-0.484809,
                          -0.500000,-0.515038,-0.529919,-0.544639,-0.559193,
                          -0.573576,-0.587785,-0.601815,-0.615661,-0.629320,
                          -0.642787,-0.656059,-0.669130,-0.681998,-0.694658,
                          -0.707107,-0.719340,-0.731353,-0.743145,-0.754709,
                          -0.766044,-0.777146,-0.788010,-0.798635,-0.809017,
                          -0.819152,-0.829037,-0.838670,-0.848048,-0.857167,
                          -0.866025,-0.874620,-0.882947,-0.891006,-0.898794,
                          -0.906308,-0.913545,-0.920505,-0.927184,-0.933580,
                          -0.939693,-0.945518,-0.951056,-0.956305,-0.961262,
                          -0.965926,-0.970296,-0.974370,-0.978148,-0.981627,
                          -0.984808,-0.987688,-0.990268,-0.992546,-0.994522,
                          -0.996195,-0.997564,-0.998630,-0.999391,-0.999848,
                          -1.000000,-0.999848,-0.999391,-0.998630,-0.997564,
                          -0.996195,-0.994522,-0.992546,-0.990268,-0.987688,
                          -0.984808,-0.981627,-0.978148,-0.974370,-0.970296,
                          -0.965926,-0.961262,-0.956305,-0.951057,-0.945519,
                          -0.939693,-0.933581,-0.927184,-0.920505,-0.913546,
                          -0.906308,-0.898794,-0.891007,-0.882948,-0.874620,
                          -0.866026,-0.857168,-0.848048,-0.838671,-0.829038,
                          -0.819152,-0.809017,-0.798636,-0.788011,-0.777146,
                          -0.766045,-0.754710,-0.743145,-0.731354,-0.719340,
                          -0.707107,-0.694659,-0.681999,-0.669131,-0.656059,
                          -0.642788,-0.629321,-0.615662,-0.601816,-0.587786,
                          -0.573577,-0.559193,-0.544639,-0.529920,-0.515039,
                          -0.500001,-0.484810,-0.469472,-0.453991,-0.438372,
                          -0.422619,-0.406737,-0.390732,-0.374607,-0.358369,
                          -0.342021,-0.325569,-0.309018,-0.292372,-0.275638,
                          -0.258819,-0.241922,-0.224952,-0.207912,-0.190810,
                          -0.173649,-0.156435,-0.139174,-0.121870,-0.104529,
                          -0.087156,-0.069757,-0.052337,-0.034900,-0.017453,
                          0.000000,0.017452,0.034899,0.052335,0.069756,
                          0.087155,0.104528,0.121869,0.139172,0.156434,
                          0.173648,0.190808,0.207911,0.224950,0.241921,
                          0.258819,0.275637,0.292371,0.309016,0.325567,
                          0.342020,0.358367,0.374606,0.390730,0.406736,
                          0.422618,0.438371,0.453990,0.469471,0.484809,
                          0.500000,0.515038,0.529919,0.544638,0.559192,
                          0.573576,0.587785,0.601814,0.615661,0.629320,
                          0.642787,0.656058,0.669130,0.681998,0.694658,
                          0.707106,0.719339,0.731353,0.743144,0.754709,
                          0.766044,0.777145,0.788010,0.798635,0.809016,
                          0.819152,0.829037,0.838670,0.848048,0.857167,
                          0.866025,0.874619,0.882947,0.891006,0.898794,
                          0.906307,0.913545,0.920505,0.927183,0.933580,
                          0.939692,0.945518,0.951056,0.956304,0.961261,
                          0.965926,0.970296,0.974370,0.978147,0.981627,
                          0.984808,0.987688,0.990268,0.992546,0.994522,
                          0.996195,0.997564,0.998629,0.999391,0.999848};

/*------------------------------------------------------------------------*/
/* Name : RotateSegment.                                                  */
/*                                                                        */
/* Description : Rotates a square around a givent center point.           */
/*                                                                        */
/* USHORT Deg : Number of degrees to rotate.                              */
/* POINTL ptlCenter : Center to rotate around.                            */
/* *PPOINT ptl : a pointer array of pointl structutes containing the      */
/* points to rotate.                                                      */
/* USHORT nr: Number of elements in the array of points.                  */
/*------------------------------------------------------------------------*/
void RotateSqrSegment(double Angle, POINTL ptlCenter, POINTL *ptls, USHORT points )
{
   USHORT i;
   POINTL tmpptl[350];  /* pointer to a pointer of pointl structs */

   /* some checks */
  if (!ptls ) return;

  for ( i = 0; i < points; i++)
  {
  tmpptl[i].x = (ptls[i].x - ptlCenter.x)*cos(Angle)-
                (ptls[i].y - ptlCenter.y)*sin(Angle)+ ptlCenter.x;
  tmpptl[i].y = (ptls[i].x - ptlCenter.x)*sin(Angle)+
                (ptls[i].y - ptlCenter.y)*cos(Angle)+ ptlCenter.y;
  }
   for ( i = 0; i < points; i++)
   {
       ptls[i].x = tmpptl[i].x;
       ptls[i].y = tmpptl[i].y;
   }
   return;
}

