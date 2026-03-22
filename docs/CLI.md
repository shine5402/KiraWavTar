# kirawavtar-cli

Command-line interface for combining and extracting audio files. Supports the same operations as the GUI but designed for scripting, automation, and batch workflows.

## Usage

```
kirawavtar-cli [global-options] <command> [command-options] [arguments]
```

## Global Options

| Option | Description |
|--------|-------------|
| `--src-quality <draft\|low\|normal\|high\|perfect>` | Sample rate conversion quality (default: `normal`) |
| `--jobs <N>` | Pipeline concurrency tokens (default: auto) |
| `-q`, `--quiet` | Suppress progress output |
| `--version` | Print version and exit |
| `-h`, `--help` | Print help and exit |

### Environment Variables

Global options can also be set via environment variables (command-line flags take priority):

| Variable | Equivalent |
|----------|------------|
| `KIRAWAVTAR_SRC_QUALITY` | `--src-quality` |
| `KIRAWAVTAR_JOBS` | `--jobs` |

## Commands

### `combine`

Combine multiple audio files (WAV and FLAC) into a single file with a description JSON for later extraction.

```
kirawavtar-cli combine [options] <input-folder> <output-file>
```

#### Format Options

Controls the format of the combined output file. All default to `auto`, which detects the maximum value from all input files.

| Option | Values | Description |
|--------|--------|-------------|
| `-ar <value>` | Hz value or `auto` | Sample rate |
| `-sf <value>` | `s16`, `s24`, `s32`, `f32`, `f64`, or `auto` | Sample format |
| `-ac <value>` | integer or `auto` | Channel count |
| `-f <value>` | `riff`, `rf64`, `w64`, `flac` | Container format (default: `riff`) |

`-sf` also accepts the long form `--sample-format`.

When `-f flac` is combined with a float sample format (`f32`/`f64`), the format is automatically converted to the closest integer equivalent with a warning.

#### Combine Options

| Option | Description |
|--------|-------------|
| `--non-recursive` | Don't scan subfolders for audio files |
| `--no-desc-file` | Don't write the description JSON (disables future extraction) |
| `--desc-filename <path>` | Write description JSON to a custom path instead of the default |
| `--gap <ms>` | Silence gap between entries in milliseconds (default: `0`) |
| `--split-by-entries <count>` | Split output into multiple volumes by entry count |
| `--split-by-duration <secs>` | Split output into multiple volumes by max duration in seconds |
| `--dry-run` | Preview the combine layout without writing files |
| `--json` | With `--dry-run`: output the layout as JSON to stdout |

#### Examples

```bash
# Basic combine (auto-detect format from inputs)
kirawavtar-cli combine ./recordings ./combined.wav

# Combine with specific format and gap
kirawavtar-cli combine -ar 48000 -sf s24 -ac 2 -f flac --gap 100 ./recordings ./combined.flac

# Split into volumes of 50 entries each
kirawavtar-cli combine --split-by-entries 50 ./recordings ./combined.wav

# Preview layout as JSON (for scripting)
kirawavtar-cli combine --dry-run --json ./recordings ./combined.wav

# Combine without writing description file
kirawavtar-cli combine --no-desc-file ./recordings ./combined.wav
```

### `extract`

Extract audio files from a combined file using its description JSON.

```
kirawavtar-cli extract [options] <input-file> <output-folder>
```

#### Format Options

Controls the format of extracted files. All default to `as-input`, which restores each file's original format from the description metadata.

| Option | Values | Description |
|--------|--------|-------------|
| `-ar <value>` | Hz value, `as-src`, or `as-input` | Sample rate |
| `-sf <value>` | `s16`, `s24`, `s32`, `f32`, `f64`, `as-src`, or `as-input` | Sample format |
| `-ac <value>` | integer, `as-src`, or `as-input` | Channel count |
| `-f <value>` | `riff`, `rf64`, `w64`, `flac`, `as-src`, or `as-input` | Container format |

Format mode keywords:

- **`as-input`** (default): Restore each file's original format as recorded in the description metadata.
- **`as-src`**: Use the combined file's format for all outputs.
- **Explicit values** (e.g. `-ar 48000`): Override with the specified value.

#### Extract Options

| Option | Description |
|--------|-------------|
| `--desc-file <path>` | Path to description JSON (if not at the default auto-detected location) |
| `--filter <glob>` | Filter entries by filename pattern (e.g. `"vocals/*"`) |
| `--remove-dc-offset` | Apply DC offset removal to extracted audio |
| `--gap-mode <original\|include>` | Gap handling mode (default: `original`) |
| `--dry-run` | Preview the extraction layout without writing files |

#### Examples

```bash
# Basic extract (restore original formats)
kirawavtar-cli extract ./combined.wav ./output/

# Extract only files matching a pattern
kirawavtar-cli extract --filter "vocals/*" ./combined.wav ./output/

# Extract with DC offset removal
kirawavtar-cli extract --remove-dc-offset ./combined.wav ./output/

# Use a custom description file
kirawavtar-cli extract --desc-file ./my-desc.json ./combined.wav ./output/

# Force all outputs to 48kHz s24 FLAC
kirawavtar-cli extract -ar 48000 -sf s24 -f flac ./combined.wav ./output/

# Preview what would be extracted
kirawavtar-cli extract --dry-run ./combined.wav ./output/
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Runtime error (I/O failure, pipeline error) |
| 2 | Invalid arguments or usage error |
| 3 | Pre-check failed |
| 4 | Cancelled (Ctrl+C) |

## Signal Handling

Pressing Ctrl+C during a combine or extract operation triggers a clean shutdown — the pipeline is cancelled and the process exits with code 4.
