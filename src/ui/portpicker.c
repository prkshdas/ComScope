#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "scanner.h"
#include "portpicker.h"

char *pick_port(void)
{
    /*scan for ports*/
    static char ports[MAX_PORTS][270];
    int count = scan_ports(ports, MAX_PORTS);

    /*calculate a centered window: 40 wide, count+6 tall*/
    int win_h = count + 6;
    int win_w = 40;
    int win_y = (LINES - win_h) / 2;
    int win_x = (COLS - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, win_y, win_x);
    keypad(win, TRUE); /*enable arrow key on this window*/

    int selected = 0; /*cuurently highligted row*/

    while (1)
    {
        /*redraw the window from scratch each loop*/
        werase(win);
        box(win, 0, 0); /*draw border*/

        /*title*/
        mvwprintw(win, 1, 2, "ComScope -- Select a port");

        if (count == 0)
        {
            mvwprintw(win, 3, 2, "No ports found");
        }
        else
        {
            /*port list*/
            for (int i = 0; i < count; i++)
            {
                if (i == selected)
                    wattron(win, A_REVERSE); /*highlight selected*/
                mvwprintw(win, 3 + i, 2, " %s", ports[i]);

                if (i == selected)
                    wattroff(win, A_REVERSE); /*turn off highlight*/
            }
        }

        /*hint bar at the bottom*/
        mvwprintw(win, win_h - 2, 2, "up/down    Enter=connect   q=quit");
        wrefresh(win);

        /*handle keypresses*/
        int ch = wgetch(win);

        if (ch == KEY_UP)
        {
            if (selected > 0)
                selected--;
        }
        else if (ch == KEY_DOWN)
        {
            if (selected < count - 1)
                selected++;
        }
        else if (ch == KEY_ENTER || ch == '\n' || ch == '\r')
        {
            if (count > 0)
            {
                delwin(win);
                return ports[selected]; /*return selected port path*/
            }
        }
        else if (ch == 'q' || ch == 'Q')
        {
            delwin(win);
            return NULL; /*quit*/
        }
    }
}