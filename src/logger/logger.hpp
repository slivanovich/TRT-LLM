#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <ctime>
#include <fstream>
#include <iostream>

class Logger {
  public:
    enum Mode { Default = 0, Streaming = 1 };
    enum LogLevel { Info = 0, Warning = 1, Error = 2 };

  private:
    Mode logMode;
    std::string logFilePath;
    std::ofstream logFile;

    explicit Logger() : logFilePath("") {}

    Logger(Logger const &) = delete;
    Logger &operator=(Logger const &) = delete;

    ~Logger() { closeLogFile(logFilePath); };

    bool openLogFile(const std::string &path);
    bool closeLogFile(const std::string &path);

    void writeLog(const LogLevel logLevel, const std::string &logText);

  public:
    static Logger &getInstance(bool isStreaming = false) {
        static Logger defaultLogger, streamingLogger;

        if (isStreaming) {
            return streamingLogger;
        }
        return defaultLogger;
    }

    const std::string &getFilePath() const { return logFilePath; }
    const Mode &getMode() const { return logMode; }

    void setFilePath(const std::string logFilePath_) {
        if (!closeLogFile(logFilePath)) {
            std::cerr << "Failed while closing file: \"" << logFilePath << "\"" << std::endl;
        }
        if (openLogFile(logFilePath_)) {
            logFilePath = logFilePath_;
            std::cout << "Logs will save into " + logFilePath + " file.." << std::endl;
        } else {
            std::cerr << "Failed while opening file: \"" << logFilePath_ << "\"" << std::endl;
        }
    }

    bool refreshLogs();

    void info(const std::string &message) { writeLog(LogLevel::Info, message); }
    void error(const std::string &message) { writeLog(LogLevel::Error, message); }
    void warning(const std::string &message) { writeLog(LogLevel::Warning, message); }

    void breakPipeline(const std::string &message);
};

#endif // LOGGER_HPP
