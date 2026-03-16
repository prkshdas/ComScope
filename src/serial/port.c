#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "port.h"
#include "config.h"

/* save original settings to restore */
static struct termios original_tty;

int open_serial(TermConfig *cfg)
{
    /* open read/write so line control (DTR/RTS) works too */
    int fd = open(cfg->port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("open_serial: open() failed");
        return -1;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        perror("open_serial: tcgetattr() failed");
        close(fd);
        return -1;
    }

    /* save original so close_serial() can restore them */
    original_tty = tty;

    /* set baud rates */
    cfsetispeed(&tty, cfg->baud);
    cfsetospeed(&tty, cfg->baud);

    /* 8N1 */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;

    /* enable receiver and local mode */
    tty.c_cflag |= (CREAD | CLOCAL);

    /* raw input / output */
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    tty.c_oflag &= ~OPOST;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    /* non-blocking read: immediate return even if no data */
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        perror("open_serial: tcsetattr() failed");
        close(fd);
        return -1;
    }

    /* CRITICAL: flush any pre-existing buffered data from the serial port */
    tcflush(fd, TCIFLUSH);
    usleep(100000);  /* 100ms delay to allow device to settle */
    tcflush(fd, TCIFLUSH);  /* flush again to be sure */

    return fd;
}

void close_serial(int fd)
{
    if (fd < 0) return;

    /* restore original settings */
    tcsetattr(fd, TCSANOW, &original_tty);
    close(fd);
}