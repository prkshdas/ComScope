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
#include <time.h>
#include "tui.h"

#define SCROLLBACK_LINES 1000 // Reduced from 20000 to 1000 (saves memory, faster)
#define LINE_LENGTH 512       // Max chars per line
#define TAB_WIDTH 4

WINDOW *status_win = NULL;

/* Circular scrollback buffer */
typedef struct
{
    char **lines;         // Array of line buffers
    int head;             // Index of newest line
    int count;            // Number of valid lines (0 to SCROLLBACK_LINES)
    int cols;             // Terminal columns
    int current_line_pos; // Current cursor position in current line
} Scrollback;

static Scrollback sb = {0};
static int auto_scroll = 1;
static int scroll_offset = 0; // For Page Up/Down scrolling
static int input_timeout = 50;
static struct timespec last_refresh = {0, 0};
static int pad_dirty = 0;
static int tui_paused = 0;

/* Throttle display to ~60Hz (16ms) */
#define REFRESH_INTERVAL_NS (16 * 1000000LL)

static inline long long timespec_diff_ns(struct timespec *a, struct timespec *b)
{
    return (a->tv_sec - b->tv_sec) * 1000000000LL + (a->tv_nsec - b->tv_nsec);
}

static int visible_rows(void) __attribute__((unused)); // avoid compilation warning
static int visible_rows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)cols;
    return (rows > 1) ? (rows - 1) : 1;
}

void tui_set_timeout(int timeout_ms)
{
    input_timeout = timeout_ms;
    timeout(timeout_ms);
}

void tui_set_paused(int paused)
{
    tui_paused = paused;
}

void tui_init(TermConfig *cfg)
{
    (void)cfg;
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    /* Allocate circular scrollback buffer */
    sb.lines = calloc(SCROLLBACK_LINES, sizeof(char *));
    for (int i = 0; i < SCROLLBACK_LINES; i++)
    {
        sb.lines[i] = calloc(1, LINE_LENGTH);
        sb.lines[i][0] = '\0';
    }
    sb.head = 0;
    sb.count = 0;
    sb.cols = cols;
    sb.current_line_pos = 0;
    scroll_offset = 0;

    status_win = newwin(1, cols, rows - 1, 0);
    keypad(status_win, TRUE);

    clock_gettime(CLOCK_MONOTONIC, &last_refresh);
    timeout(input_timeout);

    /* Initial draw */
    clear();
    refresh();
    tui_update_status(cfg, 1);
}

void tui_destroy(void)
{
    if (status_win)
    {
        delwin(status_win);
        status_win = NULL;
    }

    /* Free scrollback buffer */
    if (sb.lines)
    {
        for (int i = 0; i < SCROLLBACK_LINES; i++)
        {
            free(sb.lines[i]);
        }
        free(sb.lines);
        sb.lines = NULL;
    }
}

int tui_get_char(void)
{
    return getch();
}

/* Internal: Ensure current line exists and append char */
static void append_to_line(char c)
{
    char *line = sb.lines[sb.head];

    if (sb.current_line_pos < sb.cols - 1 && sb.current_line_pos < LINE_LENGTH - 1)
    {
        line[sb.current_line_pos++] = c;
        line[sb.current_line_pos] = '\0';
    }
}

/* Internal: Advance to new line (circular) */
static void advance_line(void)
{
    sb.head = (sb.head + 1) % SCROLLBACK_LINES;
    sb.current_line_pos = 0;
    sb.lines[sb.head][0] = '\0';

    if (sb.count < SCROLLBACK_LINES)
    {
        sb.count++;
    }

    /* If auto-scrolling, adjust offset */
    if (auto_scroll)
    {
        scroll_offset = 0;
    }
}

void tui_write(const char *buf, int len)
{
    if (len <= 0 || !sb.lines)
        return;

    for (int i = 0; i < len; i++)
    {
        unsigned char c = (unsigned char)buf[i];

        if (c == '\r')
        {
            continue; // Ignore CR
        }
        else if (c == '\n')
        {
            advance_line();
        }
        else if (c == '\b' || c == 0x7F)
        {
            // Backspace
            if (sb.current_line_pos > 0)
            {
                sb.current_line_pos--;
                sb.lines[sb.head][sb.current_line_pos] = '\0';
            }
        }
        else if (c == '\t')
        {
            for (int j = 0; j < TAB_WIDTH; j++)
            {
                append_to_line(' ');
            }
        }
        else if (c >= 32 && c != 127)
        {
            // Printable ASCII
            append_to_line((char)c);
        }
        // Ignore non-printable control chars
    }

    pad_dirty = 1;

    /* Throttle screen refresh to ~60Hz */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (timespec_diff_ns(&now, &last_refresh) > REFRESH_INTERVAL_NS)
    {
        tui_refresh();
    }
}

/* Force screen redraw */
void tui_refresh(void)
{
    if (!pad_dirty || !sb.lines)
        return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int display_rows = rows - 1; // Reserve bottom row for status
    (void)cols;                  // Suppress unused warning

    clear();

    int start_line;
    if (auto_scroll || scroll_offset == 0)
    {
        /* Show newest lines at bottom */
        start_line = (sb.head - (display_rows - 1) + SCROLLBACK_LINES) % SCROLLBACK_LINES;
        if (sb.count < display_rows)
        {
            start_line = 0;
        }
        scroll_offset = 0;
    }
    else
    {
        /* Scrolling mode */
        start_line = (sb.head - (display_rows - 1) - scroll_offset + SCROLLBACK_LINES * 2) % SCROLLBACK_LINES;
    }

    for (int row = 0; row < display_rows; row++)
    {
        int line_idx = (start_line + row) % SCROLLBACK_LINES;

        // Only draw if this line has valid data
        if (row < sb.count || sb.count == SCROLLBACK_LINES)
        {
            mvprintw(row, 0, "%s", sb.lines[line_idx]);
        }
    }

    refresh();
    pad_dirty = 0;

    clock_gettime(CLOCK_MONOTONIC, &last_refresh);
}

void tui_scroll(int direction)
{
    int rows = getmaxy(stdscr);
    int page_size = rows / 2; // Half page scroll
    if (page_size < 1)
        page_size = 1;
    tui_scroll_lines((direction > 0) ? page_size : -page_size);
}

void tui_scroll_lines(int lines)
{
    int rows = getmaxy(stdscr);
    int max_offset = sb.count - (rows - 1);
    if (max_offset < 0)
        max_offset = 0;

    auto_scroll = 0; // Disable auto-scroll when user scrolls
    scroll_offset += lines;
    if (scroll_offset > max_offset)
        scroll_offset = max_offset;
    if (scroll_offset <= 0)
    {
        scroll_offset = 0;
        auto_scroll = 1; // Re-enable auto-scroll at bottom
    }

    pad_dirty = 1;
    tui_refresh();
    tui_update_status(NULL, 1); // Refresh status to show scroll position
}

void tui_update_status(TermConfig *cfg, int connected)
{
    if (!status_win || !cfg)
        return;

    werase(status_win);
    wattron(status_win, A_REVERSE);

    int baud_rates[] = {9600, 19200, 38400, 57600, 115200, 230400};

    const char *scroll_indicator = "";
    if (!auto_scroll)
    {
        scroll_indicator = "| [SCROLLING] ";
    }

    const char *pause_indicator = "";
    if (tui_paused)
    {
        pause_indicator = "| [PAUSED] ";
    }

    mvwprintw(status_win, 0, 0,
              "%s | %d baud | [%s] %s%s%s| Ctrl+A = menu",
              cfg->port,
              baud_rates[cfg->baud_index],
              connected ? "CONNECTED" : "DISCONNECTED",
              cfg->log_enabled ? "| [LOGGING] " : "",
              pause_indicator,
              scroll_indicator);

    wattroff(status_win, A_REVERSE);
    wrefresh(status_win);
}

void tui_resize(TermConfig *cfg, int connected)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (status_win)
    {
        wresize(status_win, 1, cols);
        mvwin(status_win, rows - 1, 0);
    }

    sb.cols = cols;

    tui_update_status(cfg, connected);
    tui_refresh();
}