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

/*
 * Globals exposed to the SIGWINCH handler so it can rebuild the TUI at the
 * new terminal size.  Without this, endwin()+refresh()+clear() tears down
 * ncurses but never recreates the status window or re-draws the pad, leaving
 * only the status bar visible (or a completely blank screen).
 */
static TermConfig *g_cfg = NULL;
static int g_connected = 0;

static void handle_resize(int sig)
{
    (void)sig;
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

    /* 1 — pick port */
    char *port = pick_port();
    if (!port)
    {
        endwin();
        printf("quit.\n");
        return 0;
    }

    /* 2 — configure */
    TermConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.port, port, sizeof(cfg.port) - 1);

    strncpy(cfg.log_path, "session.txt", sizeof(cfg.log_path) - 1);
    cfg.log_enabled = 0;

    if (show_config(&cfg) < 0)
    {
        endwin();
        printf("Cancelled.\n");
        return 0;
    }

    g_cfg = &cfg;
    g_connected = 0;

    /* 3 — open serial port */
    int fd = open_serial(&cfg);
    if (fd < 0)
    {
        endwin();
        printf("Failed to open %s\n", cfg.port);
        printf("Tip: sudo usermod -aG dialout $USER\n");
        return 1;
    }

    g_connected = 1;

    /* 4 — run the terminal */
    run_engine(fd, &cfg);

    /* 5 — clean up */
    g_cfg = NULL;
    close_serial(fd);

    if (logger_active())
    {
        logger_stop();
    }

    endwin();
    printf("ComScope closed.\n");
    return 0;
}
