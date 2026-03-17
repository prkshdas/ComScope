#ifndef CONFIG_H
#define CONFIG_H

#include <termios.h>

typedef struct
{
    char port[270]; /*/dev/ttyXYZ*/
    speed_t baud;   /*baudrate*/
    int databits;   /*5,6,7 or 8*/
    int parity;     /*0=none, 1=odd, 2=even*/
    int stopbits;

    /* Logging features */
    int log_enabled;
    char log_path[256];
} TermConfig;

#endif