#ifndef CLI_PROGRESSREPORTER_H
#define CLI_PROGRESSREPORTER_H

#include <atomic>
#include <thread>

namespace cli {

class ProgressReporter
{
public:
    ProgressReporter(std::atomic<int> &progress, int total, bool quiet);
    ~ProgressReporter();

    void start();
    void finish();

private:
    void pollLoop();

    std::atomic<int> &m_progress;
    int m_total;
    bool m_quiet;
    bool m_isTTY;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    int m_lastReported = -1;
};

} // namespace cli

#endif // CLI_PROGRESSREPORTER_H
