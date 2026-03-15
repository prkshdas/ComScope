#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "portpicker.h"

static void handle_resize(int sig)
{
    (void)sig; /*suppress unused warning*/
    endwin();  /*let nucurses release the screen*/
    refresh(); /*reinitialise with new dimensions*/
    clear();   /*clear stale content*/
}

int main(void)
{
    signal(SIGWINCH, handle_resize); /*register handler before initscr*/
    initscr();                       /*start ncurses*/
    cbreak();                        /*pass kewpress immediately*/
    noecho();                        /*don't echo typed characters*/
    keypad(stdscr, TRUE);            /*enable arrow keys*/
    curs_set(0);                     /*hide cursor*/

    char *chosen = pick_port();

    endwin(); /*restore terminal before printf*/

    if (chosen == NULL)
    {
        printf("No ports selected\n");
    }
    else
    {
        printf("Selected port: %s\n", chosen);
    }
    return 0;
}