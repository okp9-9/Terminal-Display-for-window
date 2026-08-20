#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#include "../includes/getWindow.h"
#include "../includes/nstrUtils.h"

//> ASCII of brightness.
// #define ALL_CHARS "$@B8&WM#*oahkbdpqwmZO0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^\'. "
#define ALL_CHARS " .,_-~+><\'^\"*:;i!lI?(){}[]1|\\/tfjrxnuvczXYUJCLQ0OZmwqpdblhao#MW&8B&@"
int ALL_CHARS_LEN = sizeof(ALL_CHARS) / sizeof(char);

//> Only one is work fine (more than 1 may cause glich).
//> 
#define THREADNUM 1


//> Follow this pattern -> "\033[38;2;RRR;GGG;BBBmC"
//> You can change the format so printing will change (like \033[48;2;RRR;GGG;BBBmC for background color)
#define COLORED_TEXT_PATTERN "\033[38;2;CCC;CCC;CCCmC"
#define COLORED_PATTERN "\033[48;2;CCC;CCC;CCCm "
#define TEXT_PATTERN "C"


typedef enum {
    TEXT,
    COLORED,
    COLORED_TEXT,
} DISPlAY_PATTERN;

static DISPlAY_PATTERN SELECTED_DISPLAY = COLORED_TEXT;
static char* FORMAT_PATTERN = COLORED_TEXT_PATTERN;
// #define FORMAT_PATTERN "C"

//> size for 1 pixel.
// #define SPACE_FOR_PIXEL 1
int SPACE_FOR_PIXEL = sizeof(COLORED_TEXT_PATTERN) / sizeof(char)  - 1;//20

static int OFFSET_X = 5;
static int OFFSET_Y = 12;

typedef struct {
    int x;
    int y;

    int width;
    int height;

    HWND mainWin;
    HBITMAP bitMap;
    HBITMAP oldBitMap;
    HDC scrDC;
    HDC memDC;

    char title[100];

    bool isDesktop;
} WinInfo;



typedef struct {
  BYTE *colors;
  char *mainDes;

  int *maxHeight;
  int *maxWidth;

  int start;
  int end;

} threadD;



void DelWinInfo(WinInfo*, BYTE*);
void GetWinInfo(WinInfo*, char[]);

void ProjectPixels(WinInfo*, BYTE*);

void *SubRender(void*);
char *UpdateTerminal(BYTE*, WinInfo*, pthread_t*, char*);
void createNewConsole();
LRESULT CALLBACK KeyBoardHandle(int, WPARAM, LPARAM);


bool isPlay;

//> Check if target window has title bar will shift the render up to 32 to remove title bar from rendering.
// bool checkTitleTab(HWND *window){
//     LONG_PTR style = GetWindowLongPtr(window, GWL_STYLE);

//     if((style & WS_CAPTION) == WS_CAPTION) return true;

//     return false;
// }


//> Initialize window data.
void GetWinInfo(WinInfo *info, char title[]){

    HWND mainWin;

    bool isDesktop = nstr_compare(title, "DESKTOP"); //> is the whole desktop?
    info->isDesktop = isDesktop;

    //> NOTE : The desktop & specific window rendering is different.

    if(isDesktop) mainWin = GetDesktopWindow();
    else {
        mainWin = FindWindowA(NULL, title);
    }


    // SetWindowPos(mainWin, NULL, 0, 0, 1133, 688, SWP_NOMOVE);
    if(!mainWin) return;

    RECT boundingWin;
    GetWindowRect(mainWin, &boundingWin);

    int width = boundingWin.right - boundingWin.left;
    int height = boundingWin.bottom - boundingWin.top;

    //> Cut the decimal off.
    if(width%OFFSET_X || height%OFFSET_Y){

        width = (int)(width/OFFSET_X) * OFFSET_X;
        height = (int)(height/OFFSET_Y) * OFFSET_Y;

    }

    HDC hScreenDC;
    if(isDesktop) hScreenDC = GetDC(NULL);
    else hScreenDC = GetDC(mainWin);


    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    HBITMAP bitmap = CreateCompatibleBitmap(hScreenDC, width, height);

    //> Keep oldbitmap and then select back when clean (free all).
    HBITMAP Oldbitmap = (HBITMAP)SelectObject(hMemoryDC, bitmap);



    info->scrDC = hScreenDC;
    info->memDC = hMemoryDC;

    info->bitMap = bitmap;
    info->oldBitMap = Oldbitmap;

    info->mainWin = mainWin;

    info->width = width;
    info->height = height;

    // x,y
    

    return;
}


//> Clear window info.
void DelWinInfo(WinInfo *info, BYTE *colors){

    free(colors);

    SelectObject(info->memDC, info->oldBitMap);
    DeleteObject(info->bitMap);
    DeleteDC(info->memDC);
    ReleaseDC(NULL, info->scrDC);

}


//> Get color pixels.
void ProjectPixels(WinInfo *info, BYTE *colors) {
    if(info->isDesktop) {
        BitBlt(info->memDC, 0, 0, info->width, info->height, info->scrDC, 0, 0, SRCCOPY);
    }
    else {
        PrintWindow(info->mainWin, info->memDC, PW_RENDERFULLCONTENT);
        // PrintWindow(win, hMemoryDC, PW_CLIENTONLY);
    }
    SetWindowPos(info->mainWin, HWND_BOTTOM, 0, 0, info->width, info->height, SWP_NOMOVE);

    BITMAPINFO bmi;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (info->width);
    bmi.bmiHeader.biHeight = -(info->height); //> Negative height enforces top-down pixel ordering
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; 
    bmi.bmiHeader.biCompression = BI_RGB;

    //> The DIB_RGB_COLORS can be change. -? DIB_PAL_COLORS
    GetDIBits(info->memDC, info->bitMap, 0, info->height, colors, &bmi, DIB_RGB_COLORS);
}



// void makePixel(char *scr, int starti, char *color){
//     int im = 0;
//     for(int i=0; i<SPACE_FOR_PIXEL; i++){
//         if(FORMAT_PATTERN[i] == 'C'){
//             scr[starti+i] = color[im];
//             im++;
//         }else {
//             scr[starti+i] = FORMAT_PATTERN[i];
//         }
//     }
// }

//> Render for each thread.
void *SubRender(void* args){
    threadD *data = args;
    

    char *RGBC = malloc(sizeof(char) * SPACE_FOR_PIXEL);
    // int *im = malloc(sizeof(int));
    int im = 0;
    for(int ly=(data->start)/ OFFSET_Y; ly<data->end / OFFSET_Y; ly++) {
        int offsety = (ly)*(*data->maxWidth);


        for(int lx=0; lx<(*data->maxWidth) / OFFSET_X; lx++){
            
            int R = data->colors[lx*OFFSET_X *4+2  +(offsety)*OFFSET_Y *4];
            int G = data->colors[lx*OFFSET_X *4+1  +(offsety)*OFFSET_Y *4];
            int B = data->colors[lx*OFFSET_X *4    +(offsety)*OFFSET_Y *4];

            float averg = (R+G+B)/3;
            int charInt = (averg/255) * (ALL_CHARS_LEN-2);


            //> Set order of this follow by PPATTERN.
            //> COLOR TEXT. & COLOR BACKGOUND.
            if(SELECTED_DISPLAY == COLORED_TEXT || SELECTED_DISPLAY == COLORED) {
                RGBC[0] = digit2nstr(R/100);
                RGBC[1] = digit2nstr((R/10)%10);
                RGBC[2] = digit2nstr(R%10);

                RGBC[3] = digit2nstr(G/100);
                RGBC[4] = digit2nstr((G/10)%10);
                RGBC[5] = digit2nstr(G%10);

                RGBC[6] = digit2nstr(B/100);
                RGBC[7] = digit2nstr((B/10)%10);
                RGBC[8] = digit2nstr(B%10);

                RGBC[9] = ALL_CHARS[charInt];
            }
            //> PURE TEXT.
            else if(SELECTED_DISPLAY == TEXT) RGBC[0] = ALL_CHARS[charInt];

            



        //     //> COLOR TEXT & BACKGROUND (Text for "\033[48;2;CCC;CCC;CCCm\033[38;2;CCC;CCC;CCCm@" this pattern).
        // if(SELECTED_DISPLAY == COLORED_TEXT){
        //     RGBC[0] = digit2nstr(R/100);
        //     RGBC[1] = digit2nstr((R/10)%10);
        //     RGBC[2] = digit2nstr(R%10);

        //     RGBC[3] = digit2nstr(G/100);
        //     RGBC[4] = digit2nstr((G/10)%10);
        //     RGBC[5] = digit2nstr(G%10);

        //     RGBC[6] = digit2nstr(B/100);
        //     RGBC[7] = digit2nstr((B/10)%10);
        //     RGBC[8] = digit2nstr(B%10);

        //     RGBC[9] = RGBC[0];
        //     RGBC[10] = RGBC[1];
        //     RGBC[11] = RGBC[2];

        //     RGBC[12] = RGBC[3];
        //     RGBC[13] = RGBC[4];
        //     RGBC[14] = RGBC[5];

        //     RGBC[15] = RGBC[6];
        //     RGBC[16] = RGBC[7];
        //     RGBC[17] = RGBC[8];
        // }


            int offset =  ly*(*data->maxWidth)/OFFSET_X *SPACE_FOR_PIXEL + (lx)*SPACE_FOR_PIXEL ;

            im = 0;
            for(int i=0; i<SPACE_FOR_PIXEL; i++){
                if(FORMAT_PATTERN[i] == 'C'){
                    data->mainDes[offset+i] = RGBC[im];
                    im++;
                }else {
                    data->mainDes[offset+i] = FORMAT_PATTERN[i];
                }
            }



        }

        data->mainDes[data->start/ OFFSET_Y + (ly)*(*data->maxWidth)/OFFSET_X*SPACE_FOR_PIXEL + (SPACE_FOR_PIXEL-1)] = '\n';
        // data->mainDes[data->start/ OFFSET_Y + (ly)*(*data->maxWidth)/OFFSET_X*SPACE_FOR_PIXEL] = '\n';
        
    }

    free(RGBC);
    free(data);
}


//> Send data to threads.
char *UpdateTerminal(BYTE *colors, WinInfo *info, pthread_t* THREADS, char *SCREEN){
    char *Screen = SCREEN;
    pthread_t *threads = THREADS;


    for(int t=0; t<THREADNUM; t++){
        threadD *tData = malloc(sizeof(threadD));
        tData->mainDes = Screen;
        tData->colors = colors;

        tData->maxHeight = &info->height;
        tData->maxWidth = &info->width;

        //> Start Y-axis
        tData->start = t * (info->height)/THREADNUM;
        //> To Y-axis
        tData->end = (t+1) * (info->height)/THREADNUM;

        if(pthread_create(&threads[t], NULL, SubRender, tData)){
            free(tData);
        };

        pthread_join(threads[t], NULL);
        
    }

    //> Convert to default text color (white color).
    const int offsd = ((info->height)/OFFSET_Y) *(info->width/OFFSET_X)*SPACE_FOR_PIXEL;
    Screen[offsd] = '\033';
    Screen[offsd+1] = '[';
    Screen[offsd+2] = '0';
    Screen[offsd+3] = 'm';
    Screen[offsd+4] = '\n';


    if(Screen == NULL){
        return NULL;
    }

    return Screen;
}

//> For open new console (in-case that run in other terminal like IDE console).
void createNewConsole(){
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if( !CreateProcess(NULL,
        "cmd.exe /k main.exe -new",
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE, //> Create new console.
        NULL,
        NULL,
        &si,
        &pi)          
    ) {
        printf( "Create new console failed (%d).\n", GetLastError() );
    }

    return;
}



//> Handle keyboard event (press ESC (VK_ESCAPE) -> stop loop).
LRESULT CALLBACK KeyBoardHandle(int code, WPARAM wParam, LPARAM lParam){
    KBDLLHOOKSTRUCT *keyData = (KBDLLHOOKSTRUCT*)lParam;
    DWORD keyCode = keyData->vkCode; 
    if(keyCode == VK_ESCAPE){
        HWND tWin = GetConsoleWin();
        if(GetForegroundWindow() == tWin){
            // SendMessage(tWin, WM_CLOSE, 0, 0);
            isPlay = false;
        }
    }

    //> For non-blocking the destination window event. (if not have, Destination window won't recieve the event).
    //> KBDLLHOOKSTRUCT event.
    return CallNextHookEx(NULL, code, wParam, lParam);
}


int main(int arg, char **argv){

    //> Create new console?
    if(!argv[1] || !(nstr_compare(argv[1], "-new"))){
        char con;
        printf("You want to open in new console? (y/n) : ");
        nCharInput(&con);

        if(con == 'y'){
            createNewConsole();

            return 0;
        }
    }


    char sd;
    printf("Selected your styles what you want \n c - pure colored \n t - pure text \n a - colored text\n");
    nCharInput(&sd);
    if(!sd){
        printf("Style not found. Set to default : %c\n", sd);
    }


    //> Set style (Default is a);
    switch (sd)
    {
    case 'c':
        FORMAT_PATTERN = COLORED_PATTERN;
        SELECTED_DISPLAY = COLORED;
        break;
    case 'a':
        FORMAT_PATTERN = COLORED_TEXT_PATTERN;
        SELECTED_DISPLAY = COLORED_TEXT;
        break;
    case 't':
        FORMAT_PATTERN = TEXT_PATTERN;
        SELECTED_DISPLAY = TEXT;
        break;
    default:
        break;
    }

    //> Set new space size.
    SPACE_FOR_PIXEL = nstrlen(FORMAT_PATTERN);


    //> Get all show windows (some of them not work. Bruh).
    struct eWinTitle *start_title = (struct eWinTitle*)malloc(sizeof(struct eWinTitle*)); //> First title.
    struct mWinTitle *list_titles = malloc(sizeof(struct mWinTitle)); //> list of titles.
    list_titles->start = start_title; 
    list_titles->end = start_title;
    EnumWindows(HandleGetHWND, (LPARAM)list_titles); //> Gathering by callback func.

    print_winTitles(list_titles);

    //> Input for id window.
    int selected_id;
    printf("Select your window id (blank for entrie desktop): ");
    nIntInput(&selected_id);

    //> Get title from list by id.
    char *mtitle = getWindowTitle(list_titles, selected_id);

    // If not found any window set to DESKTOP.
    if(selected_id == -1){
        mtitle = "DESKTOP";
    }

    printf("You selected %s \n", mtitle);


    //> Input for pixel skip
    printf("Enter the X-axis skip (5) : ");
    nIntInput(&OFFSET_X);

    printf("Enter the Y-axis skip (12): ");
    nIntInput(&OFFSET_Y);
    
    if(OFFSET_X == -1){
        OFFSET_X = 5;
    }
    if(OFFSET_Y == -1){
        OFFSET_Y = 12;
    }
    else if(!OFFSET_X || !OFFSET_Y){
        return 0;
    }


    
    //> Get window info by title.
    WinInfo info;
    GetWinInfo(&info, mtitle);

    //> Clean.
    free_winTitle(list_titles);

    //> Get current console.
    HWND mainConsole = GetConsoleWin();
    if(!mainConsole) return 0;


    //> Attach to hook (handle for Keyboard action).
    HHOOK hKeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, KeyBoardHandle, NULL, 0);

    char *Screen = (char*)malloc((SPACE_FOR_PIXEL) * info.width * info.height * sizeof(char) + (1*info.height * sizeof(char)));
    BYTE *colorsList = (BYTE*)(malloc(info.width * info.height * sizeof(BYTE*)));

    pthread_t *threads = malloc(sizeof(pthread_t) *THREADNUM);


    isPlay = true;
    //> Loop.
    MSG msg;
    while (isPlay)
    {

        //> Catch input event.
        if(PeekMessage(&msg, NULL, 0, 0, 0)){
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        ProjectPixels(&info, colorsList);

        UpdateTerminal(colorsList, &info, threads, Screen);

        //> Clear screen.
        fputs("\033[2J", stdout);
        // fflush(stdout);
        system("cls");
        
        //> Print to screen.
        fputs(Screen, stdout);
        // printf("%s", Screen); //> <-- Slow.


        //> For check that reset text color to default color is right.
        // printf("IF YOU SEE THIS WHITE. it's good");
        // printf("win32 size : %i, %i", info.width, info.height);
    }

    //> Clean data.
    DelWinInfo(&info, colorsList);
    free(Screen);
    free(threads);
    UnhookWindowsHookEx(hKeyboardHook);

    printf("\nPlace any key to continue : ");
    getchar();
    
    //> Create new console then close this console.
    createNewConsole();
    SendMessage(mainConsole, WM_CLOSE, 0, 0);

    return 0;
}

