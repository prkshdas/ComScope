CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -O2 -Iinclude $(shell pkg-config --cflags ncurses)
LIBS     = $(shell pkg-config --libs ncurses)
SRCS     = src/core/main.c src/core/scanner.c \
           src/ui/portpicker.c src/ui/configpanel.c src/ui/tui.c \
           src/io/engine.c \
           src/serial/port.c \
           src/features/logger.c \
		   src/utils/ring_buffer.c\
		   src/features/autobaud.c

TARGET   = ComScope
PREFIX   = /usr/local
BINDIR   = $(PREFIX)/bin

# Default target
all: $(TARGET)

# Build the application
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "✓ Build successful: $(TARGET)"

# Install to system
install: $(TARGET)
	@mkdir -p $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "✓ Installed to $(BINDIR)/$(TARGET)"
	@echo "✓ Run with: ComScope"

# Uninstall from system
uninstall:
	rm -f $(BINDIR)/$(TARGET)
	@echo "✓ Uninstalled from $(BINDIR)/$(TARGET)"

# Clean build artifacts
clean:
	rm -f $(TARGET)
	rm -f *.o
	@echo "✓ Cleaned build artifacts"

# Full clean (including generated files)
distclean: clean
	@echo "✓ Full clean complete"

# Create release tarball
dist: distclean
	@mkdir -p dist
	tar czf dist/$(TARGET)-v1.0.0.tar.gz --exclude=.git --exclude=dist .
	@echo "✓ Created dist/$(TARGET)-v1.0.0.tar.gz"

# Check if dependencies are installed
check-deps:
	@which gcc > /dev/null || (echo "✗ gcc not found. Install with: sudo apt-get install build-essential" && exit 1)
	@pkg-config --exists ncurses || (echo "✗ libncurses-dev not found. Install with: sudo apt-get install libncurses-dev" && exit 1)
	@echo "✓ All dependencies found"

# Help target
help:
	@echo "ComScope Makefile Targets:"
	@echo ""
	@echo "  make              - Build ComScope"
	@echo "  make install      - Install to $(BINDIR)"
	@echo "  make uninstall    - Remove from system"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make distclean    - Remove build artifacts and generated files"
	@echo "  make dist         - Create release tarball"
	@echo "  make check-deps   - Check if dependencies are installed"
	@echo "  make help         - Show this help message"
	@echo ""

.PHONY: all install uninstall clean distclean dist check-deps help
