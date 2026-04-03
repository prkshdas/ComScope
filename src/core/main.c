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
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "portpicker.h"
#include "configpanel.h"
#include "config.h"
#include "port.h"
#include "engine.h"
#include "logger.h"
#include "tui.h"
#include "autobaud.h"

/*
 * Globals exposed to the SIGWINCH handler so it can rebuild the TUI at the
 * new terminal size.  Without this, endwin()+refresh()+clear() tears down
 * ncurses but never recreates the status window or re-draws the pad, leaving
 * only the status bar visible (or a completely blank screen).
 */
static TermConfig *g_cfg = NULL;
static int g_connected = 0;
static int ncurses_initialized = 0;

static void handle_resize(int sig)
{
    (void)sig;
    if (!ncurses_initialized)
        return;

    endwin();
    refresh(); /* re-enters curses mode; LINES and COLS are updated here */
    clear();
    if (g_cfg)
    {
        tui_resize(g_cfg, g_connected);
    }
}

int main(void)
{
    signal(SIGWINCH, handle_resize);
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    ncurses_initialized = 1;

    char *port = NULL;
    int config_done = 0;

    /* Main navigation loop: allows going back from config to port picker */
    while (!config_done)
    {
        /* 1 — pick port */
        port = pick_port();
        if (!port)
        {
            endwin();
            ncurses_initialized = 0;
            printf("quit.\n");
            return 0; /* User pressed q at port picker — exit application */
        }

        /* 2 — configure */
        TermConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        strncpy(cfg.port, port, sizeof(cfg.port) - 1);

        /* --- auto baudrate detection pop-up --- */
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        WINDOW *det_win = newwin(5, 50, (rows - 5) / 2, (cols - 50) / 2);
        box(det_win, 0, 0);
        mvwprintw(det_win, 2, 2, "Auto-detecting baud rate for %s...", port);
        wrefresh(det_win);

        /* Run the detection */
        int detected_baud = detect_baudrate(port);

        /* Clean up the popup */
        delwin(det_win);
        erase();
        refresh();

        /* Map detected baud rate to the config index */
        cfg.baud_index = 4; /* Default to 115200 */
        if (detected_baud == 9600)
            cfg.baud_index = 0;
        else if (detected_baud == 19200)
            cfg.baud_index = 1;
        else if (detected_baud == 38400)
            cfg.baud_index = 2;
        else if (detected_baud == 57600)
            cfg.baud_index = 3;
        else if (detected_baud == 115200)
            cfg.baud_index = 4;
        else if (detected_baud == 230400)
            cfg.baud_index = 5;
        /* -------------------------------- */

        /* Logging disabled by default */
        strncpy(cfg.log_path, "", sizeof(cfg.log_path) - 1);
        cfg.log_enabled = 0; /* Logging OFF by default */

        int config_result = show_config(&cfg);

        if (config_result == 0)
        {
            /* Configuration successful, proceed to open serial port */
            config_done = 1;

            g_cfg = &cfg;
            g_connected = 0;

            /* 3 — open serial port */
            int fd = open_serial(&cfg);
            if (fd < 0)
            {
                endwin();
                ncurses_initialized = 0;
                printf("Failed to open %s\n", cfg.port);
                printf("Tip: sudo usermod -aG dialout $USER\n");
                return 1;
            }

            g_connected = 1;

            /* 4 — run the terminal */
            run_engine(fd, &cfg);

            /* 5 — clean up */
            g_cfg = NULL;
            g_connected = 0;
            close_serial(fd);

            if (logger_active())
            {
                logger_stop();
            }

            endwin();
            ncurses_initialized = 0;
            printf("ComScope closed.\n");
            printf("Thanks for using -- ComScope\n");

            const char *log_dir = logger_get_log_dir();
            if (log_dir != NULL)
            {
                if (cfg.log_enabled || logger_active())
                {
                    printf("Session logs saved to: %s\n", log_dir);
                }
            }
        }
        else if (config_result < 0)
        {
            /* User pressed q to go back — loop continues and returns to port picker */
            /* No action needed; the while loop will run again */
            continue;
        }
    }

    return 0;
}