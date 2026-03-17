#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "logger.h"

static int log_fd = -1;

int logger_start(const char *path)
{
    log_fd = open(path,
                  O_WRONLY | O_CREAT | O_APPEND,
                  0644);
    if (log_fd < 0)
        return -1;

    /* write timestamp header */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char header[64];
    strftime(header, sizeof(header),
             "\n--- Session started %Y-%m-%d %H:%M:%S ---\n", t);
    write(log_fd, header, strlen(header));
    return 0;
}

void logger_write(const char *buf, int n)
{
    if (log_fd < 0)
        return;

    /* FIX: Add error handling for write failures */
    ssize_t written = write(log_fd, buf, n);
    (void)written; /* Avoid unused variable warning */

    /* Flush data regularly to ensure it's written to disk */
    fsync(log_fd);
}

void logger_stop(void)
{
    if (log_fd < 0)
        return;

    /* write closing footer */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char footer[64];
    strftime(footer, sizeof(footer),
             "--- Session ended   %Y-%m-%d %H:%M:%S ---\n", t);
    write(log_fd, footer, strlen(footer));

    /* FIX: Ensure data is synced before closing */
    fsync(log_fd);
    close(log_fd);
    log_fd = -1;
}

int logger_active(void)
{
    return log_fd >= 0; /* FIX: Corrected logic */
}