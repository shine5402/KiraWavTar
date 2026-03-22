#include "ProgressReporter.h"

#include <chrono>
#include <cstdio>
#include <print>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace cli {

ProgressReporter::ProgressReporter(std::atomic<int> &progress, int total, bool quiet)
    : m_progress(progress)
    , m_total(total)
    , m_quiet(quiet)
    , m_isTTY(isatty(fileno(stderr)))
{
}

ProgressReporter::~ProgressReporter()
{
    finish();
}

void ProgressReporter::start()
{
    if (m_quiet)
        return;
    m_running.store(true, std::memory_order_relaxed);
    m_thread = std::thread(&ProgressReporter::pollLoop, this);
}

void ProgressReporter::finish()
{
    m_running.store(false, std::memory_order_relaxed);
    if (m_thread.joinable())
        m_thread.join();

    if (m_quiet)
        return;

    if (m_isTTY)
        std::print(stderr, "\r[{}/{}] Done.                    \n", m_total, m_total);
    else
        std::print(stderr, "[{}/{}] Done.\n", m_total, m_total);
}

void ProgressReporter::pollLoop()
{
    while (m_running.load(std::memory_order_relaxed)) {
        int current = m_progress.load(std::memory_order_relaxed);
        if (current != m_lastReported) {
            m_lastReported = current;
            if (m_isTTY)
                std::print(stderr, "\r[{}/{}] Processing...", current, m_total);
            else
                std::print(stderr, "[{}/{}] Processing...\n", current, m_total);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

} // namespace cli
