#ifndef PORTPICKER_H
#define PORTPICKER_H

/* Shows the port picker UI.
   Returns the selected port path (e.g. "/dev/ttyUSB0")
   or NULL if the user pressed q to quit. */
char *pick_port(void);

#endif