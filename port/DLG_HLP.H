
BOOL InitHelp(HWND hFrame);
VOID HelpUsingHelp(MPARAM mp2);
VOID HelpGeneral(MPARAM mp2);
VOID HelpKeys(MPARAM mp2);
VOID HelpIndex(MPARAM mp2);
VOID HelpTutorial(MPARAM mp2);
VOID HelpProdInfo(MPARAM mp2);
VOID DisplayHelpPanel(SHORT nPanelId);
VOID DestroyHelpInstance(VOID);
VOID ShowDlgHelp(HWND hwnd);

#define PANEL_HELP                          2910
#define PANEL_HELPHELPFORHELP               2920
#define PANEL_HELPEXTENDED                  2930
#define PANEL_HELPKEYS                      2940
#define PANEL_HELPINDEX                     2950
#define PANEL_HELPABOUT                     2960
#define PANEL_TEXTOBJECT                    3000
#define PANEL_GETTINGSTARTED                3100
#define PANEL_CHANGINGCOLORS                3200
#define PANEL_POLYLINES                     3700
#define PANEL_LAYERDIALOG                   3800
#define PANEL_GRADIENT                      3900
#define PANEL_CIRCULARTEXT                  4300
#define PANEL_REGISTRATION                  4400
#define PANEL_QUERYPRINT                    4500
#define PANEL_EXPORTSEL2BMP                 4600
#define PANEL_FORMSIZE                      4900
#define PANEL_LOTUSPICFILE                  5000
#define PANEL_LINEENDS                      5600
#define PANEL_PRINTING                      5900
