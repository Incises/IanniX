# IanniX

IanniX is a graphical real-time open-source sequencer for digital art, inspired
by Iannis Xenakis's graphic scores. It drives external software and hardware in
real time via OSC, MIDI, HTTP, Serial, TCP, UDP, and Syphon.

> **Note:** This repository is a heavily modified fork of the original
> [buzzinglight/IanniX](https://github.com/buzzinglight/IanniX). The codebase
> is being actively modernized — vendored libraries replaced with system
> packages, Qt4/Qt5 deprecated APIs removed, and the project tracked toward Qt6
> compatibility. See [MIGRATION.md](MIGRATION.md) for the full modernization
> roadmap and current status.

## Documentation

Project home:
<https://iannix.hypar.xyz>

Original upstream documentation:
<https://github.com/buzzinglight/IanniX/wiki>

## Building

See **[BUILD.md](BUILD.md)** for full, platform-specific instructions covering
Linux (openSUSE Tumbleweed and Ubuntu), macOS, and Windows.

Quick start (Linux/macOS):

```bash
cmake -B build -S .
cmake --build build -j$(nproc)
```

## License

GPL-3.0-or-later — see the source file headers for details.
