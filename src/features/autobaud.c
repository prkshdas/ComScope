#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "autobaud.h"

static const int auto_baud_flags[] = {B230400, B115200, B57600, B38400, B19200, B9600};
static const int auto_baud_values[] = {230400, 115200, 57600, 38400, 19200, 9600};
static const int NUM_BAUDS = 6;

static long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static float score_data(unsigned char *data, int len)
{
    if (len == 0)
        return -9999;

    if (len < 3)
        return -100;

    int printable = 0, run = 0, max_run = 0;

    for (int i = 0; i < len; i++)
    {
        if (isprint(data[i]) || data[i] == '\r' || data[i] == '\n' || data[i] == '\t')
        {
            printable++;
            run++;
            if (run > max_run)
                max_run = run;
        }
        else
        {
            run = 0;
        }
    }

    float ratio = (float)printable / len;
    float score = ratio * 50.0f;

    if (max_run > 6)
        score += 20.0f;
    if (memchr(data, '\n', len) || memchr(data, '\r', len))
        score += 15.0f;

    if (memchr(data, '\0', len))
        score -= 30.0f;
    if (ratio < 0.5f)
        score -= 40.0f;

    return score;
}

static int setup_serial_for_scan(int fd, int baud_flag)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0)
        return -1;

    cfsetispeed(&tty, baud_flag);
    cfsetospeed(&tty, baud_flag);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_iflag = 0;

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    return tcsetattr(fd, TCSANOW, &tty);
}

int detect_baudrate(const char *port)
{
    unsigned char buffer[256];
    float best_score = -999;
    int best_baud = 0;

    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return 0;

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    usleep(300000);

    for (int pass = 0; pass < 2; pass++)
    {
        for (int i = 0; i < NUM_BAUDS; i++)
        {
            if (setup_serial_for_scan(fd, auto_baud_flags[i]) != 0)
                continue;

            usleep(10000);
            tcflush(fd, TCIFLUSH);

            int total = 0;
            long start = millis();

            while (millis() - start < 50)
            {
                int n = read(fd, buffer + total, sizeof(buffer) - total);
                if (n > 0)
                    total += n;
                if (total >= 32)
                    break;
            }

            float s = score_data(buffer, total);

            if (s > 65.0f && total >= 4)
            {
                best_baud = auto_baud_values[i];
                best_score = s;
                pass = 2;
                break;
            }

            if (s > best_score && total > 0)
            {
                best_score = s;
                best_baud = auto_baud_values[i];
            }
        }

        if (best_score > 0)
            break;
    }

    close(fd);

    if (best_baud != 0 && best_score > -20.0f)
    {
        return best_baud;
    }

    return 0;
}