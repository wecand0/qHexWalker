#ifndef Q_HEX_WALKER_LOGGER_H
#define Q_HEX_WALKER_LOGGER_H

#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace TD {
class Logger {
public:
    explicit Logger(std::string loggerName) : loggerName_(std::move(loggerName)) {}
    void Init(const size_t queueSize = 8192, const size_t threadCount = 1) {
        auto console = spdlog::sinks::stdout_color_sink_mt();
        spdlog::init_thread_pool(queueSize, threadCount);
        logger_ = spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>(loggerName_);
        spdlog::set_default_logger(logger_);
    }
    [[nodiscard]] auto GetLoggerInstance() { return logger_; }
    [[nodiscard]] auto GetLoggerName() const { return loggerName_; }
    ~Logger() = default;

private:
    std::string loggerName_;
    std::shared_ptr<spdlog::logger> logger_;
};
}  // namespace TD

#endif  // Q_HEX_WALKER_LOGGER_H