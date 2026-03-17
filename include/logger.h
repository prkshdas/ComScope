#ifndef LOGGER_H
#define LOGGER_H

/* Opens the log file and writes a session header.
   Returns 0 on success, -1 on failure. */
int logger_start(const char *path);

/* Writes n bytes to the log file. */
void logger_write(const char *buf, int n);

/* Writes a session footer and closes the file. */
void logger_stop(void);

/* Returns 1 if logging is currently active. */
int logger_active(void);

#endif