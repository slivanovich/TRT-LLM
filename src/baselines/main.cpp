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
    if (filePath.empty()) {
        std::cout << "Metrics:\n\t" << "Total requests: " << totalCompleteRequests
                  << "\n\tTotal input tokens: " << totalInputTokens << "\n\tTotal output tokens: " << totalOutputTokens
                  << "\n\tTTFT: " << TTFT / 1000.0 << " sec\n\t" << "IT latency: " << ITL
                  << " ms/t\n\tE2E latency: " << e2eLatency / 1000.0 << " sec\n\tTPS: " << TPS
                  << " t/sec\n\tRPS: " << RPS << " r/sec" << std::endl;
    } else {
        std::ofstream file(filePath);

        if (!file.is_open()) {
            Logger::getInstance().breakPipeline("Failed while was opening file for metrics: " + filePath);
        }
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

    for (size_t cnt = 0; cnt < 15; cnt++) {
        runtimeOptions.metrics.init();

        if (runtimeOptions.randomDataset) {
            Logger::getInstance(true).info("Random dataset size: " + std::to_string(generateRandomDataset(
                                                                         runtimeOptions.inputFilePath, 1 << cnt)));
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
            runtimeOptions.metrics.display();
        } else {
            exit(2);
        }
    }

    return 0;
}
