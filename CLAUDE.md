# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

KiraWavTar is a Qt 6 desktop application for combining multiple WAV files into a single file (with metadata) and extracting them back. It enables batch audio editing workflows where users combine files, apply effects in a DAW, then split them back out. Licensed under GPLv3.

## Build Commands

```bash
# Configure (Qt 6 path required)
cmake -B build -DCMAKE_PREFIX_PATH=/Users/shine_5402/Qt/6.10.1/macos

# Build
cmake --build build

# Run
./build/src/KiraWAVTar
```

There are no tests. No linter is configured.

## Architecture

### Core Processing (namespace-based, not class-based)

- **`WAVCombine`** (`wavcombine.h/cpp`) — Static functions for combining WAV files. Three-phase pipeline: pre-check (validate files, detect format conflicts) → read & combine (parallel via `QtConcurrent::mappedReduced`) → write result with JSON metadata.
- **`WAVExtract`** (`wavextract.h/cpp`) — Static functions for extracting WAV files. Three-phase pipeline: pre-check → read source WAV → parallel extraction. Optional DC offset removal and resampling via KFR.

### UI Layer

- **`MainWindow`** — Top-level window with `QStackedWidget` switching between combine/extract modes. Manages path inputs, language/theme menus, and update checking.
- **`WAVCombineDialog` / `WAVExtractDialog`** — Modal progress dialogs that orchestrate async operations via `QFuture`/`QFutureWatcher`. Each dialog chains its pipeline stages and reports warnings/errors.
- **`WAVFormatChooserWidget`** — Reusable widget for selecting sample rate, channels, bit depth, and container format (RIFF/RF64/W64).
- **`ExtractTargetSelectModel`** — `QAbstractTableModel` for selecting which files to extract.

### Description File Format (version 3)

When WAV files are combined, the app produces two output files side by side:
- **Combined WAV file** — all source audio concatenated sequentially into a single multi-file WAV (at a uniform sample rate, bit depth, and channel count chosen by the user).
- **Description JSON file** (`.kirawavtar-desc.json`) — metadata that records where each original file sits within the combined WAV, so extraction can split it back out.

The description file name is derived from the WAV file name via `wavtar_utils::getDescFileNameFrom()`. Its top-level JSON object contains:

| Field | Type | Description |
|---|---|---|
| `version` | int | Always `3` |
| `sample_rate` | double | Sample rate of the combined WAV (Hz) |
| `sample_type` | int | KFR `audio_sample_type` enum value |
| `channel_count` | int | Number of channels |
| `total_duration` | string | Total duration of the combined WAV as a timecode `"HH:MM:SS.ffffff"` |
| `descriptions` | array | Per-file entries (see below) |

Each entry in `descriptions`:

| Field | Type | Description |
|---|---|---|
| `file_name` | string | Relative path from the source root directory |
| `sample_rate` | double | Original file's sample rate (before resampling) |
| `sample_type` | int | Original file's sample type |
| `wav_format` | int | Original file's WAV container format (RIFF/RF64/W64) |
| `channel_count` | int | Number of channels kept (min of source and target) |
| `begin_time` | string | Start position in the combined WAV as `"HH:MM:SS.ffffff"` |
| `duration` | string | Duration of this segment as `"HH:MM:SS.ffffff"` |

Timecodes use microsecond precision (6 decimal places). They are sample-rate-independent — if the combined WAV is resampled in a DAW, the timecodes remain valid. Conversion helpers live in `wavtar_utils.h`: `samplesToTimecode()` and `timecodeToSamples()`.

### Third-Party Libraries (all vendored in `lib/`)

- **KFR** (`lib/kfr/`) — Custom fork (shine5402/kfr, dev branch) of the KFR DSP library with RF64 large-file support. Provides WAV I/O, resampling, and DC removal. DFT and tests are disabled. Key types: `kfr::audio_reader_wav`, `kfr::audio_writer_wav`, `kfr::univector2d`.
- **KiraCommonUtils** (`lib/KiraCommonUtils/`) — Shared Qt utility library providing dark mode, update checker (GitHub releases), file/dir browser widgets, translation manager, and various dialogs.
- **eternal** (`lib/eternal/`) — Header-only compile-time map from Mapbox, used for audio sample type metadata.

## Qt 6 Migration Notes

This project was recently migrated from Qt 5 / qmake to Qt 6 / CMake. Key gotchas:

- `QVector` is an alias for `QList` in Qt 6 — guard duplicate template specializations with `#if QT_VERSION_MAJOR < 6`
- `QRegExp` → `QRegularExpression` with different match API
- `QString::midRef()`/`leftRef()` removed — use `mid()`/`left()`
- `QLayout::setMargin()` removed — use `setContentsMargins()`
- `QString::length()` returns `qsizetype` (64-bit) — explicit casts needed with `std::min`/`std::max`
- `wchar_t` literals don't implicitly convert to `QChar` — use `QChar(0x00b6)` etc.
- AUTOMOC needs headers with `Q_OBJECT` listed in `add_library()` sources when in separate include dirs

## KFR Fork Patches

If modifying the vendored KFR:
- `lib/kfr/CMakeLists.txt`: `stdc++` linking guarded with `IOS OR APPLE`; `-mstackrealign` guarded with `NOT IOS AND X86`
- `lib/kfr/include/kfr/simd/vec.hpp`: Fix in non-scalar compile-time `set()` using local union trick
