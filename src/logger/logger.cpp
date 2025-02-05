#include "logger.hpp"

bool Logger::openLogFile(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex);

    try {
        logFile.open(path, std::ios_base::app);
        return logFile.is_open();
    } catch (std::exception &e) {
        std::cerr << "Failed while opening file: " << path << ".. Details: " << e.what() << std::endl;
        return false;
    }
    return false;
}

bool Logger::closeLogFile(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex);

    if (path != "") {
        try {
            logFile.close();
            return !logFile.is_open();
        } catch (std::exception &e) {
            std::cerr << "Failed while closing file: " << path << ".. Details: " << e.what() << std::endl;
            return false;
        }
    }
    return false;
}

void Logger::writeLog(const LogLevel logLevel, const std::string &logText) {
    std::lock_guard<std::mutex> lock(mutex);

    if (logMode == Mode::Default && !logFile.is_open()) {
        std::cerr << "No file for logs is provided.." << std::endl;
        return;
    }

    std::time_t time = std::time(nullptr);
    std::string timestamp(std::asctime(std::localtime(&time)));

    timestamp.pop_back();
    std::string text = "[TIME]: " + timestamp + "; ";

    switch (logLevel) {
    case LogLevel::Info:
        text += "[INFO]: ";
        break;
    case LogLevel::Error:
        text += "[ERROR]: ";
        break;
    case LogLevel::Warning:
        text += "[WARNING]: ";
        break;
    }

    text += logText;

    switch (logMode) {
    case Mode::Default:
        logFile << text << "\n";
        break;
    case Mode::Streaming:
        std::cout << text << std::endl;
        break;
    }
}

bool Logger::refreshLogs() {
    std::lock_guard<std::mutex> lock(mutex);

    if (closeLogFile(logFilePath)) {
        return openLogFile(logFilePath);
    } else {
        std::cerr << "Failed while closing file: \"" << logFilePath << "\"" << std::endl;
    }
    return false;
}

void Logger::breakPipeline(const std::string &message) {
    std::lock_guard<std::mutex> lock(mutex);

    std::cerr << "Program has been interapted by logger: " << std::endl;
    throw std::runtime_error(message);
}
