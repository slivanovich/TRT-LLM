#include "baselines.hpp"

uint64_t getTimeInMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

size_t generateRandomDataset(const std::string &inputFilePath, size_t n) {
    try {
        std::ofstream file(inputFilePath);

        if (!file.is_open()) {
            Logger::getInstance().breakPipeline("Failed to generate random dataset in " + inputFilePath +
                                                ", aborting..");
        }

        size_t m;
        n == 0 ? n = std::rand() % (1 << 12) + (1 << 10) : n = std::max(n, (size_t)0);

        for (size_t row = 0; row < n; row++) {
            m = std::rand() % (1 << 6) + (1 << 4);
            for (size_t column = 0; column < m; column++) {
                file << rand() % (1 << 10) + 1;
                column != m - 1 ? file << ", " : file << "\n";
            }
        }
        file.close();
    } catch (std::exception &e) {
        Logger::getInstance().breakPipeline(e.what());
    }
    return n;
}

RuntimeOptions parseArgs(int argc, char *argv[]) {
    RuntimeOptions runtimeOptions;

    cxxopts::Options options(argv[0], "Save baselines for a model");
    options.add_options()("help", "Print usage")("engine_path", "Path to a directory that contains model engine",
                                                 cxxopts::value<std::string>())(
        "input_file_path", "Path to a csv file that contains input tokens",
        cxxopts::value<std::string>())("output_file_path", "Path to a csv file that will contain output tokens",
                                       cxxopts::value<std::string>()->default_value("output.csv"))(
        "random", "Flag for generating random dataset into the input file", cxxopts::value<int>()->default_value("0"))(
        "timeout", "The maximum time to wait for all responses (in milliseconds)",
        cxxopts::value<size_t>()->default_value("1000000"))("max_output_tokens",
                                                            "The maximum number of tokens to generate",
                                                            cxxopts::value<size_t>()->default_value("1024"));

    auto parsedOptions = options.parse(argc, argv);

    if (parsedOptions.count("help")) {
        Logger::getInstance().info(options.help());
        // TLLM_LOG_INFO(options.help());
        exit(0);
    }

    if (!parsedOptions.count("engine_path")) {
        Logger::getInstance().error(options.help());
        // TLLM_LOG_ERROR(options.help());
        Logger::getInstance().error("Please specify engine directory.");
        // TLLM_LOG_ERROR("Please specify engine directory.");
        exit(1);
    }

    try {
        runtimeOptions.enginePath = parsedOptions["engine_path"].as<std::string>();
        std::cout << runtimeOptions.enginePath << std::endl;
        if (!std::filesystem::exists(runtimeOptions.enginePath) ||
            !std::filesystem::is_directory(runtimeOptions.enginePath)) {
            Logger::getInstance().error("Engine directory doesn't exist.");
            // TLLM_LOG_ERROR("Engine directory doesn't exist.");
            exit(1);
        }
    } catch (std::exception &e) {
        Logger::getInstance().error(e.what());
        // std::cout << e.what() << std::endl;
        exit(1);
    }

    if (!parsedOptions.count("input_file_path")) {
        Logger::getInstance().error(options.help());
        // TLLM_LOG_ERROR(options.help());
        Logger::getInstance().error("Please specify input_file_path");
        // TLLM_LOG_ERROR("Please specify input_file_path");
        exit(1);
    }

    try {
        runtimeOptions.inputFilePath = parsedOptions["input_file_path"].as<std::string>();
        runtimeOptions.outputFilePath = parsedOptions["output_file_path"].as<std::string>();
        runtimeOptions.randomDataset = parsedOptions["random"].as<int>() == 0 ? false : true;
        runtimeOptions.timeout = parsedOptions["timeout"].as<size_t>();
        runtimeOptions.maxOutputTokens = parsedOptions["max_output_tokens"].as<size_t>();
    } catch (std::exception &e) {
        Logger::getInstance().error(e.what());
        // std::cout << e.what() << std::endl;
        exit(1);
    }

    runtimeOptions.metrics = Metrics();
    return runtimeOptions;
}

std::vector<trt_executor::IdType> enqueueRequests(RuntimeOptions &runtimeOpts, trt_executor::Executor &executor) {
    trt_executor::OutputConfig outputConfig;
    trt_executor::SamplingConfig samplingConfig(runtimeOpts.beamWidth);

    samplingConfig.setNumReturnSequences(1);
    outputConfig.excludeInputFromOutput = false;

    Logger::getInstance().info("Reading input tokens from " + runtimeOpts.inputFilePath);
    // TLLM_LOG_INFO("Reading input tokens from %s", runtimeOpts.inputFilePath.c_str());
    auto inputTokens = readInputTokens(runtimeOpts.inputFilePath, runtimeOpts.metrics);
    Logger::getInstance().info("Number of requests: " + std::to_string(inputTokens.size()));
    // TLLM_LOG_INFO("Number of requests: %d", inputTokens.size());

    std::vector<trt_executor::Request> requests;
    for (auto &tokens : inputTokens) {
        Logger::getInstance().info("Creating request with " + std::to_string(tokens.size()) + " input tokens");
        // TLLM_LOG_INFO("Creating request with %d input tokens", tokens.size());
        requests.emplace_back(std::move(tokens), runtimeOpts.maxOutputTokens, false, samplingConfig, outputConfig);
    }

    runtimeOpts.metrics.startTime = getTimeInMs();
    auto requestIds = executor.enqueueRequests(std::move(requests));

    return requestIds;
}

std::vector<trt_executor::VecTokens> readInputTokens(std::string const &path, Metrics &metrics) {
    std::ifstream file(path);
    std::vector<trt_executor::VecTokens> data;

    metrics.totalInputTokens = 0;

    if (!file.is_open()) {
        const auto err = std::string{"Failed to open file: "} + path;
        Logger::getInstance().error(err);
        Logger::getInstance().breakPipeline(err);
        // TLLM_LOG_ERROR(err);
        // TLLM_THROW(err);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<trt_executor::TokenIdType> row;
        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, ',')) {
            try {
                metrics.totalInputTokens++;
                row.push_back(std::stoi(token));
            } catch (std::invalid_argument const &e) {
                Logger::getInstance().error("Invalid argument: " + std::string(e.what()));
                // TLLM_LOG_ERROR("Invalid argument: %s", e.what());
            } catch (std::out_of_range const &e) {
                Logger::getInstance().error("Out of range: " + std::string(e.what()));
                // TLLM_LOG_ERROR("Out of range: %s", e.what());
            }
        }
        data.push_back(row);
    }

    file.close();
    return data;
}

void writeOutputTokens(std::string const &path, std::vector<trt_executor::IdType> &requestIds,
                       std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens> const &outputTokens,
                       trt_executor::SizeType32 beamWidth, Metrics &metrics) {
    std::ofstream file(path);

    if (!file.is_open()) {
        Logger::getInstance().error("Failed to open file " + path);
        // TLLM_LOG_ERROR("Failed to open file %s", path.c_str());
        return;
    }

    for (auto requestId : requestIds) {
        auto const &outTokens = outputTokens.at(requestId);
        for (trt_executor::SizeType32 beam = 0; beam < beamWidth; ++beam) {
            auto const &beamTokens = outTokens.at(beam);
            for (size_t i = 0; i < beamTokens.size(); ++i) {
                file << beamTokens[i];
                metrics.totalOutputTokens++;
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
waitForResponses(RuntimeOptions &runtimeOpts, std::vector<trt_executor::IdType> const &requestIds,
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
            // Logger::getInstance().info("Got " + std::to_string(respTokens.size()) + " tokens for seqIdx " +
            //                            std::to_string(seqIdx) + " for requestId " + std::to_string(requestId));
            // TLLM_LOG_INFO("Got %d tokens for seqIdx %d for requestId %d", respTokens.size(), seqIdx, requestId);

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
                if (numFinished == 1) {
                    runtimeOpts.metrics.TTFT = getTimeInMs() - runtimeOpts.metrics.startTime;
                }

                if (runtimeOpts.beamWidth > 1) {
                    for (trt_executor::SizeType32 beam = 0; beam < numSequences; beam++) {
                        insertResponseTokens(requestId, beam, result.outputTokenIds.at(beam));
                    }
                } else {
                    insertResponseTokens(requestId, result.sequenceIndex, result.outputTokenIds.at(0));
                }
                if (result.isFinal) {
                    // Logger::getInstance().info("Request id " + std::to_string(requestId) + " is completed.");
                    // TLLM_LOG_INFO("Request id %lu is completed.", requestId);
                }
            } else {
                // Allow response with error only if awaitResponse processed a terminated request id
                std::string err = "ReqId " + std::to_string(response.getRequestId()) +
                                  " has already been processed and was terminated.";
                if (response.getErrorMsg() != err) {
                    Logger::getInstance().breakPipeline("Request id " + std::to_string(requestId) +
                                                        " encountered error: " + response.getErrorMsg());
                    // TLLM_THROW("Request id %lu encountered error: %s", requestId, response.getErrorMsg().c_str());
                }
            }
        }
        ++iter;
    }
    if (iter == runtimeOpts.timeout) {
        Logger::getInstance().breakPipeline("Timeout exceeded.");
        // TLLM_THROW("Timeout exceeded.");
    }

    return outputTokens;
}
