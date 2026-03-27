/*
 * ComScope - Serial Port Terminal for Embedded Development
 * Copyright (c) 2026 Prakash Das
 *
 * This file is part of ComScope.
 * ComScope is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Author: Prakash Das
 * GitHub: https://github.com/prkshdas/ComScope
 */

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

    /*calculate a centered window: 50 wide, count+6 tall*/
    int win_h = count + 6;
    int win_w = 50;
    int win_y = (LINES - win_h) / 2;
    int win_x = (COLS - win_w) / 2;

    WINDOW *win = newwin(win_h, win_w, win_y, win_x);
    keypad(win, TRUE); /*enable arrow key on this window*/
    wtimeout(win, 100);

    int selected = 0; /*currently highlighted row*/

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
        mvwprintw(win, win_h - 2, 2, "up/down=select   Enter=connect   q=quit");
        wrefresh(win);

        /*handle keypresses*/
        int ch = wgetch(win); /* 300ms timeout prevents hang */

        if (ch == ERR)
        {
            /*Adding temporary ports array*/
            static char new_ports[MAX_PORTS][270];
            int new_count = scan_ports(new_ports, MAX_PORTS);

            int changed = (new_count != count);
            if (!changed)
            {
                for (int i = 0; i < count; i++)
                {
                    if (strcmp(ports[i], new_ports[i]) != 0)
                    {
                        changed = 1;
                        break;
                    }
                }
            }
            if (changed)
            {
                /* Clear the old window from screen completely */
                erase();
                refresh();

                memcpy(ports, new_ports, sizeof(ports));
                count = new_count;

                if (count == 0)
                    selected = 0;
                else if (selected >= count)
                    selected = count - 1;

                delwin(win);
                win_h = (count > 0 ? count : 1) + 6;
                win_y = (LINES - win_h) / 2;
                win = newwin(win_h, win_w, win_y, win_x);
                keypad(win, TRUE);
                wtimeout(win, 100);

                clearok(win, TRUE);
                touchwin(win);
            }
            continue;
        }

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