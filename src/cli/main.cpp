#include <QCoreApplication>
#include <QString>
#include <print>

#include "CombineCommand.h"
#include "ExtractCommand.h"
#include "utils/Utils.h"

#include <csignal>
#include <atomic>

// TBB's profiling.h defines void emit() which conflicts with Qt's 'emit' macro
#pragma push_macro("emit")
#undef emit
#include <oneapi/tbb/task_group.h>
#pragma pop_macro("emit")

// Global signal state for Ctrl+C handling
std::atomic<bool> g_cancelled{false};
oneapi::tbb::task_group_context *g_tbbContext = nullptr;

static void signalHandler(int)
{
    g_cancelled.store(true, std::memory_order_relaxed);
    if (g_tbbContext)
        g_tbbContext->cancel_group_execution();
}

static void printHelp()
{
    std::println("Usage: kirawavtar-cli [global-options] <command> [command-options]");
    std::println("");
    std::println("Commands:");
    std::println("  combine    Combine multiple audio files into a single file");
    std::println("  extract    Extract audio files from a combined file");
    std::println("");
    std::println("Global options:");
    std::println("  --src-quality <draft|low|normal|high|perfect>  SRC quality (default: normal)");
    std::println("  --jobs <N>            Pipeline concurrency tokens (default: auto)");
    std::println("  -q, --quiet           Suppress progress output");
    std::println("  --version             Print version and exit");
    std::println("  -h, --help            Print this help");
    std::println("");
    std::println("Environment variables:");
    std::println("  KIRAWAVTAR_SRC_QUALITY   Same as --src-quality");
    std::println("  KIRAWAVTAR_JOBS          Same as --jobs");
    std::println("");
    std::println("Run 'kirawavtar-cli <command> --help' for command-specific help.");
}

static bool parseSrcQuality(const QString &value)
{
    static const struct {
        const char *name;
        kfr::sample_rate_conversion_quality quality;
    } map[] = {
        {"draft", kfr::sample_rate_conversion_quality::draft},
        {"low", kfr::sample_rate_conversion_quality::low},
        {"normal", kfr::sample_rate_conversion_quality::normal},
        {"high", kfr::sample_rate_conversion_quality::high},
        {"perfect", kfr::sample_rate_conversion_quality::perfect},
    };

    auto lower = value.toLower();
    for (const auto &entry : map) {
        if (lower == entry.name) {
            utils::setSampleRateConversionQuality(entry.quality);
            return true;
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("KiraTools");
    app.setApplicationName("kirawavtar-cli");

    // Install signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Read environment variables (lower priority than flags)
    if (auto envQuality = qEnvironmentVariable("KIRAWAVTAR_SRC_QUALITY"); !envQuality.isEmpty()) {
        if (!parseSrcQuality(envQuality)) {
            std::println(stderr, "Warning: invalid KIRAWAVTAR_SRC_QUALITY='{}', ignoring.", envQuality.toStdString());
        }
    }
    if (auto envJobs = qEnvironmentVariable("KIRAWAVTAR_JOBS"); !envJobs.isEmpty()) {
        bool ok;
        int jobs = envJobs.toInt(&ok);
        if (ok && jobs >= 0) {
            utils::setMaxLiveTokens(jobs);
        } else {
            std::println(stderr, "Warning: invalid KIRAWAVTAR_JOBS='{}', ignoring.", envJobs.toStdString());
        }
    }

    // Parse global options and find subcommand
    QStringList allArgs = app.arguments();
    QStringList subcommandArgs;
    QString subcommand;
    bool quiet = false;

    for (int i = 1; i < allArgs.size(); ++i) {
        const auto &arg = allArgs[i];

        if (!subcommand.isEmpty()) {
            // Everything after subcommand is passed through
            subcommandArgs.append(arg);
            continue;
        }

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        if (arg == "--version") {
            std::println("kirawavtar-cli {}", app.applicationVersion().toStdString());
            return 0;
        }
        if (arg == "--src-quality") {
            if (i + 1 >= allArgs.size()) {
                std::println(stderr, "Error: --src-quality requires a value");
                return 2;
            }
            auto val = allArgs[++i];
            if (!parseSrcQuality(val)) {
                std::println(stderr, "Error: invalid SRC quality '{}'. Use: draft, low, normal, high, perfect",
                             val.toStdString());
                return 2;
            }
        } else if (arg == "--jobs") {
            if (i + 1 >= allArgs.size()) {
                std::println(stderr, "Error: --jobs requires a value");
                return 2;
            }
            auto val = allArgs[++i];
            bool ok;
            int jobs = val.toInt(&ok);
            if (!ok || jobs < 0) {
                std::println(stderr, "Error: invalid jobs value '{}'", val.toStdString());
                return 2;
            }
            utils::setMaxLiveTokens(jobs);
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg.startsWith("-")) {
            std::println(stderr, "Error: unknown global option '{}'", arg.toStdString());
            std::println(stderr, "Run 'kirawavtar-cli --help' for usage.");
            return 2;
        } else {
            subcommand = arg;
        }
    }

    if (subcommand.isEmpty()) {
        printHelp();
        return 2;
    }

    // Pass quiet flag to subcommands
    if (quiet)
        subcommandArgs.prepend("--quiet");

    if (subcommand == "combine") {
        return cli::runCombine(subcommandArgs);
    } else if (subcommand == "extract") {
        return cli::runExtract(subcommandArgs);
    } else {
        std::println(stderr, "Error: unknown command '{}'", subcommand.toStdString());
        std::println(stderr, "Run 'kirawavtar-cli --help' for available commands.");
        return 2;
    }
}
