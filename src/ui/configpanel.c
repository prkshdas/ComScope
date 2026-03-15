#include <ncurses.h>
#include <string.h>
#include "configpanel.h"
#include "config.h"

/*baud rate options*/
static const char *baud_labels[] = {

    "9600",
    "19200",
    "38400",
    "57600",
    "115200",
    "230400"};

static const speed_t baud_values[] = {

    B9600,
    B19200,
    B38400,
    B57600,
    B115200,
    B230400};

#define BAUD_COUNT 6

int show_config(TermConfig *cfg)
{
    int win_h = 16;
    int win_w = 44;
    int win_y = (LINES - win_h) / 2;
    int win_x = (COLS - win_x) / 2;

    WINDOW *win = newwin(win_h, win_w, win_y, win_x);
    keypad(win, TRUE);

    int baud_sel = 4; /*defauld index - 115200*/
    int data_sel = 0; /*0 = 8bit*/
    int par_sel = 0;  /*0=None*/
    int stop_sel = 0; /*0=1bit*/

    /*curson is on row 0=baud, 1=data, 2=parity, 3=stop*/
    int row = 0;

    while (1)
    {
        werase(win);
        box(win, 0, 0);

        mvwprintw(win, 1, 2, "ComScope -- Connection Settings");
        mvwprintw(win, 2, 2, "Port: %s", cfg->port);

        /* ── Baud rate row ── */
        if (row == 0)
            wattron(win, A_REVERSE);
        mvwprintw(win, 4, 2, "Baud rate  : %-8s  (up/down)", baud_labels[baud_sel]);
        if (row == 0)
            wattroff(win, A_REVERSE);

        /* ── Data bits row ── */
        if (row == 1)
            wattron(win, A_REVERSE);
        mvwprintw(win, 6, 2, "Data bits  : 8");
        if (row == 1)
            wattroff(win, A_REVERSE);

        /* ── Parity row ── */
        if (row == 2)
            wattron(win, A_REVERSE);
        mvwprintw(win, 8, 2, "Parity     : None");
        if (row == 2)
            wattroff(win, A_REVERSE);

        /* ── Stop bits row ── */
        if (row == 3)
            wattron(win, A_REVERSE);
        mvwprintw(win, 10, 2, "Stop bits  : 1");
        if (row == 3)
            wattroff(win, A_REVERSE);

        mvwprintw(win, 13, 2, "Tab=next row  left/right=change  Enter=connect  q=back");
        wrefresh(win);

        int ch = wgetch(win);

        if (ch == '\t')
        {
            /* Tab moves between rows */
            row = (row + 1) % 4;
        }
        else if (ch == KEY_UP || ch == KEY_LEFT)
        {
            if (row == 0 && baud_sel > 0)
                baud_sel--;
        }
        else if (ch == KEY_DOWN || ch == KEY_RIGHT)
        {
            if (row == 0 && baud_sel < BAUD_COUNT - 1)
                baud_sel++;
        }
        else if (ch == KEY_ENTER || ch == '\n' || ch == '\r')
        {
            /* save into config struct and return */
            cfg->baud = baud_values[baud_sel];
            cfg->databits = 8;
            cfg->parity = 0;
            cfg->stopbits = 1;
            delwin(win);
            return 0;
        }
        else if (ch == 'q' || ch == 'Q')
        {
            delwin(win);
            return -1; /* go back to port picker */
        }
    }
}