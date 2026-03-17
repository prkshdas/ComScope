#include <ncurses.h>
#include <string.h>
#include "tui.h"

#define PAD_ROWS 20000
#define PAD_COLS 1024

WINDOW *pad_win;
WINDOW *status_win;

static int pad_row = 0;
static int pad_pos = 0;
static int auto_scroll = 1;
static int input_timeout = 50;

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

void tui_init(TermConfig *cfg)
{
    (void)cfg;

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors())
    {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_WHITE, -1);
        init_pair(2, COLOR_GREEN, -1);
        init_pair(3, COLOR_CYAN, -1);
    }

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    pad_win = newpad(PAD_ROWS, PAD_COLS);
    scrollok(pad_win, TRUE);

    if (has_colors())
    {
        wattron(pad_win, COLOR_PAIR(1));
    }

    status_win = newwin(1, cols, rows - 1, 0);
    keypad(status_win, TRUE);
    wrefresh(status_win);

    timeout(input_timeout);
}

void tui_destroy(void)
{
    if (status_win)
        delwin(status_win);
    if (pad_win)
        delwin(pad_win);
    endwin();
}

int tui_get_char(void)
{
    return getch();
}

void tui_write(const char *buf, int len)
{
    if (!pad_win)
        return;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int vis = visible_rows();

    if (has_colors())
    {
        wattron(pad_win, COLOR_PAIR(1));
    }

    for (int i = 0; i < len; ++i)
    {
        char c = buf[i];
        if (c == '\r')
        {
            continue;
        }
        waddch(pad_win, c);
    }

    int y, x;
    getyx(pad_win, y, x);
    (void)x;
    if (y < 0)
        y = 0;
    pad_row = y;

    if (auto_scroll)
    {
        if (pad_row >= vis)
        {
            pad_pos = pad_row - vis + 1;
            if (pad_pos < 0)
                pad_pos = 0;
        }
        else
        {
            pad_pos = 0;
        }
    }
    else
    {
        if (pad_pos > pad_row)
            pad_pos = pad_row;
    }

    int last_display_row = (rows >= 2) ? (rows - 2) : 0;
    prefresh(pad_win, pad_pos, 0, 0, 0, last_display_row, cols - 1);
}

void tui_scroll(int direction)
{
    if (!pad_win)
        return;

    int rows = getmaxy(stdscr);

    auto_scroll = 0;

    pad_pos += direction * 3;

    if (pad_pos < 0)
        pad_pos = 0;
    if (pad_pos > pad_row)
        pad_pos = pad_row;

    int last_display_row = (rows >= 2) ? (rows - 2) : 0;
    prefresh(pad_win, pad_pos, 0, 0, 0, last_display_row, COLS - 1);
}

void tui_update_status(TermConfig *cfg, int connected)
{
    if (!status_win) return;

    werase(status_win);
    wattron(status_win, A_REVERSE);

    /* Display: PORT | BAUD | CONNECTION_STATUS | LOG_STATUS | MENU_HINT */
    mvwprintw(status_win, 0, 0,
              "%s | %d baud | [%s] %s | Ctrl+A = menu",
              cfg->port,
              cfg->baud,
              connected ? "CONNECTED" : "DISCONNECTED",
              cfg->log_enabled ? "| [LOG]" : "");

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

    tui_update_status(cfg, connected);

    int last_display_row = (rows >= 2) ? (rows - 2) : 0;
    prefresh(pad_win, pad_pos, 0, 0, 0, last_display_row, cols - 1);
}