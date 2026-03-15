#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "scanner.h"

int scan_ports(char ports[][270], int max_ports)
{
    DIR *dir = opendir("/dev");
    if (!dir)
        return 0;

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < max_ports)
    {
        char *name = entry->d_name;

        /* scan only ttyUSB, ttyACM, ttyS*/
        if (strncmp(name, "ttyUSB", 6) != 0 &&
            strncmp(name, "ttyACM", 6) != 0 &&
            strncmp(name, "ttyS", 4) != 0)
            continue;

        char path[270];
        snprintf(path, sizeof(path), "/dev/%s", name);
        int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
            continue;
        close(fd);

        strncpy(ports[count], path, 269);
        count++;
    }
    closedir(dir);
    return count; /*return number of the ports found*/
}