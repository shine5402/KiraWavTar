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

`KFR_ARCH=sse41` is required for KFR's CPU detection in cmake files run correctly. It would not affect the performance of the resulting binary, as KFR would still detect and use the best available instruction set at runtime.

## CMake Options

| Variable                  | Default                | Description                                                                                                                                                                                                                             |
| ------------------------- | ---------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `CMAKE_PREFIX_PATH`       | —                      | Path to your Qt 6 installation (required)                                                                                                                                                                                               |
| `CMAKE_OSX_ARCHITECTURES` | —                      | Target architectures for macOS (e.g. `"x86_64;arm64"`)                                                                                                                                                                                  |
| `KFR_ARCH`                | `target` (auto-detect) | KFR baseline SIMD architecture. Set to `sse41` for universal builds                                                                                                                                                                     |
| `OSX_SEPARATE_DEBUG_INFO` | `OFF`                  | When `ON` (macOS + `RelWithDebInfo` only), runs `dsymutil` to extract a `.dSYM` bundle and strips debug symbols from the binary. Useful for producing a smaller release binary while keeping debug symbols available for crash analysis |

## CMake Presets

`CMakePresets.json` provides template presets as a starting point. Since the Qt path varies per machine, you supply it via a local `CMakeUserPresets.json` that inherits from one of the provided presets.

**Available template presets:**

| Preset name              | Platform | Build type     |
| ------------------------ | -------- | -------------- |
| `macos-debug`            | macOS    | Debug          |
| `macos-relwithdebinfo`   | macOS    | RelWithDebInfo |
| `windows-debug`          | Windows  | Debug          |
| `windows-relwithdebinfo` | Windows  | RelWithDebInfo |

Create a `CMakeUserPresets.json` at the project root (gitignored) and set your Qt path:

```json
{
    "version": 8,
    "configurePresets": [
        {
            "name": "my-macos-debug",
            "inherits": "macos-debug",
            "cacheVariables": {
                "CMAKE_PREFIX_PATH": "/path/to/Qt/6.x.x/macos"
            }
        }
    ]
}
```

Then configure and build with:

```bash
cmake --preset my-macos-debug
cmake --build --preset my-macos-debug
```
