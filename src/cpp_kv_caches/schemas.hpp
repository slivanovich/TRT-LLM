#ifndef SCHEMAS_HPP
#define SCHEMAS_HPP

#include <string>

#include "logger.hpp"

struct RuntimeOptions {
    std::string enginePath;
    std::string inputFilePath;
    std::string outputFilePath;

    bool randomDataset;

    size_t timeout;
    size_t maxOutputTokens;

    const size_t beamWidth = 1;

    // std::optional<size_t> numReturnSequences;
};

#endif // SCHEMAS_HPP
