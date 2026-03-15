#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include "port.h"
#include "config.h"

/*save original settings to restore*/
static struct termios original_tty;

int open_serial(TermConfig *cfg)
{
    /*open the device*/
    int fd = open(cfg->port, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
    {
        perror("Open_serial: open() failed");
        return -1;
    }

    /*read current termios settings*/
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0)
    {
        perror("open_serial: tcgetatter() failed");
        close(fd);
        return -1;
    }

    /*save original so close_serial() can restore them*/
    original_tty = tty;

    /*baud rate*/
    cfsetispeed(&tty, cfg->baud); /*input speed*/
    cfsetospeed(&tty, cfg->baud); /*output speed*/

    /*data bits*/
    tty.c_cflag &= ~CSIZE; /*clear data sie bits first*/
    tty.c_cflag |= CS8;    /*8 data bits*/

    /*parity */
    tty.c_cflag &= ~PARENB; /*disable parity*/
    tty.c_cflag &= ~PARODD; /*even parity if enabled*/

    /*stop bits*/
    tty.c_cflag &= ~CSTOPB; /*1 stop bit*/

    /*enable receiver*/
    tty.c_cflag |= CREAD | CLOCAL;

    /*raw input*/
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);

    /*raw output*/
    tty.c_oflag &= ~OPOST;

    /*disable flow control*/
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    /*disable special byte handling*/
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    /* read timing */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    /* apply settings immediately */
    if (tcsetattr(fd, TCSANOW, &tty) < 0)
    {
        perror("open_serial: tcsetattr() failed");
        close(fd);
        return -1;
    }

    return fd; /* success — caller gets the file descriptor */
}

void close_serial(int fd)
{
    if (fd < 0)
        return;
    /* restore the terminal to its original state */
    tcsetattr(fd, TCSANOW, &original_tty);
    close(fd);
}