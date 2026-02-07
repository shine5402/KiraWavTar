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

// UI options: omit 32-bit int since internal processing uses float anyway.
constexpr std::array<std::pair<audio_sample_type, std::string_view>, 4> audio_sample_type_entries_for_ui = {{
    {audio_sample_type::i16, "16-bit int"},
    {audio_sample_type::i24, "24-bit int"},
    {audio_sample_type::f32, "32-bit single float"},
    {audio_sample_type::f64, "64-bit double float"},
}};

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
