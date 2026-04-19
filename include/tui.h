/*
 * ComScope - Configuration Header
 * Copyright (c) 2026 Prakash Das
 * Licensed under MIT License
 */

#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include "config.h"

extern WINDOW *status_win;

void tui_init(TermConfig *cfg);
void tui_destroy(void);
void tui_write(const char *buf, int len);
void tui_refresh(void);
void tui_scroll(int direction);
void tui_scroll_lines(int lines);
void tui_update_status(TermConfig *cfg, int connected);
void tui_resize(TermConfig *cfg, int connected);
void tui_set_paused(int paused);
int tui_get_char(void);
void tui_set_timeout(int timeout_ms);

#endif