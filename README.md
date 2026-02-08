# <img src="assets/icon.svg" alt="KiraWAVTar logo" width="48" /> KiraWAVTar

Fast, easy-to-use utility for combining and splitting WAV files, offering a streamlined alternative for batch audio processing.

## Usage

### When to use

Applying consistent effects or fixes to a batch of recordings is a common requirement in audio post-production. For example, if you have multiple tracks sharing the same issues—such as background noise or mouth clicks—they can often be resolved by passing them through a single FX chain. Manually applying the same processing to individual files is time-consuming and tedious.

With KiraWAVTar, you can combine these scattered audio files into a single master file. This allows you to perform your edits in one place, ensuring uniformity across all segments before splitting them back into their original files.

It is also useful if you only need the combined file itself—for instance, when preparing datasets for training RVC, SVS, or other machine learning voice models.

#### But why not use standard batch processing?

Professional software like Adobe Audition or iZotope RX provides built-in batch processing that can achieve similar results. However, these tools usually apply a whole FX chain to all files in one go—a "set-and-forget" approach that lacks interactivity. This makes it difficult to adjust parameters for specific segments or catch issues until the entire process is finished. By working with a single combined file in your familiar DAW, you gain immediate visibility and the ability to fine-tune your processing for every part of the audio dataset.

### How to use

#### Basic workflow

![KiraWAVTar workflow](docs/images/workflow.gif)

Organize your WAV files in a folder (any subfolder structure is supported), use the **Combine** page to merge them into a single file, apply your edits in your preferred software, then split them back out using the **Extract** page.

As shown in the workflow above, you can drag and drop a folder or file directly onto a path input field to set the path quickly.

KiraWAVTar also suggests a default output path whenever you provide a source path:
- **Combining:** the suggested output file is placed next to the input folder, using the folder's name as the filename (e.g., `MyAudioFiles` → `MyAudioFiles.wav`).
- **Extracting:** the suggested output folder is placed next to the input file, with `_result` appended to its name (e.g., `CombinedAudio.wav` → `CombinedAudio_result/`). The suffix is added to avoid confusion with the original file and to prevent accidental overwriting.

These suggestions can be changed at any time before proceeding.

### Why KiraWAVTar?

#### Speed

While this utility handles a straightforward task, performance is still a priority so it fits naturally into your workflow without slowing you down.

Under the hood, KiraWAVTar leverages the highly SIMD-optimized [KFR audio library](https://kfrlib.com/) for resampling and processes files in parallel to keep things moving fast.

For reference, on an Intel i7-9750H with a WD MyPassport 25E1, combining 2,000 WAV files totaling 3 hours of audio takes around 48 seconds, while extracting them back takes around 34 seconds. (This is a casual benchmark with no resampling involved — your mileage may vary.)

#### Easy to use

- Intuitive, drag-and-drop friendly interface that also caters to power users.
- Remembers the source folder structure, file names, and formats during combining, and faithfully restores them on extraction.
- Automatically converts sample rates when needed.

## Format Support

KiraWAVTar supports reading and writing the following WAV containers: RIFF, RF64, and W64. Other containers (such as AIFF) may work for reading, but have not been tested.

Supported bit depths for writing: 8-bit integer, 16-bit integer, 24-bit integer, 32-bit float (IEEE), and 64-bit double (IEEE). Other bit depths (such as 32-bit integer) may work for reading, but have not been tested.

## Supported Platforms

Official prebuilt binaries are provided for:
- Windows 10/11 (x64)
- macOS 13+ (Universal)

Note that the prebuilt binaries are not code-signed, so you may need to dismiss a security warning the first time you run the application. If you prefer, you can always [build from source](docs/BUILD.md) instead.

The codebase has no platform-specific code, and all dependencies are cross-platform, so building and running on other platforms (such as Linux, Windows on ARM etc.) should work without issues as long as the dependencies support it.

## Translations

Official translations are available in the following languages:
- English
- Chinese (Simplified) [简体中文]
- Japanese [日本語]

## By the way...

### Where does this name come from?

The name is inspired by [nwp8861's program "wavtar"](https://osdn.net/users/nwp8861/pf/wavTar/wiki/FrontPage). KiraWAVTar is essentially my *"that's way too hard to use — let me start over"* moment. As for the name itself, it's simply wav + [tar](https://man7.org/linux/man-pages/man1/tar.1.html). Nothing too cryptic!

### Use it as utauwav

Utauwav is a dedicated tool for UTAU voicebank users that optimizes WAV files for UTAU resamplers. KiraWAVTar can achieve the same result by extracting files as 44,100 Hz, 16-bit integer, mono (1 channel), RIFF (non-64-bit) WAV.

## License

This project is licensed under the GPL v3 License. See [LICENSE](LICENSE) for details.

## Acknowledgements

- Qt, The Qt Company Ltd, under LGPL v3.
- KFR - Fast, modern C++ DSP framework, under GNU GPL v2+.
- dr_wav - Public domain single-header WAV reader/writer library by David Reid.
- nwp8861, for the original wavtar program.