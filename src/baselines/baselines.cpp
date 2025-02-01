#ifndef BASELINES_HPP
#define BASELINES_HPP

#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "tensorrt_llm/common/assert.h"
#include "tensorrt_llm/common/logger.h"
#include "tensorrt_llm/executor/executor.h"
#include "tensorrt_llm/plugins/api/tllmPlugin.h"

namespace trt_executor = tensorrt_llm::executor;

struct RuntimeOptions {
    std::string enginePath;
    std::string inputFilePath;
    std::string outputFilePath;

    size_t timeout;
    size_t maxOutputTokens;

    const size_t beamWidth = 1;

    // std::optional<size_t> numReturnSequences;
};

RuntimeOptions parseArgs(int argc, char *argv[]);

std::vector<trt_executor::IdType> enqueueRequests(RuntimeOptions const &runtimeOpts, trt_executor::Executor &executor);

std::vector<trt_executor::VecTokens> readInputTokens(std::string const &path);

std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens>
waitForResponses(RuntimeOptions const &runtimeOptions, std::vector<trt_executor::IdType> const &requestIds,
                 trt_executor::Executor &executor);

void writeOutputTokens(std::string const &path, std::vector<trt_executor::IdType> &requestIds,
                       std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens> const &outputTokens,
                       trt_executor::SizeType32 beamWidth);

int main(int argc, char *argv[]) {
    initTrtLlmPlugins();

    auto runtimeOptions = parseArgs(argc, argv);
    auto executorConfig = trt_executor::ExecutorConfig(runtimeOptions.beamWidth);
    auto executor =
        trt_executor::Executor(runtimeOptions.enginePath, trt_executor::ModelType::kDECODER_ONLY, executorConfig);

    if (executor.canEnqueueRequests()) {
        auto requestIds = enqueueRequests(runtimeOptions, executor);
        std::cout << requestIds.size() << std::endl;
        auto outputTokens = waitForResponses(runtimeOptions, requestIds, executor);

        TLLM_LOG_INFO("Writing output tokens to %s", runtimeOptions.outputFilePath.c_str());
        writeOutputTokens(runtimeOptions.outputFilePath, requestIds, outputTokens, runtimeOptions.beamWidth);
    } else {
        exit(2);
    }
    return 0;
}

RuntimeOptions parseArgs(int argc, char *argv[]) {
    RuntimeOptions runtimeOptions;

    cxxopts::Options options(argv[0], "Save baselines for a model");
    options.add_options()("help", "Print usage")("engine_path", "Path to a directory that contains model engine",
                                                 cxxopts::value<std::string>())(
        "input_file_path", "Path to a csv file that contains input tokens",
        cxxopts::value<std::string>())("output_file_path", "Path to a csv file that will contain output tokens",
                                       cxxopts::value<std::string>()->default_value("output.csv"))(
        "timeout", "The maximum time to wait for all responses (in milliseconds)",
        cxxopts::value<size_t>()->default_value("10000"))(
        "max_output_tokens", "The maximum number of tokens to generate", cxxopts::value<size_t>()->default_value("10"));

    auto parsedOptions = options.parse(argc, argv);

    if (parsedOptions.count("help")) {
        TLLM_LOG_INFO(options.help());
        exit(0);
    }

    if (!parsedOptions.count("engine_path")) {
        TLLM_LOG_ERROR(options.help());
        TLLM_LOG_ERROR("Please specify engine directory.");
        exit(1);
    }

    try {
        runtimeOptions.enginePath = parsedOptions["engine_path"].as<std::string>();
        std::cout << runtimeOptions.enginePath << std::endl;
        if (!std::filesystem::exists(runtimeOptions.enginePath) ||
            !std::filesystem::is_directory(runtimeOptions.enginePath)) {
            TLLM_LOG_ERROR("Engine directory doesn't exist.");
            exit(1);
        }
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }

    if (!parsedOptions.count("input_file_path")) {
        TLLM_LOG_ERROR(options.help());
        TLLM_LOG_ERROR("Please specify input_file_path");
        exit(1);
    }

    try {
        runtimeOptions.inputFilePath = parsedOptions["input_file_path"].as<std::string>();
        runtimeOptions.outputFilePath = parsedOptions["output_file_path"].as<std::string>();
        std::cout << runtimeOptions.inputFilePath << std::endl;
        runtimeOptions.timeout = parsedOptions["timeout"].as<size_t>();
        runtimeOptions.maxOutputTokens = parsedOptions["max_output_tokens"].as<size_t>();
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        exit(1);
    }

    return runtimeOptions;
}

std::vector<trt_executor::IdType> enqueueRequests(RuntimeOptions const &runtimeOpts, trt_executor::Executor &executor) {
    trt_executor::OutputConfig outputConfig;
    trt_executor::SamplingConfig samplingConfig(runtimeOpts.beamWidth);

    samplingConfig.setNumReturnSequences(1);
    outputConfig.excludeInputFromOutput = false;

    TLLM_LOG_INFO("Reading input tokens from %s", runtimeOpts.inputFilePath.c_str());
    auto inputTokens = readInputTokens(runtimeOpts.inputFilePath);
    TLLM_LOG_INFO("Number of requests: %d", inputTokens.size());

    std::vector<trt_executor::Request> requests;
    for (auto &tokens : inputTokens) {
        TLLM_LOG_INFO("Creating request with %d input tokens", tokens.size());
        requests.emplace_back(std::move(tokens), runtimeOpts.maxOutputTokens, false, samplingConfig, outputConfig);
    }

    auto requestIds = executor.enqueueRequests(std::move(requests));

    return requestIds;
}

std::vector<trt_executor::VecTokens> readInputTokens(std::string const &path) {
    std::ifstream file(path);
    std::vector<trt_executor::VecTokens> data;

    if (!file.is_open()) {
        const auto err = std::string{"Failed to open file: "} + path;
        TLLM_LOG_ERROR(err);
        TLLM_THROW(err);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<trt_executor::TokenIdType> row;
        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, ',')) {
            try {
                row.push_back(std::stoi(token));
            } catch (std::invalid_argument const &e) {
                TLLM_LOG_ERROR("Invalid argument: %s", e.what());
            } catch (std::out_of_range const &e) {
                TLLM_LOG_ERROR("Out of range: %s", e.what());
            }
        }
        data.push_back(row);
    }
    file.close();

    return data;
}

void writeOutputTokens(std::string const &path, std::vector<trt_executor::IdType> &requestIds,
                       std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens> const &outputTokens,
                       trt_executor::SizeType32 beamWidth) {
    std::ofstream file(path);

    if (!file.is_open()) {
        TLLM_LOG_ERROR("Failed to open file %s", path.c_str());
        return;
    }

    for (auto requestId : requestIds) {
        auto const &outTokens = outputTokens.at(requestId);
        for (trt_executor::SizeType32 beam = 0; beam < beamWidth; ++beam) {
            auto const &beamTokens = outTokens.at(beam);
            for (size_t i = 0; i < beamTokens.size(); ++i) {
                file << beamTokens[i];
                if (i < beamTokens.size() - 1) {
                    file << ", ";
                }
            }
            file << "\n";
        }
    }
    file.close();
}

std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens>
waitForResponses(RuntimeOptions const &runtimeOpts, std::vector<trt_executor::IdType> const &requestIds,
                 trt_executor::Executor &executor) {
    // Map that will be used to store output tokens for requests
    std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens> outputTokens;
    const auto numSequences = 1;

    for (auto requestId : requestIds) {
        outputTokens[requestId] = trt_executor::BeamTokens(numSequences);
    }

    size_t iter = 0;
    size_t numFinished = 0;

    // Get the new tokens for each request
    while (numFinished < requestIds.size() && iter < runtimeOpts.timeout) {
        std::chrono::milliseconds waitTime(1);
        auto responses = executor.awaitResponses(waitTime);

        auto insertResponseTokens = [&outputTokens](trt_executor::IdType requestId, trt_executor::SizeType32 seqIdx,
                                                    trt_executor::VecTokens const &respTokens) {
            TLLM_LOG_INFO("Got %d tokens for seqIdx %d for requestId %d", respTokens.size(), seqIdx, requestId);

            // Store the output tokens for that request id
            auto &outTokens = outputTokens.at(requestId).at(seqIdx);
            outTokens.insert(outTokens.end(), std::make_move_iterator(respTokens.begin()),
                             std::make_move_iterator(respTokens.end()));
        };

        // Loop over the responses
        for (auto const &response : responses) {
            auto requestId = response.getRequestId();
            if (!response.hasError()) {
                auto result = response.getResult();
                numFinished += result.isFinal;
                if (runtimeOpts.beamWidth > 1) {
                    for (trt_executor::SizeType32 beam = 0; beam < numSequences; beam++) {
                        insertResponseTokens(requestId, beam, result.outputTokenIds.at(beam));
                    }
                } else {
                    insertResponseTokens(requestId, result.sequenceIndex, result.outputTokenIds.at(0));
                }
                if (result.isFinal) {
                    TLLM_LOG_INFO("Request id %lu is completed.", requestId);
                }
            } else {
                // Allow response with error only if awaitResponse processed a terminated request id
                std::string err = "ReqId " + std::to_string(response.getRequestId()) +
                                  " has already been processed and was terminated.";
                if (response.getErrorMsg() != err) {
                    TLLM_THROW("Request id %lu encountered error: %s", requestId, response.getErrorMsg().c_str());
                }
            }
        }
        ++iter;
    }
    if (iter == runtimeOpts.timeout) {
        TLLM_THROW("Timeout exceeded.");
    }

    return outputTokens;
}

#endif // BASELINES_HPP
