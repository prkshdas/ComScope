# ComScope

A lightweight, fast serial port terminal for embedded development boards on Linux. ComScope provides an intuitive ncurses-based interface for communicating with microcontrollers, Arduino boards, and other serial devices.

## Features

- **Fast & Responsive**: Ultra-low latency serial communication with 30-50ms response time
- **User-Friendly Interface**: Clean ncurses terminal UI with intuitive navigation
- **Auto-Port Detection**: Automatically detects available serial ports
- **Session Logging**: Save all serial communication to log files with timestamps
- **Color Support**: Clean text display without artifacts on various terminals
- **Keyboard Navigation**: Easy keyboard shortcuts for all operations
- **Error Handling**: Helpful error messages and diagnostics

## Requirements

- **Linux** (Ubuntu, Debian, Fedora, Arch, etc.)
- **GCC** compiler
- **libncurses-dev** library

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install gcc ncurses-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel ncurses
```

## Installation

### Option 1: Build & Install from Source

```bash
# Clone the repository
git clone https://github.com/prkshdas/ComScope.git
cd ComScope

# Build
make

# Install (optional - requires sudo)
sudo make install

# Run
./ComScope
# or if installed: ComScope
```

### Option 2: Just Build & Run

```bash
git clone https://github.com/prkshdas/ComScope.git
cd ComScope
make
./ComScope
```

## Usage

### Starting ComScope

```bash
./ComScope
```

### Step 1: Select Serial Port

- Use **↑/↓** arrow keys to select a port
- Press **Enter** to connect
- Press **q** to quit

```
ComScope -- Select a port

 /dev/ttyUSB0
 /dev/ttyUSB1
 /dev/ttyACM0

up/down=select   Enter=connect   q=quit
```

### Step 2: Configure Connection

The connection settings are pre-configured for standard embedded devices:
- **Baud Rate**: 115200 (adjustable)
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1

- Use **Tab** to move between settings
- Use **←/→** arrow keys to change baud rate
- Press **Enter** to connect
- Press **q** to go back

```
ComScope -- Connection Settings
Port: /dev/ttyUSB0

Baud rate  : 115200
Data bits  : 8
Parity     : None
Stop bits  : 1

Tab=next   left/right=change   Enter=connect   q=back
```

### Step 3: Terminal Session

Once connected, you'll see incoming data from your device:

```
Hello from Arduino
Ready for commands
Temperature: 25.5°C
```

#### Available Commands

| Command | Function |
|---------|----------|
| **Ctrl+A** | Open command menu |
| **Page Up** | Scroll up through history |
| **Page Down** | Scroll down through history |
| **q** | Quit application |

#### Command Menu (Ctrl+A)

Press **Ctrl+A** to open the menu:

```
CMD: l=toggle log   q=quit   Esc=cancel
```

- **l** - Toggle session logging on/off
- **q** - Quit application
- **Esc** - Cancel and return to terminal

## Advanced Usage

### Logging Sessions

1. Press **Ctrl+A** to open the command menu
2. Press **l** to enable logging
3. All data will be saved to `session.txt` with timestamps
4. Press **Ctrl+A** then **l** again to disable logging

Log files include:
- Session start time
- All transmitted/received data
- Session end time

Example log:
```
--- Session started 2024-03-16 14:23:45 ---
Hello
OK
Temperature: 25.5
--- Session ended   2024-03-16 14:25:12 ---
```

### Scrolling Through History

- Use **Page Up** to scroll up through received data
- Use **Page Down** to scroll down
- The terminal automatically follows new data when at the bottom

### Sending Text

Simply type any text and press Enter. It will be sent to the serial device:
```
Type: hello
      ↓ (transmitted to device)
Device responds...
```

## Troubleshooting

### "Permission denied" when opening port

**Problem**: Cannot access `/dev/ttyUSB0` or similar

**Solution 1**: Run with sudo (temporary)
```bash
sudo ./ComScope
```

**Solution 2**: Add your user to dialout group (permanent)
```bash
sudo usermod -aG dialout $USER
# Log out and log back in for changes to take effect
```

### "No ports found"

**Problem**: Device not detected

**Solutions**:
- Ensure device is connected via USB
- Check device is powered on
- Try different USB port
- Verify device drivers are installed
- List ports manually:
  ```bash
  ls -la /dev/tty*
  ```

### Text appears garbled or colors are wrong

**Problem**: Terminal color compatibility issue

**Solution**:
- Ensure your terminal supports color
- Try a different terminal emulator (GNOME Terminal, Konsole, xterm)
- Check TERM environment variable:
  ```bash
  echo $TERM
  ```

### Application crashes

**Problem**: ComScope crashes unexpectedly

**Solution**:
- Ensure serial device is still connected
- Try reconnecting the device
- Report the issue with terminal output

## Configuration Files

ComScope creates the following files:

| File | Purpose |
|------|---------|
| `session.txt` | Default log file for serial data |
| `.comscope_history` | (Future) Command history |

## Keyboard Shortcuts Reference

| Key | Action |
|-----|--------|
| **↑** | Navigate up / Change baud rate up |
| **↓** | Navigate down / Change baud rate down |
| **←** | Previous option |
| **→** | Next option |
| **Tab** | Move to next configuration field |
| **Enter** | Select / Connect |
| **q** | Quit / Go back |
| **Ctrl+A** | Open command menu |
| **Page Up** | Scroll history up |
| **Page Down** | Scroll history down |
| **Esc** | Cancel menu / Return to terminal |

## Build Options

### Development Build
```bash
make clean
make
```

### Install System-Wide
```bash
sudo make install
# Binary installed to /usr/local/bin/ComScope
```

### Uninstall
```bash
sudo make uninstall
```

### Clean Build Files
```bash
make clean
```

## Known Limitations

- Serial settings are fixed (8 data bits, no parity, 1 stop bit) - works for 99% of devices
- Maximum 20,000 lines of scrollback history
- No hex view mode (text only)
- No multi-session support

## Future Features

- [ ] Configurable serial parameters (data bits, parity, stop bits)
- [ ] Multiple session tabs
- [ ] Hex/ASCII view modes
- [ ] Search and filter functionality
- [ ] Macro/script support
- [ ] Auto-baud rate detection
- [ ] Serial port monitoring

## Contributing

Found a bug or have a feature request? Feel free to:
1. Open an issue on GitHub
2. Submit a pull request
3. Share feedback

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Author

**Prakash Das** - [GitHub Profile](https://github.com/prkshdas)

## Acknowledgments

- Built with [ncurses](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) library
- Inspired by `minicom`, `screen`, and `picocom`
- Thanks to the embedded development community

## Support

For help and support:
- Check the Troubleshooting section above
- Review issues on GitHub
- Ensure your device is using standard serial settings

## Additional Resources

- [Serial Communication Basics](https://en.wikipedia.org/wiki/Serial_communication)
- [Linux Serial Programming](https://tldp.org/HOWTO/Serial-Programming-HOWTO/)
- [ncurses Programming Guide](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)

---

**Made with ❤️ for embedded developers on Linux**