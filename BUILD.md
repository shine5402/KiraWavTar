# Building KiraWavTar

## Prerequisites

- CMake 3.16+
- Qt 6 (Core, Gui, Widgets, Concurrent, Network)
- A C++17 compiler (Clang recommended)

## Build

```bash
# Configure (adjust Qt path as needed)
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/macos

# Build
cmake --build build
```

## Run

```bash
./build/src/KiraWAVTar
```
