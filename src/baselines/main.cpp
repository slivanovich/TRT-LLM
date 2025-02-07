#include "baselines.hpp"
#include "logger.hpp"

void Metrics::init() {
    generationTime = 0;
    startTime = 0;
    endTime = 1;
    totalCompleteRequests = 0;
    totalInputTokens = totalOutputTokens = 0;

    ITL = 0;
    TTFT = 0;
    e2eLatency = 0;

    TPS = 0;
    RPS = 0;
}

void Metrics::compute() {
    generationTime = endTime - startTime - TTFT;
    e2eLatency = generationTime + TTFT;
    ITL = abs(e2eLatency - TTFT) / (static_cast<float>(totalOutputTokens) - 1);
    TPS = (totalOutputTokens - totalInputTokens) / (static_cast<float>(e2eLatency) / 1000.0);
    RPS = totalCompleteRequests / (static_cast<float>(e2eLatency) / 1000.0);
}

void Metrics::display(const std::string &filePath) const {
    const std::string data =
        "Metrics:\n\tTotal requests: " + std::to_string(totalCompleteRequests) +
        "\n\tTotal input tokens: " + std::to_string(totalInputTokens) +
        "\n\tTotal output tokens: " + std::to_string(totalOutputTokens) + "\n\tTTFT: " + std::to_string(TTFT / 1000.0) +
        " sec\n\tIT latency: " + std::to_string(ITL) + " ms/t\n\tE2E latency: " + std::to_string(e2eLatency / 1000.0) +
        " sec\n\tTPS: " + std::to_string(TPS) + " t/sec\n\tRPS: " + std::to_string(RPS) + " r/sec";

    if (filePath.empty()) {
        Logger::getInstance(true).info(data);
    } else {
        Logger::getInstance(false).info(data);

        std::ofstream file(filePath);

        if (!file.is_open()) {
            Logger::getInstance().breakPipeline("Failed while was opening file for metrics: " + filePath);
        }
        file << data << std::endl;
        file.close();
    }
}

int main(int argc, char *argv[]) {
    initTrtLlmPlugins();

    Logger::getInstance().setFilePath("/TRT-LLM/src/logs.txt");

    auto runtimeOptions = parseArgs(argc, argv);
    auto executorConfig = trt_executor::ExecutorConfig(runtimeOptions.beamWidth);
    auto executor =
        trt_executor::Executor(runtimeOptions.enginePath, trt_executor::ModelType::kDECODER_ONLY, executorConfig);

    for (size_t cnt = 110; cnt < 501; cnt += 10) {
        // const size_t n = std::max(100 * cnt, (size_t)1);
        const size_t n = 1;
        const size_t m = std::max(10 * cnt, (size_t)1);

        runtimeOptions.metrics.init();
        runtimeOptions.inputFilePath = "/TRT-LLM/datasets/" + std::to_string(n) + "_" + std::to_string(m) + ".csv";
        runtimeOptions.maxOutputTokens = 1024;

        if (runtimeOptions.randomDataset) {
            Logger::getInstance(true).info("Random dataset size: " +
                                           std::to_string(generateRandomDataset(runtimeOptions.inputFilePath, n, m)));
        }

        if (executor.canEnqueueRequests()) {
            auto requestIds = enqueueRequests(runtimeOptions, executor);
            auto outputTokens = waitForResponses(runtimeOptions, requestIds, executor);
            runtimeOptions.metrics.endTime = getTimeInMs();

            runtimeOptions.metrics.totalCompleteRequests = requestIds.size();

            // TLLM_LOG_INFO("Writing output tokens to %s", runtimeOptions.outputFilePath.c_str());
            writeOutputTokens(runtimeOptions.outputFilePath, requestIds, outputTokens, runtimeOptions.beamWidth,
                              runtimeOptions.metrics);

            runtimeOptions.metrics.compute();
            runtimeOptions.metrics.display("/TRT-LLM/plots/plots_data/" + std::to_string(n) + "_" + std::to_string(m) + ".txt");
        } else {
            exit(2);
        }
    }

    return 0;
}
