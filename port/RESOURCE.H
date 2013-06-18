/*------------------------------------------------------------------------*/
/*  Name: resource.h                                                      */
/*                                                                        */
/*  Description : Contains (almost) all resource id's. The rest is def    */
/*                in dlg.h.                                               */
/*                                                                        */
/*                                                                        */
/*------------------------------------------------------------------------*/
#define ID_RESOURCE   150
#define ID_DRAWIT     150
#define IDM_PUSH      128
#define IDM_LINE      129
#define IDM_DOT       130
#define IDM_DASHDOT   131
#define IDM_DOUBLEDOT 132
#define IDM_LONGDASH  133
#define IDM_LIN       202
#define IDM_FNT       203
#define IDM_FONT      204
#define IDM_CUSTOM    205
#define IDM_CST       206
#define IDM_FILEMENU  207
#define IDM_COPYATTR  208     /* copy attrib from selected obj to applic */
#define IDM_COPYATTR2OBJ 209  /* copy attributes from applic into obj    */
#define IDM_ATTR      210
#define IDM_ANG0      211
#define IDM_ANG2      212
#define IDM_TEXT      213
#define IDM_SELTEXT   214
#define IDD_TXT       215
#define IDM_GRD       216
#define IDM_ROT       217
#define IDM_FLIP      218
#define IDM_FLIPH     219
#define IDM_FLIPV     220
#define IDM_FILE      229
#define IDM_FILN      230
#define IDM_SAVE      231
#define IDM_SAVA      232
#define IDM_PRINT     233
#define IDM_PRINTAS   234
#define IDM_SELECTALL 235
#define IDM_DESELECTALL 236
#define IDM_EXIT      237

#define IDM_BMP          241
#define IDM_IMAGE        242
#define IDM_SELBMP       243
#define IDM_NORBMP       244
#define IDM_BOTTOP       245
#define IDM_TOPBOT       246
#define IDM_CHARBOX      247
#define IDM_FLIPIMG      248   /* Flip Image.                         */
#define IDM_FLIPHIMG     249   /* Flip Image Horizontal               */
#define IDM_FLIPVIMG     250   /* Flip Image vertical.                */
#define IDM_SPEC         251   /* Special objects                     */
#define IDM_SPECLINES    252   /* Special Lines                       */
#define IDM_MM           253   /* Show coords in paper values  0.1 mm */
#define IDM_INCH         254   /* Paper coords in inches.             */
#define IDM_COORD        256   /* SUBMENU Coord system paper/window   */
#define IDM_PARTARC      257   /* in object, partial arc drawing open */
#define IDM_FPARTARC     258   /* in object, partial arc filled figure*/
#define IDM_SNAPONGRID   259   /* Snap all drawing on a grid.....     */
#define IDM_GRIDOFF      260   /* Grid off selection                  */
#define IDM_GRIDON       261   /* Grid on  selection                  */
#define IDM_REFRESH      262   /* refresh now                         */
#define IDM_METAEXP      263   /* Export as OS/2 metafile             */
#define IDM_SAVEIMG      264   /* Save select                         */
#define IDM_BOX          265   /* Box drawing.                        */

#define IDM_PRINTPREVIEW 266   /* print preview  dialog....           */
#define IDM_IMPORT       267   /* FILE: submenu import image          */
#define IDM_METAFILE     268   /* FILE: import metafile               */
#define IDM_EXPSEL       269   /* FILE: Export selected area as....   */
#define IDM_EXPSELMETA   270   /*       Export selected area METAFILE */
#define IDM_EXPSELBMP    271   /*       Export selected area OS2BMP   */
#define IDM_EXPSELPCX    272   /*       Export selected area PCX file */
#define IDM_EXPSELGIF    273   /*       Export selected area GIF file */
#define IDM_EXPSELTIF    274   /*       Export selected area TIF file */
#define IDM_EXPSELTGA    275   /*       Export selected area TGA file */
/*
** Imagemenu.
*/
#define IDM_IMGROT       280    /* Image rotate submenu.               */
#define IDM_IMGROT90     281    /* Image rotate 90 degrees.            */
#define IDM_IMGROT270    282    /* Image rotate -90 (270) degrees.     */
#define IDM_ROTATION     283    /* Image color rotation                */
#define IDM_ROTATIONUP   284    /* Image color rotation   UP (PGUP)    */
#define IDM_ROTATIONDOWN 285    /* Image color rotation   DOWN (PGDOWN)*/
#define IDM_GRAYSCALE    286    /* Convert image to b&w image          */
#define IDM_SAVEOS2BMP   287    /* SAVE selected image as os/2 bmp     */
#define IDM_CROP         288    /* Image crop.                         */
#define IDM_CIRBMP       289    /* Make image circular.                */
#define IDM_RESTASPECT   290    /* Restore aspect ratio of image.      */
#define IDM_RESTORESIZE  291    /* Restore the size of the image.      */
#define IDM_UNDOPALETTE  292    /* Undo palette change in pal based img*/
#define IDM_RESTOREIMG   293    /* Restore submenu for images.         */
#define IDM_IMGCLIPPATH  294    /* Add a clippath to the selected image*/
#define IDM_SETCLIPPATH  295    /* Add a clippath to the selected image*/
#define IDM_DELCLIPPATH  296    /* Delete clippath from image.         */
#define IDM_BLACKWHITE   297    /* Convert image to b&w image.         */
/*
** Options-menu, submenu for filling patterns. Ownerdraw menu!
** Plus alignment.
*/
#define IDM_ALIGN      310     /* OPTIONS: alignment of multi select. */
#define IDM_ALIGNHORZ  311     /* Horizontal alignment of multi select*/
#define IDM_ALNHORZB   312     /* Horizontal alignment on bottom obj  */
#define IDM_ALNHORZT   313     /* Horizontal alignment on top obj     */
#define IDM_ALIGNVERT  314     /* Vertical alignment of multi select  */
#define IDM_ALNVERTL   315     /* Vertical alignment using leftside as ref */
#define IDM_ALNVERTR   316     /* Vertical alignment using rightside as ref*/

#define IDM_HALFTONE   320
#define IDM_SOLID      321
#define IDM_DENSE1     322
#define IDM_DENSE2     323
#define IDM_DENSE3     324
#define IDM_VERT       325
#define IDM_HORIZ      326
#define IDM_DIAG1      327
#define IDM_DIAG2      328
#define IDM_DIAG3      329
#define IDM_NOPATTERN  330
#define IDM_GRADIENT   331
#define IDM_FOUNTAIN   332
#define IDM_COLORSET   333 /* submenu color setting */
#define IDM_ROUNDING   334 /* box rounding.         */
#define IDM_CLROUTL    335 /* color outline         */
/*
** EditMenu
*/
#define IDM_EDIT       335
#define IDM_GROUP      336
#define IDM_UNGROUP    337
#define IDM_PASTE      338
#define IDM_CUT        339
#define IDM_COPY       340
#define IDM_CLEAR      341
/*
** Textobject MENU
*/
#define IDM_INSTEXT    343     /* Create new text block while in input mode*/
#define IDM_TXTCIRC    345     /* Make selected text circular */
#define IDM_EDITTEXT   344     /* Edit selected text          */
/*
** IMAGE MENU.
*/
#define IDM_COLORSMENU      346
#define IDM_COLORCOPY       347
#define IDM_COLORPASTE      348
#define IDM_IMGOTHERFORMATS 349
#define IDM_COLORFILTER     350
#define IDM_IMGINVERT       351
#define IDM_IMGBRIGHT       352

#define IDM_LNJOIN      363
#define IDM_LNDEFAULT   364
#define IDM_LNBEVEL     365
#define IDM_LNROUND     366
#define IDM_LNMITRE     367

/*
** Export selection in other formats.
*/
#define IDM_OTHERFORM   368
#define IDM_BITMAPEXP   369
/*
** regular polypoint creation....
*/
#define IDM_REGPOLYSTAR   370
#define IDM_FREESTYLELINE 371 /* Freestyle lines submenu    */
#define IDM_FREESTYLEFINE 372 /* Freestyle fine granularity */
#define IDM_FREESTYLENORM 373 /* Freestyle normal           */
/*
** VIEW menu
*/
#define IDM_VIEW         375
#define IDM_SUPPRESSFILL 376
#define IDM_SUPPRESSON   377
#define IDM_SUPPRESSOFF  378
#define IDM_ZOOMMENU     379
#define IDM_ZOOM2        380
#define IDM_ZOOM5        381
#define IDM_ZOOM10       382
#define IDM_ZOOM15       383
#define IDM_ZOOM20       384
#define IDM_ZOOM25       385
#define IDM_ZOOM30       386
/*
** OPTIONS menu
*/
#define IDM_OPTIONS      388
#define IDM_FORMSIZE     389
#define IDM_SELTYPE      390
#define IDM_ONAREA       391
#define IDM_ONLINE       392
/*
** Lotus pic file & Windows metafile menu items
*/
#define IDM_WINDOWSWMF   393
#define IDM_LOTUSPIC     394
/*
** Arrow dialog menu item.
*/
#define IDM_ARROWS       395
/*
** Text object dialog..
*/
#define ID_TXTOBJ       400   /*Text dialog id                 */
#define ID_TXWND        402   /* Window showing the result     */
#define ID_LTTEXT       404   /* static text saying 'text'     */
/* Next 3 items are tied together with the coding!             */
#define ID_SPNVERT     405   /* Spin for vert shade.          */
#define ID_SPNHORZ     406   /* Spin for horz shade.          */
#define ID_SPNSHEAR    407   /* Shear spin button.            */

#define ID_LTSHEAR      409   /* 'shear'                       */
#define ID_CHKOUTLINE   410   /* Checkbox for outlinefont      */
#define ID_RADSHADE     411   /* Radio button for shadecolor   */
#define ID_RADOUTLINE   412   /* Radio button for outlinecolor */
#define ID_GRPCOLOR     413   /* Groupbox 'color'              */
#define ID_LTCX         414   /* Text box cx                   */
#define ID_LTCY         415   /* Text box cy                   */
#define ID_SPNCX        416   /* Spinbutton for cx of font.    */
#define ID_SPNCY        417   /* Spinbutton for cy of font.    */
#define ID_SPNLAYER     418   /* Spinbutton for layer setting  */
#define ID_GRPSIZE      419   /* Groupbox for font size.       */
#define ID_VALCOLOR     420   /* Color valueset.               */
#define ID_LTSHADEX     422   /* text window 'shadex'          */
#define ID_LTSHADEY     423   /* text window 'shadey'          */
/* Next 3 items are tied together with the coding!             */
#define ID_LTSHADEYVAL  424   /* text window 0 mm initial text */
#define ID_LTSHADEXVAL  425   /* text window 0 mm cy shading   */
#define ID_LTSHEARVAL   426   /* text field showing shear value*/

#define ID_GRPLAYER     427   /* Groupbox for layer spinbutton */
/*
** Linetype dialog in option menu.
*/
#define ID_LINETYPE_DEFAULT                500
#define ID_LINETYPE_DOT                    501
#define ID_LINETYPE_SHORTDASH              502
#define ID_LINETYPE_DASHDOT                503
#define ID_LINETYPE_DOUBLEDOT              504
#define ID_LINETYPE_LONGDASH               505
#define ID_LINETYPE_DASHDOUBLEDOT          506
#define ID_LINETYPE_SOLID                  507
#define ID_LINETYPE_INVISIBLE              508
#define ID_WLINETYPE                       509 /* Window showing result */
#define ID_LINEGROUP                       510 /* groupbox.             */
#define GRP_LNEXAMPLE                      511 /* groupbox (example)    */
#define IDM_FINDCOLOR                      512 /* find color under mouse ptr */
/*
** Linetype menu
** Follows the defines as specified in the toolkit for OS/2.
** lintype_default 0 ..... linetype_alternate 9.
*/
#define IDM_LINETYPE                       549
#define IDM_LINEDEFAULT                    550
#define IDM_LINEDOT                        551
#define IDM_LINESHORTDASH                  552
#define IDM_LINEDASHDOT                    553
#define IDM_LINEDOUBLEDOT                  554
#define IDM_LINELONGDASH                   555
#define IDM_LINEDASHDOUBLEDOT              556
#define IDM_LINESOLID                      557
#define IDM_LINEINVISIBLE                  558
#define IDM_LINEALTERNATE                  559
/*
** Gradient color dialog...
*/
#define ID_GRADIENT    800
#define ID_GRADIENTWND 810
#define ID_GRADSTART   815
#define ID_GRADSWEEP   820
#define ID_TXT360      825
#define ID_TXT270      850
#define ID_TXT180      875
#define ID_TXT90       900
#define ID_TXT0        925
#define ID_GRDSAT      950
#define ID_TXTSAT      975
#define ID_TXTSTART    980
#define ID_TXTSWEEP    985
#define ID_GRADLEFT    990
#define ID_GRADRIGHT   995
#define ID_GRADTOP    1000
#define ID_GRADBOTTOM 1005
#define ID_GRADRANGE  1010
#define ID_GRADCENTER 1015
/*
** Circular text dialog.
*/
#define ID_TXTCIRCULAR  1100 /* dialog id.                            */
#define ID_TXTCIRWIN    1105 /* Window showing the result             */
#define ID_SLRSTART     1110 /* Slider for the start angle            */
#define ID_SLRRANGE     1115 /* Slider for the range                  */
#define ID_STXTSTART    1130 /* static text element saying start.     */
#define ID_STXTRANGE    1135 /* static text element saying range.     */
#define ID_CHKCIRCULAR  1140 /* Checkbox to switch circular ON/OFF    */
/*
** Change text dialog....
*/
#define DLG_CHTXT       1145  /* The dialog template id.               */
#define ID_CHTXT        1146  /* The entry field for changing the text */

/*
** Layer dialog
*/
#define ID_LAYER        1200
#define ID_LAYERCLASS   1201
#define ID_LAYERPREV    1202
#define ID_LAYERNEXT    1203
#define ID_LAYERTXT1    1204
#define ID_LAYERTXT2    1205
#define ID_LAYERGROUP   1206
#define ID_LAYERTXT3    1207
#define ID_LAYERCHK     1209
#define ID_LAYERACT     1210
/*
** Splash screen..
*/
#define ID_SPLASHDLG   1300
#define ID_SPLASCLS    1301   /* Window in the dialog */
#define ID_SPLTEXT1    1302
#define ID_SPLTEXT2    1303
#define ID_SPLTEXT3    1304
#define ID_SPLTEXT4    1305
#define ID_SPLTEXT5    1307
#define ID_SPLHLP      1308
/*
** Bitmap detail dialog.
*/
#define ID_IMGDETAIL     1400
#define ID_GRPIMGDETAIL  1405
#define ID_SPNIMLAYER    1410
#define ID_IMGWIDTH      1415
#define ID_IMGHEIGHT     1420
#define ID_IMGCOLORS     1425
#define ID_GFPBMP        1430
/*
** Print preview dialog
*/
#define ID_PREV          1700  /* Portrait print preview dialog */
#define ID_PREVWND       1701
#define ID_PREVGRP       1702
#define SPIN_PERCENTAGE  1705
#define GRP_PERCENTAGE   1706
#define GRP_CURFORM      1707
#define TXT_FORMNAME     1708
#define TXT_FORMWIDTH    1709
#define TXT_FORMHEIGHT   1710
#define ICO_PRINTPREV    1711
#define ID_PREVL         1720  /* Landscape print preview dialog */
#define ID_PREVOUTLINE   1730
#define ID_PRNSTRETCH    1731
#define ID_GRPDRAFT      1732

/*
** Pointers.
*/
#define IDP_CROP      1753
#define IDP_CIRCLE    1754
#define IDP_SQUARE    1755
#define IDP_POLYLINE  1756
#define IDP_PARTARC   1757
#define IDP_TEXT      1759
#define IDP_ROTATE    1760
/*
** Polyline / Spline
*/
#define ID_SPLINE      1800
#define ID_SPLINELAYER 1801
#define ID_SPLINECLOSE 1802
#define ID_SPLINETXT   1803
#define ID_SPLINEFILL  1804
#define ID_SPLINEGRP   1805
#define ID_SPLINEDEF   1806
#define ID_SPLINELINE  1807
#define ID_SPLINEFILLET 1808

/*
** Save selection into a bitmap...
*/
#define IDB_SAVESELIMG 1840
#define IDB_SAVESEL    1850
#define ID_RADPCX      1851
#define ID_RADGIF      1852
#define ID_RADTIF      1853
#define ID_RADTGA      1854
#define ID_RADOS2      1855
#define ID_RADWIN      1856
#define ID_EXPGROUP    1857
#define ID_RADTRUE     1858
#define ID_RAD256      1859
#define ID_RAD64       1860
#define ID_RAD16       1861
#define ID_CLRGROUP    1862
#define ID_GRPBBMSIZE  1863
#define ID_SPINBMPCX   1864
#define ID_SPINBMPCY   1865
#define ID_RADJPG      1866
/*
** dialog showing a spinbutton. used to define the number
** of point in a regular polypoint.
*/
#define ID_POLYPOINT   1900
#define ID_NRPOINTS    1901
#define ID_TXTMAXPTS   1902
/*
** BITMAP ID's for alignment. used in the
** owner draw menu items.
*/
#define IDB_ALIGNHORZ  2000    /* Horizontal alignment on center */
#define IDB_ALNHORZT   2001    /* Horizontal alignment on top    */
#define IDB_ALNHORZB   2002    /* Horizontal alignment on bottom */
#define IDB_ALIGNVERT  2005    /* Vertical alignment on center   */
#define IDB_ALNVERTL   2006    /* Vertical alignment on leftside */
#define IDB_ALNVERTR   2007    /* Vertical alignment on rightside*/
/*
** BITMAP ID's for the linejoin menu.
*/
#define IDB_LNMITRE    2010
#define IDB_LNROUND    2011
#define IDB_LNBEVEL    2012
/*
** Gridsize dialog.
*/
#define ID_GRIDSIZE    2100
#define ID_GRIDSPIN    2110
#define IDM_GRIDSIZE   2115 /* Menu item */
#define ID_SPINGRIDX   2120 /* Entry field gridsize x direction */
#define ID_SPINGRIDY   2121 /* Entry field gridsize Y direction */
#define ID_GRPGRID     2122
/*
** Rotate and snap dialog.
*/
#define ID_ROTSNAP       2150
#define ID_SPINROTSNAP   2155
#define ID_TXTDEGREES    2160
/*
** Formsize dialog.
*/
#define ID_MCLB          2200          // ID of MCLB control
#define ID_RADPORTRAIT   2205
#define ID_RADLANDSCAPE  2210
#define ID_FRMDIALOG     2215
#define ID_GRPFORM       2220
#define ID_CHKASDEFAULT  2225
/*
** Lotus pic-file dialog.
*/
#define PIC_FILEDLG       2300
#define ID_LTFONTNAME     2302
#define ID_PBFONT         2303
#define ID_LTFONTNAME1    2304
#define ID_PBFONT1        2305
#define ID_ICOPIC         2306
#define ID_DRPFILLING     2307
#define ID_DRPLINETYPE    2308
#define ID_CHKTXTOBJ      2309
#define PIC_WAIT          2311
#define STRID_PICINVALID  2312
#define STRID_NOFILE      2313
#define ID_PICGRP1        2314
#define ID_PICGRP2        2315
/*
** Fountain fill stuff........
*/
#define  ID_DLGFOUNTAIN         3000
#define  ID_FOUNTAINWND         3010       /* Window in dialog */
#define  ID_GRPANGLE            3011
#define  ID_SPINHORZ            3012
#define  ID_SPINVERT            3013
#define  ID_STATTXTH            3014
#define  ID_STATTXTV            3015
#define  ID_GRPCENTEROFF        3016
#define  ID_SPINANGLE           3017
#define  ID_RADCIR              3018
#define  ID_RADSQR              3019
#define  ID_RADLINEAR           3020
#define  ID_GRPDIRECTION        3021
#define  ID_PBTO                3022
#define  ID_PBFROM              3023
#define  ID_GRCOLORS            3024
#define  ID_TXTFROM             3025
#define  ID_TXTFTO              3026
#define  IDM_FOUNTAINCOLOR      3027
#define  IDM_CLR1               3028
#define  IDM_CLR2               3029
#define  IDM_CLR3               3030
#define  IDM_CLR4               3031
#define  IDM_CLR5               3032
#define  IDM_CLR6               3033
#define  IDM_CLR7               3034
#define  IDM_CLR8               3035
#define  IDM_CLR9               3036
#define  IDM_CLR10              3037
#define  IDM_CLR11              3038
#define  IDM_CLR12              3039
#define  IDM_CLR13              3040
#define  IDM_CLR14              3041
#define  IDM_CLR15              3042
#define  IDM_CLR16              3043
#define  IDM_CLR17              3044
#define  IDM_CLR18              3045
#define  IDM_CLR19              3046
#define  IDM_CLR20              3047
#define  IDM_CLR21              3048
#define  IDM_CLR22              3049
#define  IDM_CLR23              3050
#define  IDM_CLR24              3051
#define  IDM_CLR25              3052
#define  IDM_CLR26              3053
#define  IDM_CLR27              3054
#define  IDM_CLR28              3055
#define  IDM_CLR29              3056
#define  IDM_CLR30              3057
#define  IDM_CLR31              3058
#define  IDM_CLR32              3059
#define  IDM_CLR33              3060
#define  IDM_CLR34              3061
#define  IDM_CLR35              3062
#define  IDM_CLR36              3063
#define  IDM_CLR37              3064
#define  IDM_CLR38              3065
#define  IDM_CLR39              3066
#define  IDM_CLR40              3067
#define  IDM_CLR41              3068
#define  IDM_CLR42              3069

#define  IDM_FTAIN              3070 /* fountain menu item for attributes */
/*
** BOX details dialog.
*/
#define ID_BOXDETAIL            3390
#define ID_TXTHORIZ             3400
#define ID_TXTVERT              3401
#define ID_GRPROUNDING          3402
#define ID_SPINLAYER            3403
#define ID_TXTLAYER             3404
#define ID_GRPBOXLAYER          3405
#define ID_BOXWND               3407
#define ID_SPINROUND            3409

/*
** error message ids which are in the resource stringtable
*/
#define IDS_WRONGJSPFORMAT        3999
#define IDS_HELPWINDOWTITLE       4000
#define IDS_CANNOTLOADSTRING      4001
#define IDS_HELPLIBRARYNAME       4002
#define IDS_HELPLOADERROR         4003
#define IDS_HELPDISPLAYERROR      4004
#define IDS_HELPGENERALERROR      4005
#define IDS_HELPKEYSERROR         4006
#define IDS_HELPINDEXERROR        4007

#define IDS_FILEOPENEXT           4008
#define IDS_APPNAME               4009
#define IDS_UNTITLED              4010
#define IDS_TITLEBARSEPARATOR     4011
#define IDS_UNKNOWNMSG            4012
#define IDS_DRAWING               4013
#define IDS_WAIT                  4014
#define IDS_PRINTING              4015
#define IDS_CALC                  4016
#define IDS_SAVESELIMGAS          4020 /*Save the selected image as..PCX,TIF.*/
#define IDS_IMAGENOTINGROUP       4021
#define IDS_ASKFORSAVE            4022
#define IDS_WARNING               4023
#define IDS_CANNOTOPENFILE        4024
#define IDS_FILEOPENERROR         4025
#define IDS_WRONGFORMAT           4026
#define IDS_NOTADRAWITFILE        4027
#define IDS_SAVING                4028
#define IDS_LOADING               4029

/*
**HELP
*/
#define IDM_HELP                            9000
#define IDM_HELPUSINGHELP                   9100
#define IDM_HELPGENERAL                     9200
#define IDM_HELPKEYS                        9300
#define IDM_HELPINDEX                       9400
#define IDM_HELPTUTORIAL                    9500
#define IDM_HELPPRODINFO                    9600
#define DID_HELP                            9601

#define IDM_ZOOM                            9602
#define ID_RADHORIZ                 3001
#define ID_RADVERTICAL              3002
/*
**--------------------Toolbar definitions-----------------------------
*/
#define IDB_SELECT                  4000
#define IDB_TEXT                    4001
#define IDB_CIRCLE                  4002
#define IDB_SQUARE                  4003
#define IDB_LINE                    4004
#define IDB_FLINE                   4005
#define IDB_TRIANGLE                4006
#define IDB_SPLINE                  4007
#define IDB_FILL                    4008
#define IDB_FONT                    4009
#define IDB_LAYER                   4010
#define IDB_FCHANGE                 4011
#define IDB_CHGOBJ                  4012
#define IDB_ZOOM                    4013
#define IDB_ROTATE                  4014

#define IDB_PSELECT                 4040
#define IDB_PTEXT                   4041
#define IDB_PCIRCLE                 4042
#define IDB_PSQUARE                 4043
#define IDB_PLINE                   4044
#define IDB_PFLINE                  4045
#define IDB_PTRIANGLE               4046
#define IDB_PSPLINE                 4047
#define IDB_PLAYER                  4050
#define IDB_PCHGOBJ                 4052
#define IDB_PROTATE                 4054
#define IDB_BLCKTEXT                4055 /* Bitmap for blocktext button */
/*
** Circle buttons, the bitmap id's
*/
#define IDB_CLTOP                   4060 /* Circle left top     */
#define IDB_CLBOT                   4061 /* Left bottom         */
#define IDB_CRTOP                   4062 /* Right top           */
#define IDB_CRBOT                   4063 /* Right bottom.       */
#define IDB_CLFTOP                  4064 /* Left top closed     */
#define IDB_CLFBOT                  4065 /* Left bottom closed  */
#define IDB_CRFTOP                  4066 /* Right top closed    */
#define IDB_CRFBOT                  4067 /* Right bottom closed */
/*
** Toolbar buttons id's. As they are
** known in the WM_COMMAND.
*/
#define IDBTN_SELECT                4100
#define IDBTN_TEXT                  4101
#define IDBTN_TBCIRCLE              4102 /* Starts the Circle toolbar*/
#define IDBTN_SQUARE                4103
#define IDBTN_LINE                  4104
#define IDBTN_FLINE                 4105
#define IDBTN_TRIANGLE              4106
#define IDBTN_SPLINE                4107
#define IDBTN_FILL                  4108
#define IDBTN_FONT                  4109
#define IDBTN_LAYER                 4110
#define IDBTN_FCHANGE               4111
#define IDBTN_CHGOBJ                4112
#define IDBTN_ZOOM                  4113
#define IDBTN_ROTATE                4114
/*
** Command ID's comming from the circle buttons.
** Look out IDBTN_CIRCLE ... IDBTN_CRFBOT is a range!!!
** See dlg_cir.c
*/
#define IDBTN_CIRCLE                4118 /* Starts the Circle tbar */
#define IDBTN_CLTOP                 4120 /* Circle left top     */
#define IDBTN_CLBOT                 4121 /* Left bottom         */
#define IDBTN_CRTOP                 4122 /* Right top           */
#define IDBTN_CRBOT                 4123 /* Right bottom.       */
#define IDBTN_CLFTOP                4124 /* Left top closed     */
#define IDBTN_CLFBOT                4125 /* Left bottom closed  */
#define IDBTN_CRFTOP                4126 /* Right top closed    */
#define IDBTN_CRFBOT                4127 /* Right bottom closed */
/*
** TEXT BUTTONS
*/
#define IDBTN_SNTEXT                4128 /* Single text objects */
#define IDBTN_BLCKTEXT              4129 /* Block text objects  */
/*
** ID of the main toolbar....
*/
#define ID_TBAR                     4200
#define ID_CIRTBAR                  4250
#define ID_TEXTTBAR                 4255
/*
** Circle dialog.
*/
#define DLG_CIROBJECT               5000
#define ID_SPNCIRLAYER              5001
#define ID_SPNSWEEP                 5002
#define ID_SPNSTART                 5003
#define ID_CIRWIN                   5004
#define ID_CHKOPENCIR               5005
#define ID_CHKCIR                   5006

/*
** Arrow dialog
*/
#define DLG_ARROWS                  6000
#define ID_LINESTART                6005
#define ID_LINEEND                  6010
#define DID_APPLY                   6015
#define DID_CHKHALF                 6020
#define ID_RADHALF                  6025
#define ID_SPINARROWSIZE            6030
#define DID_DEFAULT                 6035
#define ID_ARROWWND                 6040
/*
** Bitblit ROPCODES-- Code needs consecutive numbers for the ropcodes!!
*/
#define IDM_ROPCODE        6051
#define IDM_SRCCOPY        6052      /* NORMAL */
#define IDM_SRCPAINT       6053
#define IDM_SRCAND         6054
#define IDM_SRCINVERT      6055
#define IDM_SRCERACE       6056
#define IDM_NOTSRCCOPY     6057
#define IDM_NOTSRCERASE    6058
#define IDM_MERGECOPY      6059
#define IDM_PATCOPY        6060
#define IDM_PATPAINT       6061
#define IDM_PATINVERT      6062
#define IDM_PATDSTINVERT   6063
#define IDM_PATZERO        6064
#define IDM_PATONE         6065
/*
** CLIPPATH Popup menu id.
*/
#define IDM_CLIPPATH       6080
/*
** Dialogbox containing the multiline editbox for blocktext editing.
*/
#define DLG_BLOCKTEXT      6500  /* The dialog itself     */
#define ID_BTEXTMLE        6510  /* The MLE in the dialog */
/*
** BLOCKTEXT POPUPMENU definitions.
** Keep numbers with an offset of 1!! 
** See drwmenu where the numbers are used in a loop. 
*/
#define IDM_BLOCKTEXT      6600
#define IDM_BTEXTALIGN     6611  /* Submenu item */
#define IDM_BTEXTLEFT      6612  /* Left  alignment of text in column */
#define IDM_BTEXTRIGHT     6613  /* Right alignment of text in column */
#define IDM_BTEXTJUST      6614  /* Justified text.                   */
#define IDM_BTEXTCENTER    6615  /* Center alignment of text in column*/

#define IDM_BTEXTSPACE         6619  /* Submenu item         */
#define IDM_BTEXTSPACENONE     6620
#define IDM_BTEXTSPACESINGLE   6621  /* Normal line spacing. */
#define IDM_BTEXTSPACEHALF     6622  /* Half line spacing.   */
#define IDM_BTEXTSPACEDOUBLE   6623  /* Double line spacing. */
/*
** Shading bitmaps
*/
#define IDB_LBSHADE            6800
#define IDB_LTSHADE            6805
#define IDB_RTSHADE            6810
#define IDB_RBSHADE            6815
/*
** Shading menu options
*/
#define IDM_OBJSHADE           6816  /* Submenu option */
#define IDM_LBSHADE            6821
#define IDM_LTSHADE            6822
#define IDM_RTSHADE            6823
#define IDM_RBSHADE            6824
#define IDM_TOPSHADE           6825
#define IDM_BOTTOMSHADE        6826
#define IDM_LEFTSHADE          6827
#define IDM_RIGHTSHADE         6828
#define IDM_SHADENONE          6829  /* No shading    */
#define IDM_SHADECLR           6840  /* Shading color */
/*
** Shading dialog
*/
#define DLG_SHADING            7000
#define ID_GRPSHADECOLOR       7001
#define ID_BTNSHADECOLOR       7002
#define ID_RADSHADEMIN         7003
#define ID_RADSHADEMED         7004
#define ID_RADSHADELARGE       7005
#define ID_GRPSHADEDEPTH       7006
/*
** Metafile object save option in popupmenu.
*/
#define IDM_METAPOPUP          7100
#define IDM_SAVEMETA           7105
/*
** Figure menu for arrows, consequtive numbers!
*/
#define IDM_ARROW              7109
#define IDM_ARROW1             7110
#define IDM_ARROW2             7111
#define IDM_ARROW3             7112
#define IDM_ARROW4             7113
#define IDM_ARROW5             7114
#define IDM_ARROW6             7115
#define IDM_ARROW7             7116
#define IDM_ARROW8             7117
#define IDM_ARROW9             7118
#define IDM_ARROW10            7119
#define IDM_ARROW11            7120
#define IDM_ARROW12            7121

/*
** Arrow menu bitmap id's
*/
#define IDB_ARROW1             7130
#define IDB_ARROW2             7131
#define IDB_ARROW3             7132
#define IDB_ARROW4             7133
#define IDB_ARROW5             7134
#define IDB_ARROW6             7135
#define IDB_ARROW7             7136
#define IDB_ARROW8             7137
#define IDB_ARROW9             7138
#define IDB_ARROW10            7139
#define IDB_ARROW11            7140
#define IDB_ARROW12            7141

/*
** Linesize menu option..
*/
#define IDM_NOOUTLINE   7353 /* for text only */
#define IDM_LNSIZE      7354
#define IDM_LNNORM      7355
#define IDM_LN2         7356
#define IDM_LN3         7357
#define IDM_LN4         7358
#define IDM_LN5         7359
#define IDM_LN6         7360
#define IDM_LN7         7361
#define IDM_LN8         7362
#define IDM_LN9         7363
#define IDM_LN10        7364
#define IDM_LN11        7365
#define IDM_LN12        7366
#define IDM_LN13        7367
#define IDM_LN14        7368
#define IDM_LN15        7369
#define IDM_LN16        7370
#define IDM_LNEND       7370

#define IDM_DUMPSPLINE         7200

/*
** Properties menu item
*/
#define IDM_PROPERTIES         7205
