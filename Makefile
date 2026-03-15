CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -Iinclude $(shell pkg-config --cflags ncurses)
LIBS    = $(shell pkg-config --libs ncurses)
SRCS    = src/core/main.c src/core/scanner.c src/ui/portpicker.c src/ui/configpanel.c
TARGET  = ComScope

all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
clean:
	rm -f $(TARGET)