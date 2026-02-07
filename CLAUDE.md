# AI Coding Agent Guideline

This file provides guidance to AI coding agents when working with code in this repository.

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

- **`WAVCombine`** (`WavCombine.h/cpp`) — Static functions for combining WAV files. Three-phase pipeline: pre-check (validate files, detect format conflicts) → read & combine (parallel via `QtConcurrent::mappedReduced`) → write result with JSON metadata.
- **`WAVExtract`** (`WavExtract.h/cpp`) — Static functions for extracting WAV files. Three-phase pipeline: pre-check → read source WAV → parallel extraction. Optional DC offset removal and resampling via KFR.

### UI Layer

- **`MainWindow`** — Top-level window with `QStackedWidget` switching between combine/extract modes. Manages path inputs, language menu, and update checking.
- **`WavCombineDialog` / `WavExtractDialog`** — Modal progress dialogs that orchestrate async operations via `QFuture`/`QFutureWatcher`. Each dialog chains its pipeline stages and reports warnings/errors.
- **`WAVFormatChooserWidget`** — Reusable widget for selecting sample rate, channels, bit depth, and container format (RIFF/RF64/W64).
- **`ExtractTargetSelectModel`** — `QAbstractTableModel` for selecting which files to extract.

### Utilities (merged from KiraCommonUtils)

- **`KfrHelper.h`** — Qt file I/O adapters for KFR (`qt_file_reader`, `qt_file_writer`), audio sample type metadata (`audio_sample_type_entries`, `audio_sample_type_to_string()`, `audio_sample_type_to_precision()`), and multi-channel WAV writing helpers.
- **`UpdateChecker.h/cpp`** — GitHub release update checker with configurable schedule.
- **`TranslationManager.h/cpp`** + **`Translation.h/cpp`** — i18n support via JSON-based translation file discovery and QTranslator management.
- **`Filesystem.h/cpp`** — `getAbsoluteWAVFileNamesUnder()` for recursive WAV file discovery.
- **`DirNameEditWithBrowse.h/cpp`** — Widget: line edit + browse button for directory selection, with drag-drop support.
- **`FileNameEditWithBrowse.h/cpp`** — Widget: line edit + browse button for file selection (open/save), with drag-drop support.
- **`CommonHtmlDialog.h/cpp`** — Dialog for displaying HTML/Markdown content (used by update checker).

### Description File Format (version 3)

When WAV files are combined, the app produces two output files side by side:
- **Combined WAV file** — all source audio concatenated sequentially into a single multi-file WAV (at a uniform sample rate, bit depth, and channel count chosen by the user).
- **Description JSON file** (`.kirawavtar-desc.json`) — metadata that records where each original file sits within the combined WAV, so extraction can split it back out.

The description file name is derived from the WAV file name via `utils::getDescFileNameFrom()`. Its top-level JSON object contains:

| Field            | Type   | Description                                                       |
| ---------------- | ------ | ----------------------------------------------------------------- |
| `version`        | int    | Always `3`                                                        |
| `sample_rate`    | double | Sample rate of the combined WAV (Hz)                              |
| `sample_type`    | int    | KFR `audio_sample_type` enum value                                |
| `channel_count`  | int    | Number of channels                                                |
| `total_duration` | string | Total duration of the combined WAV as a timecode `"HH:MM:SS.fff"` |
| `descriptions`   | array  | Per-file entries (see below)                                      |

Each entry in `descriptions`:

| Field           | Type   | Description                                            |
| --------------- | ------ | ------------------------------------------------------ |
| `file_name`     | string | Relative path from the source root directory           |
| `sample_rate`   | double | Original file's sample rate (before resampling)        |
| `sample_type`   | int    | Original file's sample type                            |
| `wav_format`    | int    | Original file's WAV container format (RIFF/RF64/W64)   |
| `channel_count` | int    | Number of channels kept (min of source and target)     |
| `begin_time`    | string | Start position in the combined WAV as `"HH:MM:SS.fff"` |
| `duration`      | string | Duration of this segment as `"HH:MM:SS.fff"`           |

Timecodes use millisecond precision (3 decimal places). They are sample-rate-independent — if the combined WAV is resampled in a DAW, the timecodes remain valid. Conversion helpers live in `Utils.h`: `samplesToTimecode()` and `timecodeToSamples()`.

### Third-Party Libraries (vendored in `3rdparty/`)

- **KFR** (`3rdparty/kfr/`) — Custom fork (shine5402/kfr, dev branch) of the KFR DSP library. It now aligns closely with upstream, providing resampling, DC removal and other DSP features.
- **dr_libs** (`3rdparty/dr_libs`) — Provides WAV audio loader and writer, it supports 64bit wav file formats also.
