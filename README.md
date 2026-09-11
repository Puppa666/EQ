# BalancEQ - Intelligent Spectral Balancing EQ

Automatic spectral balancing VST3 plugin that analyzes audio via FFT and applies
6-band corrective EQ to match a -4.5 dB/octave pink noise reference curve.

Inspired by HoRNet BalancEQ.

## Features

- **One-click balancing** - press RESET and the plugin analyzes your audio
- **FFT-based spectral analysis** - 8192-point FFT with Hann windowing
- **6-band parametric EQ** - Sub (60Hz), Low (200Hz), Low-Mid (800Hz), Mid (2.5kHz), Hi-Mid (8kHz), High (15kHz)
- **Pink noise reference** - targets -4.5 dB/octave slope for natural spectral balance
- **Real-time visual feedback** - spectrum, reference curve, corrected spectrum, and EQ curve
- **Dark themed GUI** with color-coded band indicators

## How It Works

1. Insert BalancEQ on your track/bus
2. Play audio for a few seconds to fill the analysis buffer
3. Press **RESET** - the plugin analyzes the accumulated audio
4. BalancEQ computes the difference between your spectrum and the pink noise reference
5. A 6-band corrective EQ is applied to balance the spectrum

## Auto-Build via GitHub Actions

This repo includes a **complete CI/CD pipeline** that automatically builds
the VST3 for all platforms on every push:

| Platform | Runner |
|----------|--------|
| Windows x64 | `windows-latest` (MSVC 2022) |
| macOS x64 | `macos-13` (Xcode) |
| macOS ARM64 | `macos-latest` (Apple Silicon) |
| Linux x64 | `ubuntu-latest` (GCC) |

### To get your builds:

1. **Push to main** -> go to **Actions** tab -> download artifacts from the latest run
2. **Create a Release tag** (e.g. `v1.0.0`) -> GitHub Actions auto-creates a Release with all 4 ZIP files

### Quick Start:

```bash
# 1. Create a new repo on GitHub
# 2. Upload this project (or git clone + push)
# 3. GitHub Actions builds automatically!

# Or build locally:
git clone https://github.com/YOUR_USER/BalancEQ.git
cd BalancEQ
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target BalancEQ_VST3
```

## Local Build Instructions

### Prerequisites
- CMake >= 3.22
- C++17 compiler (MSVC/Clang/GCC)
- JUCE is downloaded automatically via CMake FetchContent

### Windows (Visual Studio)

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BalancEQ_VST3
```

The VST3 will be at: `build\BalancEQ_artefacts\Release\VST3\BalancEQ.vst3`

### macOS (Xcode)

```bash
cmake -B build -G Xcode
cmake --build build --config Release --target BalancEQ_VST3
```

The VST3 will be at: `build/BalancEQ_artefacts/Release/VST3/BalancEQ.vst3`

### Linux (GCC)

```bash
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
  libfreetype6-dev libasound2-dev libgl1-mesa-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target BalancEQ_VST3
```

The VST3 will be at: `build/BalancEQ_artefacts/Release/VST3/BalancEQ.vst3`

### Install the VST3

- **Windows**: Copy `BalancEQ.vst3` to `C:\Program Files\Common Files\VST3\`
- **macOS**: Copy `BalancEQ.vst3` to `/Library/Audio/Plug-Ins/VST3/`
- **Linux**: Copy `BalancEQ.vst3` to `~/.vst3/` or `/usr/lib/vst3/`

## Technical Details

- **FFT**: 8192-point, Hann window, 50% overlap
- **Reference curve**: -4.5 dB/octave pink noise slope (`ref(f) = -4.5 * log2(f/1000)` dB)
- **EQ bands**: 6 peaking filters at 60, 200, 800, 2500, 8000, 15000 Hz
- **Correction range**: +/-12 dB per band
- **Analysis buffer**: accumulates ~1.5 seconds of stereo audio at 44.1kHz

## License

MIT License - see [LICENSE](LICENSE) file.
