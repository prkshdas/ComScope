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

#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include "engine.h"
#include "tui.h"
#include "config.h"
#include "logger.h"
#include "ring_buffer.h"

#define RING_BUF_SIZE (128 * 1024) // 128KB ring buffer (power of 2)
#define SERIAL_OUT_BUF_SIZE 1024   // 1KB output buffer
#define MAX_READ_PER_CYCLE 4096    // Max 4KB per poll cycle
#define DISPLAY_BATCH_SIZE 2048    // Batch display writes
#define BUF_SIZE 512               // For hex mode formatting

#define FORMAT_LINE_READ_CYCLE_MULTIPLIER 2
#define MAX_FORMAT_LINE_BYTES (MAX_READ_PER_CYCLE * FORMAT_LINE_READ_CYCLE_MULTIPLIER) // Keep enough room when a payload line spans multiple reads
#define HEX_ASCII_OUT_MULTIPLIER 4                                                     // "HH " + optional printable ASCII per byte
#define HEX_ASCII_OUT_PADDING 8                                                        // Separator + trailing newline + safety margin
#define HEX_ASCII_LINE_BUF_SIZE ((MAX_FORMAT_LINE_BYTES * HEX_ASCII_OUT_MULTIPLIER) + HEX_ASCII_OUT_PADDING)

static ring_buff_t g_ring_buf;
static uint8_t g_ring_buffer_storage[RING_BUF_SIZE];
static char g_serial_out_buf[SERIAL_OUT_BUF_SIZE];
static int g_serial_out_len = 0;
static uint8_t g_display_batch[DISPLAY_BATCH_SIZE];
static int g_display_batch_len = 0;
static struct timespec last_display_update = {0, 0};

static uint8_t g_hex_line[MAX_FORMAT_LINE_BYTES];
static int g_hex_line_len = 0;
static uint8_t g_hex_ascii_line[MAX_FORMAT_LINE_BYTES];
static int g_hex_ascii_line_len = 0;

static int g_paused = 0; // Pause state: 0 = running, 1 = paused

#define CMD_ESCAPE 0x01 // Ctrl+A

typedef enum
{
    DISPLAY_MODE_RAW = 0,
    DISPLAY_MODE_HEX,
    DISPLAY_MODE_HEX_ASCII
} display_mode_t;

/* Toggle pause/resume state - can be called from main loop */
static void toggle_pause(void)
{
    g_paused = !g_paused;
}

static inline long long timespec_diff_ms(struct timespec *a, struct timespec *b)
{
    return (a->tv_sec - b->tv_sec) * 1000LL + (a->tv_nsec - b->tv_nsec) / 1000000LL;
}

static int flush_serial_output(int fd)
{
    if (g_serial_out_len == 0)
        return 0;

    ssize_t written = write(fd, g_serial_out_buf, g_serial_out_len);

    if (written < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }

    if ((size_t)written < (size_t)g_serial_out_len)
    {
        memmove(g_serial_out_buf, g_serial_out_buf + written, g_serial_out_len - written);
        g_serial_out_len -= (int)written;
    }
    else
    {
        g_serial_out_len = 0;
    }

    return (int)written;
}

static void queue_serial_output(int fd, char c)
{
    if (g_serial_out_len < SERIAL_OUT_BUF_SIZE)
    {
        g_serial_out_buf[g_serial_out_len++] = c;
    }
    flush_serial_output(fd);
}

static int append_hex_line(const uint8_t *line, int len, char *out, int out_size, int pos)
{
    for (int i = 0; i < len && pos < out_size - 4; i++)
    {
        pos += snprintf(out + pos, out_size - pos, "%02X ", line[i]);
    }
    if (pos < out_size - 1)
    {
        out[pos++] = '\n';
    }
    return pos;
}

static int flush_hex_line_to_output(char *out, int out_size, int pos)
{
    if (g_hex_line_len == 0)
    {
        return pos;
    }
    pos = append_hex_line(g_hex_line, g_hex_line_len, out, out_size, pos);
    g_hex_line_len = 0;
    return pos;
}

static void format_hex(const char *in, int len, char *out, int out_size)
{
    int pos = 0;

    for (int i = 0; i < len && pos < out_size - 4; i++)
    {
        if (g_hex_line_len == MAX_FORMAT_LINE_BYTES)
        {
            pos = flush_hex_line_to_output(out, out_size, pos);
        }

        g_hex_line[g_hex_line_len++] = (uint8_t)in[i];

        if (in[i] == '\r' || in[i] == '\n' || g_hex_line_len == MAX_FORMAT_LINE_BYTES)
        {
            pos = flush_hex_line_to_output(out, out_size, pos);
        }
    }

    out[pos] = '\0';
}

static int append_hex_ascii_line(const uint8_t *line, int len, char *out, int out_size, int pos)
{
    for (int i = 0; i < len && pos < out_size - 4; i++)
    {
        pos += snprintf(out + pos, out_size - pos, "%02X ", line[i]);
    }

    if (pos < out_size - 2)
    {
        // Use a tab so HEX and ASCII columns stay visually separated like RAW line-based output.
        out[pos++] = '\t';
    }

    for (int i = 0; i < len && pos < out_size - 2; i++)
    {
        unsigned char c = line[i];
        // Newline bytes are represented in the HEX column and should not render as ASCII glyphs.
        if (c == '\r' || c == '\n')
        {
            continue;
        }
        out[pos++] = isprint(c) ? (char)c : '.';
    }

    if (pos < out_size - 1)
    {
        out[pos++] = '\n';
    }

    return pos;
}

static int flush_hex_ascii_line_to_output(char *out, int out_size, int pos)
{
    if (g_hex_ascii_line_len == 0)
    {
        return pos;
    }
    pos = append_hex_ascii_line(g_hex_ascii_line, g_hex_ascii_line_len, out, out_size, pos);
    g_hex_ascii_line_len = 0;
    return pos;
}

static void format_hex_ascii(const char *in, int len, char *out, int out_size)
{
    int pos = 0;

    for (int i = 0; i < len && pos < out_size - 1; i++)
    {
        if (g_hex_ascii_line_len == MAX_FORMAT_LINE_BYTES)
        {
            pos = flush_hex_ascii_line_to_output(out, out_size, pos);
        }

        g_hex_ascii_line[g_hex_ascii_line_len++] = (uint8_t)in[i];

        if (in[i] == '\r' || in[i] == '\n' || g_hex_ascii_line_len == MAX_FORMAT_LINE_BYTES)
        {
            pos = flush_hex_ascii_line_to_output(out, out_size, pos);
        }
    }

    out[pos] = '\0';
}

static void flush_pending_format(TermConfig *cfg, display_mode_t display_mode)
{
    if (g_paused)
        return;

    if (display_mode == DISPLAY_MODE_HEX && g_hex_line_len > 0)
    {
        char hex_buf[HEX_ASCII_LINE_BUF_SIZE];
        int pos = flush_hex_line_to_output(hex_buf, (int)sizeof(hex_buf), 0);
        tui_write(hex_buf, pos);
        if (cfg->log_enabled)
        {
            logger_write(hex_buf, pos);
        }
        g_hex_line_len = 0;
    }
    else if (display_mode == DISPLAY_MODE_HEX_ASCII && g_hex_ascii_line_len > 0)
    {
        char hex_ascii_buf[HEX_ASCII_LINE_BUF_SIZE];
        int len = flush_hex_ascii_line_to_output(hex_ascii_buf, (int)sizeof(hex_ascii_buf), 0);
        if (len > 0)
        {
            tui_write(hex_ascii_buf, len);
            if (cfg->log_enabled)
            {
                logger_write(hex_ascii_buf, len);
            }
        }
        g_hex_ascii_line_len = 0;
    }
}

static void process_serial_input(int serial_fd, display_mode_t display_mode)
{
    uint8_t *write_ptr;
    size_t available;
    ssize_t n;
    int total_read = 0;

    // Drain kernel buffer using zero-copy ring buffer interface
    while (total_read < MAX_READ_PER_CYCLE)
    {
        available = ring_buff_get_write_ptr(&g_ring_buf, &write_ptr);
        if (available == 0)
            break; // Ring buffer full

        size_t to_read = available;
        if (to_read > (size_t)(MAX_READ_PER_CYCLE - total_read))
        {
            to_read = (size_t)(MAX_READ_PER_CYCLE - total_read);
        }

        n = read(serial_fd, write_ptr, to_read);

        if (n > 0)
        {
            ring_buff_advance_write(&g_ring_buf, (size_t)n);
            total_read += (int)n;

            if (logger_active() && display_mode == DISPLAY_MODE_RAW && !g_paused)
            {
                logger_write((const char *)write_ptr, (int)n);
            }
        }
        else if (n == 0)
        {
            break; // EOF
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == EINTR)
                continue;
            perror("serial read");
            break;
        }
    }
}

static void flush_display_batch(TermConfig *cfg, display_mode_t display_mode)
{
    // Skip flushing if paused - don't write to display or log
    if (g_paused)
        return;

    if (g_display_batch_len > 0)
    {
        if (display_mode == DISPLAY_MODE_HEX)
        {
            // In hex mode, format the batch before display
            char hex_buf[HEX_ASCII_LINE_BUF_SIZE];
            format_hex((const char *)g_display_batch, g_display_batch_len, hex_buf, sizeof(hex_buf));
            int hex_len = (int)strlen(hex_buf);
            tui_write(hex_buf, hex_len);
            if (cfg->log_enabled)
            {
                logger_write(hex_buf, hex_len);
            }
        }
        else if (display_mode == DISPLAY_MODE_HEX_ASCII)
        {
            // In hex mode, format the batch before display
            char hex_ascii_buf[HEX_ASCII_LINE_BUF_SIZE];
            format_hex_ascii((const char *)g_display_batch, g_display_batch_len, hex_ascii_buf, sizeof(hex_ascii_buf));
            int hex_ascii_len = (int)strlen(hex_ascii_buf);
            tui_write(hex_ascii_buf, hex_ascii_len);
            if (cfg->log_enabled)
            {
                logger_write(hex_ascii_buf, hex_ascii_len);
            }
        }
        else
        {
            tui_write((const char *)g_display_batch, g_display_batch_len);
        }
        g_display_batch_len = 0;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    if (timespec_diff_ms(&now, &last_display_update) > 16)
    { // 60Hz throttle
        tui_refresh();
        last_display_update = now;
    }
}

static void drain_ring_buffer(TermConfig *cfg, display_mode_t display_mode)
{
    // Skip draining if paused - data stays in ring buffer
    if (g_paused)
        return;

    uint8_t *read_ptr;
    size_t contiguous;

    while ((contiguous = ring_buff_get_read_ptr(&g_ring_buf, &read_ptr)) > 0)
    {
        // Copy to display batch
        size_t to_copy = contiguous;
        if (to_copy > (size_t)(DISPLAY_BATCH_SIZE - g_display_batch_len))
        {
            to_copy = (size_t)(DISPLAY_BATCH_SIZE - g_display_batch_len);
        }

        memcpy(g_display_batch + g_display_batch_len, read_ptr, to_copy);
        g_display_batch_len += (int)to_copy;
        ring_buff_advance_read(&g_ring_buf, to_copy);

        if (g_display_batch_len >= DISPLAY_BATCH_SIZE / 2)
        {
            flush_display_batch(cfg, display_mode);
        }
    }

    flush_display_batch(cfg, display_mode);
}

static void handle_command_mode(TermConfig *cfg, int serial_fd)
{
    (void)serial_fd;

    tui_update_status(cfg, 1);
    mvwprintw(status_win, 0, 1, "CMD: l=toggle log   q=quit   p=pause/resume   Esc=cancel");
    wrefresh(status_win);

    tui_set_timeout(1000);
    int ch = tui_get_char();
    tui_set_timeout(50);

    if (ch == 'l' || ch == 'L')
    {
        if (!logger_active())
        {
            if (logger_start(cfg->log_path) == 0)
                cfg->log_enabled = 1;
        }
        else
        {
            logger_stop();
            cfg->log_enabled = 0;
        }
    }
    else if (ch == 'p' || ch == 'P')
    {
        // Toggle pause state
        g_paused = !g_paused;
    }
    else if (ch == 'q' || ch == 'Q' || ch == 27)
    {
        ungetch('q');
    }

    tui_update_status(cfg, 1);
}

void run_engine(int serial_fd, TermConfig *cfg)
{
    if (!ring_buff_init(&g_ring_buf, g_ring_buffer_storage, RING_BUF_SIZE))
    {
        fprintf(stderr, "Failed to initialize ring buffer\n");
        return;
    }

    g_serial_out_len = 0;
    g_display_batch_len = 0;
    g_paused = 0; // Start in running state
    clock_gettime(CLOCK_MONOTONIC, &last_display_update);

    tui_init(cfg);
    tui_set_timeout(50);
    tui_update_status(cfg, 1);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = serial_fd;
    fds[1].events = POLLIN | POLLERR | POLLHUP;

    int running = 1;
    display_mode_t display_mode = DISPLAY_MODE_RAW; // Changed from int hex_mode to enum based mode toggles

    while (running)
    {
        if (g_serial_out_len > 0)
        {
            fds[1].events |= POLLOUT;
        }
        else
        {
            fds[1].events &= ~POLLOUT;
        }

        int ret = poll(fds, 2, 16); // 16ms timeout for ~60Hz

        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // Keyboard input
        if (fds[0].revents & POLLIN)
        {
            int ch = tui_get_char();

            if (ch == CMD_ESCAPE)
            {
                handle_command_mode(cfg, serial_fd);
                int next = tui_get_char();
                if (next == 'q' || next == 'Q')
                    running = 0;
                if (next != ERR)
                    ungetch(next);
            }
            else if (ch == KEY_MOUSE)
            {
                MEVENT event;
                if (getmouse(&event) == OK)
                {
#if defined(BUTTON4_PRESSED)
                    if (event.bstate & BUTTON4_PRESSED)
                    {
                        tui_scroll_lines(100);
                    }
#if defined(BUTTON5_PRESSED)
                    else if (event.bstate & BUTTON5_PRESSED)
                    {
                        tui_scroll_lines(-100);
                    }
#endif
#endif
                }
            }
            else if (ch == KEY_PPAGE)
            {
                tui_scroll(1);
            }
            else if (ch == KEY_NPAGE)
            {
                tui_scroll(-1);
            }
            else if (ch == 'q' || ch == 'Q')
            {
                running = 0;
            }
            else if (ch == 'h' || ch == 'H')
            {
                // Cycle display mode: RAW -> HEX -> HEX-ASCII -> RAW
                display_mode = (display_mode + 1) % 3;
            }
            else if (ch == 'p' || ch == 'P')
            {
                // Toggle pause/resume
                toggle_pause();
                tui_update_status(cfg, 1); // Update status to show PAUSED state
            }
            else if (ch > 0 && ch != ERR)
            {
                if (isprint(ch) || ch == '\n' || ch == '\r' || ch == '\t' || ch == 0x08)
                {
                    queue_serial_output(serial_fd, (char)ch);
                }
            }
        }

        // Serial output (non-blocking)
        if (fds[1].revents & POLLOUT)
        {
            if (flush_serial_output(serial_fd) < 0)
            {
                perror("serial write");
                running = 0;
            }
        }

        // Serial input (drain to ring buffer)
        if (fds[1].revents & POLLIN)
        {
            // Changed from hex_mode to display_mode_t enum to switch between RAW -> HEX -> HEX+ASCII -> RAW
            process_serial_input(serial_fd, display_mode);
        }

        // Serial errors
        if (fds[1].revents & (POLLHUP | POLLERR))
        {
            tui_update_status(cfg, 0);
            break;
        }

        // Drain ring buffer to display (throttled internally)
        // Changed from hex_mode to display_mode_t enum to switch between RAW -> HEX -> HEX+ASCII -> RAW
        drain_ring_buffer(cfg, display_mode);

        // Periodic display update - also respect pause state
        if (!g_paused)
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            if (timespec_diff_ms(&now, &last_display_update) > 16)
            {
                tui_refresh();
                last_display_update = now;
            }
        }
    }

    // Final flush
    // Changed from hex_mode to display_mode_t enum to switch between RAW -> HEX -> HEX+ASCII -> RAW
    drain_ring_buffer(cfg, display_mode);
    flush_pending_format(cfg, display_mode);
    tui_refresh();

    tui_destroy();
}
