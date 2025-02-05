#include "baselines.hpp"
#include "logger.hpp"

void Metrics::compute() {
    generationTime = endTime - startTime - TTFT;
    e2eLatency = TTFT + generationTime;
    ITL = static_cast<signed long>(e2eLatency - TTFT) / (static_cast<signed long>(totalOutputTokens) - 1);
    TPS = totalOutputTokens / static_cast<float>(endTime - startTime) * 1000.0;
    RPS = totalCompleteRequests / static_cast<float>(endTime - startTime) * 1000.0;
}

void Metrics::display() {
    compute();
    std::cout << "Metrics:\n\t" << "Total requests: " << totalCompleteRequests
              << "\n\tTotal input tokens: " << totalInputTokens << "\n\tTotal output tokens: " << totalOutputTokens
              << "\n\tTTFT: " << TTFT / 1000.0 << " sec\n\tE2E latency: " << e2eLatency / 1000.0 << " sec\n\tTPS: " << TPS << " t/sec\n\tRPS: " << RPS << " t/sec"
              << std::endl;
}

int main(int argc, char *argv[]) {
    initTrtLlmPlugins();

    Logger::getInstance().setFilePath("/TRT-LLM/src/logs.txt");

    auto runtimeOptions = parseArgs(argc, argv);
    auto executorConfig = trt_executor::ExecutorConfig(runtimeOptions.beamWidth);
    auto executor =
        trt_executor::Executor(runtimeOptions.enginePath, trt_executor::ModelType::kDECODER_ONLY, executorConfig);

    if (executor.canEnqueueRequests()) {
        auto requestIds = enqueueRequests(runtimeOptions, executor);
        auto outputTokens = waitForResponses(runtimeOptions, requestIds, executor);
        runtimeOptions.metrics.endTime = getTimeInMs();

        runtimeOptions.metrics.totalCompleteRequests = requestIds.size();

        TLLM_LOG_INFO("Writing output tokens to %s", runtimeOptions.outputFilePath.c_str());
        writeOutputTokens(runtimeOptions.outputFilePath, requestIds, outputTokens, runtimeOptions.beamWidth);

        runtimeOptions.metrics.display();
    } else {
        exit(2);
    }

    return 0;
}
