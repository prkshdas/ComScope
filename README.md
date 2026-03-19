# 🖥️ ComScope

> **A lightweight, fast serial port terminal for embedded development boards on Linux**

ComScope is a modern ncurses-based serial terminal designed for embedded developers. It provides an intuitive interface for communicating with microcontrollers, Arduino boards, and other serial devices with ultra-low latency and zero artifacts.

**Perfect for:** Arduino developers • Embedded engineers • IoT enthusiasts • UART debugging

---

## ✨ Key Features

- **⚡ Ultra-Fast**: 30-50ms response time with minimal latency
- **🎨 Clean Interface**: Intuitive ncurses-based terminal UI with smart navigation
- **🔌 Auto-Detection**: Automatically discovers available serial ports
- **📝 Session Logging**: Record all communications with precise timestamps
- **🌈 Terminal Compatible**: Works seamlessly across different terminal emulators
- **⌨️ Keyboard Optimized**: Comprehensive shortcuts for power users
- **🛡️ Robust**: Helpful error messages and detailed diagnostics

---

## 🚀 Quick Start (30 seconds)

### Option 1: Snap Install (Recommended)
```bash
sudo snap install comscope --devmode
comscope
```

### Option 2: Build from Source
```bash
git clone https://github.com/prkshdas/ComScope.git
cd ComScope
make && ./ComScope
```

---

## 📦 Installation

### Method 1: Snap Store (Easiest for All Distributions)

ComScope is available on Snap Store for all Linux distributions.

> **⚠️ Note:** On classic Linux, the snap must be installed in `--devmode` to access serial ports like `/dev/ttyUSB0` or `/dev/ttyACM0`

```bash
# Install
sudo snap install comscope --devmode

# Run
comscope

# Update
sudo snap refresh comscope --devmode

# Remove
sudo snap remove comscope
```

**Supported Architectures:** x86_64, ARM 64-bit (arm64), ARM 32-bit (armhf)

---

### Method 2: Build & Install from Source

#### Prerequisites

- **Linux OS**: Ubuntu, Debian, Fedora, Arch, CentOS, RHEL, etc.
- **Build tools**: GCC compiler
- **Libraries**: libncurses development files

#### Install Dependencies

**Ubuntu / Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses-dev
```

**Fedora / RHEL / CentOS:**
```bash
sudo dnf install gcc ncurses-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel ncurses
```

#### Build & Install

```bash
# Clone repository
git clone https://github.com/prkshdas/ComScope.git
cd ComScope

# Compile
make

# Install system-wide (optional)
sudo make install

# Run from build directory
./ComScope

# Or if installed system-wide
ComScope
```

---

### Method 3: Quick Run (No Installation)

```bash
git clone https://github.com/prkshdas/ComScope.git
cd ComScope
make && ./ComScope
```

---

## 📖 Usage Guide

### Starting ComScope

```bash
comscope          # From Snap or system-wide installation
./ComScope        # From build directory
```

### Workflow Overview

ComScope guides you through three simple steps:

#### Step 1️⃣ — Select Serial Port

Use arrow keys to choose your device, then press Enter:

```
ComScope -- Select a port
  ▸ /dev/ttyUSB0
    /dev/ttyUSB1
    /dev/ttyACM0

up/down=select   Enter=connect   q=quit
```

**Controls:**
- **↑/↓** — Navigate port list
- **Enter** — Connect to selected port
- **q** — Exit application

#### Step 2️⃣ — Configure Connection

Preset for standard embedded devices (customizable):

```
ComScope -- Connection Settings
Port: /dev/ttyUSB0

Baud rate  : 115200
Data bits  : 8
Parity     : None
Stop bits  : 1

Tab=next   left/right=change   Enter=connect   q=back
```

**Default Settings:**
| Parameter | Value |
|-----------|-------|
| Baud Rate | 115200 (adjustable) |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |

**Controls:**
- **Tab** — Move to next field
- **←/→** — Adjust baud rate
- **Enter** — Establish connection
- **q** — Return to port selection

#### Step 3️⃣ — Monitor & Interact

Once connected, receive real-time data:

```
Hello from Arduino
Ready for commands
Temperature: 25.5°C
Voltage: 4.95V
```

Simply type and press Enter to send commands to your device.

---

## ⌨️ Terminal Commands

### Main Terminal Controls

| Command | Function |
|---------|----------|
| **Type text + Enter** | Send data to device |
| **Page Up** | Scroll history up |
| **Page Down** | Scroll history down |
| **Ctrl+A** | Open command menu |
| **q** | Quit application |

### Command Menu (Ctrl+A)

Press **Ctrl+A** to open the menu:

```
CMD: l=toggle log   q=quit   Esc=cancel
```

**Menu Options:**
- **l** — Toggle session logging on/off
- **q** — Exit application
- **Esc** — Return to terminal

---

## 🎯 Advanced Features

### Session Logging

Record all communications with timestamps for debugging and documentation:

1. Press **Ctrl+A** during active session
2. Press **l** to enable logging
3. Data saves to `session.txt` with timestamps
4. Press **Ctrl+A** + **l** again to stop logging

**Log file includes:**
- Session start time
- All transmitted/received data
- Session end time

**Example log:**
```
--- Session started 2026-03-19 14:23:45 ---
Hello
OK
Temperature: 25.5
--- Session ended   2026-03-19 14:25:12 ---
```

### Scrolling Through Terminal History

- **Page Up** — Scroll backward through received data
- **Page Down** — Scroll forward through received data
- Auto-follows new data when at bottom

---

## 🔧 Keyboard Shortcuts Reference

| Key | Action |
|-----|--------|
| **↑** | Navigate up / Increase baud rate |
| **↓** | Navigate down / Decrease baud rate |
| **←** | Previous field / Previous option |
| **→** | Next field / Next option |
| **Tab** | Move to next configuration field |
| **Enter** | Select / Confirm / Connect |
| **q** | Quit / Go back |
| **Ctrl+A** | Open command menu |
| **Page Up** | Scroll history up |
| **Page Down** | Scroll history down |
| **Esc** | Cancel menu / Return to terminal |

---

## 🐛 Troubleshooting

### Snap Installation Issues

**Problem:** `snapd has no serial-port interface slots`

**Cause:** ComScope installed with strict confinement (no hardware access)

**Solution:** Reinstall in developer mode:
```bash
sudo snap remove comscope
sudo snap install comscope --devmode
```

---

### Build from Source Issues

**Problem:** `Permission denied` when opening port

**Cause:** User lacks permission to access `/dev/ttyUSB0` or `/dev/ttyACM0`

**Solution 1 (Temporary):**
```bash
sudo ./ComScope
```

**Solution 2 (Permanent):**
```bash
sudo usermod -aG dialout $USER
# Log out and log back in
```

---

**Problem:** "No ports found"

**Causes & Solutions:**
- Device not connected → Check USB cable and connection
- Device powered off → Turn on your device
- Wrong USB port → Try different USB port on computer
- Missing drivers → Install device-specific drivers
- Manual port check:
  ```bash
  ls -la /dev/tty*
  ```

---

**Problem:** Text appears garbled or colors are wrong

**Solution:**
- Verify terminal supports color output
- Try different terminal emulator (GNOME Terminal, Konsole, xterm)
- Check terminal type:
  ```bash
  echo $TERM
  ```

---

**Problem:** Application crashes unexpectedly

**Solution:**
- Ensure device remains connected
- Try disconnecting and reconnecting device
- Report with terminal output on GitHub Issues

---

## 📂 Generated Files & Configuration

ComScope creates and manages:

| File | Purpose |
|------|---------|
| `session.txt` | Default log file for serial communications |
| `.comscope_history` | Command history *(planned for future release)* |

---

## 🔨 Build & Maintenance

### Development Build
```bash
make clean
make
```

### Install System-Wide
```bash
sudo make install
# Binary installed to: /usr/local/bin/ComScope
```

### Uninstall
```bash
sudo make uninstall
```

### Clean Build Artifacts
```bash
make clean
```

### Create Distribution Package
```bash
make dist
# Creates: dist/ComScope-v1.0.0.tar.gz
```

---

## ⚙️ Known Limitations

- **Fixed Serial Settings**: Data bits (8), parity (none), stop bits (1) — compatible with 99% of devices
- **Scrollback Buffer**: Maximum 20,000 lines of history
- **Text-Only Mode**: No hex/binary view (text display only)
- **Single Session**: No multi-session tab support

---

## 🎯 Planned Features

- [ ] Configurable serial parameters (data bits, parity, stop bits)
- [ ] Multiple session tabs for side-by-side monitoring
- [ ] Hex/ASCII view modes for binary data
- [ ] Search and filter functionality
- [ ] Macro/script support for automation
- [ ] Auto-baud rate detection
- [ ] Advanced serial port monitoring

---

## 📍 Availability & Deployment

### Get ComScope

- **[Snap Store](https://snapcraft.io/comscope)** — Multi-platform installation
- **[GitHub Releases](https://github.com/prkshdas/ComScope/releases)** — Pre-compiled binaries
- **[Source Code](https://github.com/prkshdas/ComScope)** — Build from scratch

### Platform Support

**Operating System:** Linux (all major distributions)
- Ubuntu, Debian, Fedora, Arch, CentOS, RHEL, etc.

**Architecture:** x86_64, ARM 64-bit (arm64), ARM 32-bit (armhf)

**Terminal:** Any POSIX-compliant terminal with ncurses support

---

## 🤝 Contributing

Have a bug report or feature idea? We'd love to hear from you!

1. **[Open an Issue](https://github.com/prkshdas/ComScope/issues)** — Report bugs or request features
2. **[Submit a Pull Request](https://github.com/prkshdas/ComScope/pulls)** — Contribute code improvements
3. **Share Feedback** — Help us improve the project

---

## 📄 License

Licensed under the **MIT License** — See [LICENSE](LICENSE) file for complete terms.

**Permission granted to:** use, modify, and distribute freely.

---

## 👤 Author

**Prakash Das**  
[View GitHub Profile](https://github.com/prkshdas)

---

## 🙏 Acknowledgments

- **[ncurses Library](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)** — Terminal UI framework
- **Inspired by:** minicom, screen, picocom
- **Special thanks:** Embedded development community
- **Distributed via:** [Snap Store](https://snapcraft.io)

---

## 📚 Additional Resources

- **[Serial Communication Basics](https://en.wikipedia.org/wiki/Serial_communication)** — Protocol overview
- **[Linux Serial Programming](https://tldp.org/HOWTO/Serial-Programming-HOWTO/)** — Detailed guide
- **[ncurses Programming](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)** — UI development
- **[Snap Store Docs](https://snapcraft.io/docs)** — Package management
- **[Snapcraft Examples](https://github.com/snapcore/examples)** — Reference implementations

---

## 💬 Support & Help

### Getting Assistance

1. **Check this guide** — See the [Troubleshooting](#-troubleshooting) section
2. **Search GitHub Issues** — [View existing issues](https://github.com/prkshdas/ComScope/issues)
3. **Device Configuration** — Verify your device uses standard serial settings
4. **Snap Help** — Consult [Snap documentation](https://snapcraft.io/docs)

---

**Made with ❤️ for embedded developers on Linux** 🐧

*Last Updated: March 2026*