#include <ncurses.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "portpicker.h"
#include "configpanel.h"
#include "config.h"
#include "port.h"

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

    /* 1. pick a port*/
    char *port = pick_port();
    if (port == NULL)
    {
        endwin(); /*restore terminal before printf*/
        printf("No ports selected\n");
        return 0;
    }

    /*2. configure connection*/

    TermConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.port, port, sizeof(cfg.port) - 1);

    if (show_config(&cfg) < 0)
    {
        endwin();
        printf("Cancelled.\n");
        return 0;
    }
    endwin();

    /*3. open serial port*/
    int fd = open_serial(&cfg);
    if (fd < 0)
    {
        printf("Failed to open %s\n", cfg.port);
        return 1;
    }

    /*confirmation*/
    printf("Port opened successfully!\n");
    printf("Port    : %s\n", cfg.port);
    printf("FD      : %d\n", fd);
    printf("Baud    : %d\n", (int)cfg.baud);

    close_serial(fd);
    return 0;
}