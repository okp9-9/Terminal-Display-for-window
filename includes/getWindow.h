#ifndef GETWINDOW_H
#define GETWINDOW_H


struct DWORD;
struct HWND;
struct LPARAM;
struct BOOL;

struct eWinTitle {
    char *title;
    DWORD id;
    struct eWinTitle *next;
};
struct mWinTitle {
    struct eWinTitle *start;
    struct eWinTitle *end;
};

void push_winTitle(struct mWinTitle*, char*, DWORD);
BOOL CALLBACK HandleGetHWND(HWND, LPARAM);
void print_winTitles(struct mWinTitle*);
void free_winTitle(struct mWinTitle*);
char *getWindowTitle(struct mWinTitle*, int);
HWND GetConsoleWin();

#endif