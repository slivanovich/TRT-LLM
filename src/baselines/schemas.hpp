#ifndef SCHEMAS_HPP
#define SCHEMAS_HPP

#include <string>

#include "logger.hpp"

struct Metrics {
    uint64_t generationTime;
    uint64_t startTime, endTime;
    size_t totalCompleteRequests;
    size_t totalInputTokens, totalOutputTokens;

    uint64_t ITL;
    uint64_t TTFT;
    uint64_t e2eLatency;

    float TPS;
    float RPS;

    explicit Metrics()
        : generationTime(0), startTime(0), endTime(1), totalCompleteRequests(0), totalInputTokens(0),
          totalOutputTokens(0), ITL(0), TTFT(0), e2eLatency(0), TPS(0.0f), RPS(0.0f) {}
    ~Metrics() = default;

    void compute();
    void display();
};

struct RuntimeOptions {
    std::string enginePath;
    std::string inputFilePath;
    std::string outputFilePath;

    size_t timeout;
    size_t maxOutputTokens;

    const size_t beamWidth = 1;

    Metrics metrics;

    // std::optional<size_t> numReturnSequences;
};

#endif // SCHEMAS_HPP
