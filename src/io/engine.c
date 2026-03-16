#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "engine.h"
#include "tui.h"
#include "config.h"
#include "logger.h"

#define BUF_SIZE 512
#define CMD_ESCAPE 0x01   /* Ctrl+A */

static void handle_command_mode(TermConfig *cfg, int serial_fd)
{
    /* Show menu in status via tui_update_status + a short prompt */
    tui_update_status(cfg, 1);

    /* draw a small inline prompt on status line */
    mvwprintw(status_win, 0, 1, "CMD: l=toggle log   q=quit   Esc=cancel");
    wrefresh(status_win);

    int ch = tui_get_char();

    if (ch == 'l' || ch == 'L') {
        if (!logger_active()) {
            if (logger_start(cfg->log_path) == 0)
                cfg->log_enabled = 1;
        } else {
            logger_stop();
            cfg->log_enabled = 0;
        }
    }
    else if (ch == 'q' || ch == 'Q') {
        ungetch('q');
        return;
    }

    /* refresh status */
    tui_update_status(cfg, 1);
}

void run_engine(int serial_fd, TermConfig *cfg)
{
    tui_init(cfg);
    tui_update_status(cfg, 1);

    struct pollfd fds[2];
    char buf[BUF_SIZE];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = serial_fd;
    fds[1].events = POLLIN | POLLERR | POLLHUP;

    while (1) {
        /* 100 ms timeout for responsive UI and quick serial data polling */
        int ret = poll(fds, 2, 100);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (ret == 0) {
            /* timeout — refresh status/pad in case something changed */
            tui_update_status(cfg, 1);
        }

        /* keyboard input */
        if (fds[0].revents & POLLIN) {
            int ch = tui_get_char();

            if (ch == CMD_ESCAPE) {
                handle_command_mode(cfg, serial_fd);

                int next = tui_get_char();
                if (next == 'q' || next == 'Q') break;
                ungetch(next);
            }
            else if (ch == KEY_PPAGE) {
                tui_scroll(1);
            }
            else if (ch == KEY_NPAGE) {
                tui_scroll(-1);
            }
            else if (ch == 'q') {
                break;
            }
            else {
                char c = (char)ch;
                ssize_t w = write(serial_fd, &c, 1);
                if (w < 0) {
                    /* log write error for debugging */
                    perror("serial write failed");
                }
            }
        }

        /* serial input */
        if (fds[1].revents & POLLIN) {
            ssize_t n = read(serial_fd, buf, sizeof(buf));
            if (n > 0) {
                /* send to UI and logger */
                tui_write(buf, (int)n);
                if (cfg->log_enabled) logger_write(buf, (int)n);
            } else if (n == 0) {
                /* 0 bytes = hangup/closed */
                tui_update_status(cfg, 0);
                break;
            } else {
                /* read error */
                if (errno == EINTR) continue;
                perror("serial read failed");
                break;
            }
        }

        if (fds[1].revents & (POLLHUP | POLLERR)) {
            tui_update_status(cfg, 0);
            break;
        }
    }

    tui_destroy();
}