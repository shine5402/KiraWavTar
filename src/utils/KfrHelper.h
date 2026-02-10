/*
 * Copyright 2022, shine_5402.
 *
 * kfr is licensed under GPL v2+,
 * so you can use this file under apache v2 license when licensing your own application in GPL v3.
 * Or if you'd like to, this file can also licensed under MIT License, which should have no conflict with GPL v2.
 * If you have bought kfr's commercial license, you can use this file under apache v2 license without problem.
 */

#ifndef KFR_HELPER_H
#define KFR_HELPER_H

#include <array>
#include <kfr/all.hpp>
#include <string_view>

namespace kfr {

// audio_sample_is_float is now provided by KFR 7

constexpr std::array<std::pair<audio_sample_type, std::string_view>, 5> audio_sample_type_entries = {{
    // {audio_sample_type::i8,  "8-bit int"},
    {audio_sample_type::i16, "16-bit int"},
    {audio_sample_type::i24, "24-bit int"},
    {audio_sample_type::i32, "32-bit int"},
    // {audio_sample_type::i64, "64-bit int"},
    {audio_sample_type::f32, "32-bit single float"},
    {audio_sample_type::f64, "64-bit double float"},
}};

// UI options
constexpr std::array<std::pair<audio_sample_type, std::string_view>, 5> audio_sample_type_entries_for_ui = {{
    {audio_sample_type::i16, "16-bit int"},
    {audio_sample_type::i24, "24-bit int"},
    {audio_sample_type::i32, "32-bit int"},
    {audio_sample_type::f32, "32-bit single float"},
    {audio_sample_type::f64, "64-bit double float"},
}};

// Integer-only sample types for FLAC container (FLAC does not support floating-point)
constexpr std::array<std::pair<audio_sample_type, std::string_view>, 3> audio_sample_type_entries_for_flac = {{
    {audio_sample_type::i16, "16-bit int"},
    {audio_sample_type::i24, "24-bit int"},
    {audio_sample_type::i32, "32-bit int"},
}};

// Map a sample type to the closest FLAC-compatible integer type (by byte width).
// f32 (4 bytes) -> i32 (4 bytes), f64 (8 bytes) -> i32 (capped at FLAC max).
constexpr audio_sample_type to_flac_compatible_sample_type(audio_sample_type t)
{
    switch (t) {
    case audio_sample_type::i16:
    case audio_sample_type::i24:
    case audio_sample_type::i32:
        return t;
    case audio_sample_type::f32:
    case audio_sample_type::f64:
        return audio_sample_type::i32;
    default:
        return audio_sample_type::i24; // safe default
    }
}

constexpr std::string_view audio_sample_type_to_string(audio_sample_type t)
{
    for (const auto &[type, str] : audio_sample_type_entries)
        if (type == t)
            return str;
    return "";
}

constexpr std::array<std::pair<audio_sample_type, int>, 5> audio_sample_type_precision_entries = {{
    // {audio_sample_type::i8, 8},
    {audio_sample_type::i16, 16},
    {audio_sample_type::i24, 24},
    {audio_sample_type::i32, 32},
    // {audio_sample_type::i64, 64},
    {audio_sample_type::f32, 24},
    {audio_sample_type::f64, 53},
}};

constexpr int audio_sample_type_to_precision(audio_sample_type t)
{
    for (const auto &[type, len] : audio_sample_type_precision_entries)
        if (type == t)
            return len;
    return 0;
}
} // namespace kfr

#endif // KFR_HELPER_H
