#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "../includes/getWindow.h"

//> Put the title (similar to link-list)
void push_winTitle(struct mWinTitle *win, char *title, DWORD id){

    struct eWinTitle *win_next = malloc(sizeof(struct eWinTitle));

    win->end->title = title;
    win->end->next = win_next;
    win->end->id = id;

    win->end = win_next;
    win_next->title = NULL;
    win_next->id = -1;
    win_next->next = NULL;
}

//> Loop get through all windows.
BOOL CALLBACK HandleGetHWND(HWND hwnd, LPARAM lParam){

    int length = GetWindowTextLengthW(hwnd);


    if(length) {
        char *title = malloc(sizeof(char) * length + 1);
        GetWindowTextA(hwnd, title, length+1);

        if(IsWindowVisible(hwnd)) {
            struct mWinTitle *list_winTitle = (struct mWinTitle*)(lParam);
            push_winTitle(list_winTitle, title, GetWindowThreadProcessId(hwnd, NULL));
        }
    }
    return TRUE;
}


//> Print all title (first title -> next title -> ...).
void print_winTitles(struct mWinTitle *win_list){
    int idSpace = 10;
    struct eWinTitle *cur_win = win_list->start;

    printf("    ids    |              Window Titles\n");
    printf("           |\n");

    while(cur_win){
        if(cur_win->id == -1) break;
        int idLen = log10(cur_win->id);
        int leftSpace = idSpace >= idLen ? idSpace-idLen : idLen ;
        
        for(size_t i=0; i<(int)(leftSpace/2); i++){
            printf(" ", i);
        }
        printf("%i", cur_win->id);
        for(size_t i=0; i<(int)(leftSpace-(leftSpace/2)); i++){
            printf(" ", i);
        }
        printf("--> %s\n", cur_win->title);

        cur_win = cur_win->next;
    }
}

//> Clean.
void free_winTitle(struct mWinTitle *win_list){
    struct eWinTitle *cwin = win_list->start;

    struct eWinTitle *rcwin;

    while(cwin){
        rcwin = cwin->next;
        free(cwin->title);
        free(cwin);
        cwin = NULL;
        cwin = rcwin;
    }

    free(win_list);
}


//> Loop through link-list.
char *getWindowTitle(struct mWinTitle *win_list, int id){
    struct eWinTitle *cwin = win_list->start;

    while(cwin){
        if(cwin->id == id){
            return cwin->title;
        }
        cwin = cwin->next;
    }

    return NULL;
}


//> Get current console.
HWND GetConsoleWin(){
    char *t = malloc(sizeof(char) * 1024);
    GetConsoleTitle(t, 1024);

    HWND terminalWin = FindWindowA(NULL, t);
    free(t);
    return terminalWin;
}