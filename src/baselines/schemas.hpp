#ifndef SCHEMAS_HPP
#define SCHEMAS_HPP

#include <string>

#include "logger.hpp"

struct Metrics {
    uint64_t generationTime;
    uint64_t startTime, endTime;
    size_t totalCompleteRequests;
    size_t totalInputTokens, totalOutputTokens;

    float ITL;
    uint64_t TTFT;
    uint64_t e2eLatency;

    float TPS;
    float RPS;

    explicit Metrics() { init(); }

    void init();
    void compute();
    void display(const std::string &filePath = "") const;

    ~Metrics() = default;
};

struct RuntimeOptions {
    std::string enginePath;
    std::string inputFilePath;
    std::string outputFilePath;

    bool randomDataset;

    size_t timeout;
    size_t maxOutputTokens;

    const size_t beamWidth = 1;

    Metrics metrics;

    // std::optional<size_t> numReturnSequences;
};

#endif // SCHEMAS_HPP
