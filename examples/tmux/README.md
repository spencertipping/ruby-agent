# mytmux

A simple terminal multiplexer using `libvterm` and `ncurses`.

## Features
- Multiple tabs support
- Persistent status bar
- Key bindings:
    - `C-a c`: Create new tab
    - `C-a n`: Switch to next tab
    - `C-a C-a`: Send `C-a` to the shell
- Handles resizing / proper shell environment (mostly)

## Build
```bash
mkdir build
cd build
cmake ..
make
```

## Run
```bash
./mytmux
```

## Dependencies
- libvterm
- ncurses (with wide char support preferred)
- standard build tools (cmake, g++)
