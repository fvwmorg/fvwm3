#ifndef _DEBUG_
#define _DEBUG_

#ifdef DEBUG
void DB_WI_WINDOWS(char *label, FvwmWindow *fw);
void DB_WI_SUBWINS(char *label, FvwmWindow *fw);
void DB_WI_FRAMEWINS(char *label, FvwmWindow *fw);
void DB_WI_BUTTONWINS(char *label, FvwmWindow *fw);
void DB_WI_FRAME(char *label, FvwmWindow *fw);
void DB_WI_ICON(char *label, FvwmWindow *fw);
void DB_WI_SIZEHINTS(char *label, FvwmWindow *fw);
void DB_WI_TITLE(char *label, FvwmWindow *fw);
void DB_WI_BORDER(char *label, FvwmWindow *fw);
void DB_WI_XWINATTR(char *label, FvwmWindow *fw);
void DB_WI_ALL(char *label, FvwmWindow *fw);
#else
#define DB_WI_WINDOWS(x,y)
#define DB_WI_SUBWINS(x,y)
#define DB_WI_FRAMEWINS(x,y)
#define DB_WI_BUTTONWINS(x,y)
#define DB_WI_FRAME(x,y)
#define DB_WI_ICON(x,y)
#define DB_WI_SIZEHINTS(x,y)
#define DB_WI_TITLE(x,y)
#define DB_WI_BORDER(x,y)
#define DB_WI_XWINATTR(x,y)
#define DB_WI_ALL(x,y)
#endif

#endif /* _DEBUG_ */
