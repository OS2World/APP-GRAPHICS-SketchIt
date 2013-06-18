/*
** Function prototypes for fountainfill.
*/
MRESULT LinearFountFill(HPS hps, RECTL *prcl,
                      ULONG ulStart, ULONG ulEnd, LONG lPct);

MRESULT SquareFountFill(HPS hps, RECTL *prcl,
                        ULONG ulStart, ULONG ulEnd, LONG lxOffset,LONG lyOffset);

MRESULT CircularFountFill(HPS hps, RECTL *prcl,
                        ULONG ulStart, ULONG ulEnd, LONG lxOffset,LONG lyOffset);


void FountainFill(WINDOWINFO *pwi,HPS hps,RECTL *prcl,FOUNTAIN *fnt);
void RegisterFountain(HAB hab);
MRESULT EXPENTRY FountainDlgProc(HWND,ULONG,MPARAM,MPARAM);
