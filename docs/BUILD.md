# Building KiraWavTar

## Prerequisites

- CMake 3.16+
- Qt 6 (tested with 6.10.1)
- C++23-capable compiler (Clang 16+ or GCC 13+)

## Basic Build

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-qt6>
cmake --build build
```

## macOS Universal Binaries (x86_64 + arm64)

To build a universal binary, set:

```bash
cmake -B build \
  -DCMAKE_PREFIX_PATH=<path-to-qt6> \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DKFR_ARCH=sse41
```

`KFR_ARCH=sse41` is required because KFR's CPU detection (`try_run`) cannot run during cross-compilation, and architecture-specific compiler flags (e.g. `-mavx2`) would fail when compiling the arm64 slice.

**This does not cause a performance downgrade:**

- **x86_64**: `KFR_ARCH` only sets the compile-time *baseline*. KFR's runtime dispatch (`KFR_ENABLE_MULTIARCH`, on by default for x86) compiles separate codepaths for SSE4.1, AVX, AVX2, and AVX512, selecting the best one at runtime.
- **arm64**: KFR automatically overrides `KFR_ARCH` to `neon64` for non-x86 targets. There are no higher SIMD tiers on aarch64 that KFR supports, so nothing is lost.

## CMake Options

| Variable | Default | Description |
|---|---|---|
| `CMAKE_PREFIX_PATH` | — | Path to your Qt 6 installation (required) |
| `CMAKE_OSX_ARCHITECTURES` | — | Target architectures for macOS (e.g. `"x86_64;arm64"`) |
| `KFR_ARCH` | `target` (auto-detect) | KFR baseline SIMD architecture. Set to `sse41` for universal builds |
| `OSX_SEPARATE_DEBUG_INFO` | `OFF` | When `ON` (macOS + `RelWithDebInfo` only), runs `dsymutil` to extract a `.dSYM` bundle and strips debug symbols from the binary. Useful for producing a smaller release binary while keeping debug symbols available for crash analysis |
