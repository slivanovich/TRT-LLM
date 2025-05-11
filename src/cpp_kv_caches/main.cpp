#ifndef _WIN32
#  ifdef ENABLE_CUFILE
#    include <cufile.h>
#  endif
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include <cstring>
#include <string>
#include <vector>

#include "logger.hpp"
#include "kv_caches.hpp"

bool gpuToFile(tensorrt_llm::runtime::ITensor::SharedPtr const& srcPtr, std::string const& filename) {
    int fd = ::open(filename.c_str(), O_CREAT | O_WRONLY, 0664);

    if (fd < 0) {
        Logger::getInstance(true).error("Failed to open '" + filename.c_str() + "' for writing (POSIX fallback)");
    }

    ssize_t numBytes = static_cast<ssize_t>(srcPtr->getSizeInBytes());
    std::vector<uint8_t> hostBuffer(numBytes);

    cudaError_t cpyStatus = cudaMemcpy(hostBuffer.data(), srcPtr->data(), numBytes, cudaMemcpyDeviceToHost);
    if (cpyStatus != cudaSuccess) {
        Logger::getInstance(true).error("cudaMemcpy to host failed");
    }

    ssize_t written = ::write(fd, hostBuffer.data(), numBytes);
    if (written < 0) {
        Logger::getInstance(true).error("POSIX write error");
    }

    ::close(fd);
    return true;
}

bool fileToGpu(tensorrt_llm::runtime::ITensor::SharedPtr const& dstPtr, std::string const& filename) {
    int fd = ::open(filename.c_str(), O_RDONLY);
    if (fd < 0) {
        Logger::getInstance(true).error("Failed to open '" + filename.c_str() + "' for reading (POSIX fallback)");
    }

    ssize_t numBytes = static_cast<ssize_t>(dstPtr->getSizeInBytes());
    std::vector<uint8_t> hostBuffer(numBytes);

    ssize_t bytesRead = ::read(fd, hostBuffer.data(), numBytes);
    if (bytesRead < 0) {
        Logger::getInstance(true).error("POSIX read error");
    }

    cudaError_t cpyStatus = cudaMemcpy(dstPtr->data(), hostBuffer.data(), numBytes, cudaMemcpyHostToDevice);
    if (cpyStatus != cudaSuccess) {
        Logger::getInstance(true).error("cudaMemcpy to device failed");
    }

    ::close(fd);
    return true;
}

int main(int argc, char *argv[]) {
    initTrtLlmPlugins();

    Logger::getInstance().setFilePath("/TRT-LLM/src/logs.txt");

    auto runtimeOptions = parseArgs(argc, argv);
    auto executorConfig = trt_executor::ExecutorConfig(runtimeOptions.beamWidth);
    auto executor =
        trt_executor::Executor(runtimeOptions.enginePath, trt_executor::ModelType::kDECODER_ONLY, executorConfig);

    for (size_t cnt = 0; cnt < 101; cnt++) {
        runtimeOptions.inputFilePath = "/TRT-LLM/datasets/" + std::to_string(n) + "_" + std::to_string(m) + ".csv";
        runtimeOptions.maxOutputTokens = 1024;

        if (executor.canEnqueueRequests()) {
            executor.getKVCacheEventManager()->get()->getLatestEvents()[0].data.emplace
            auto requestIds = enqueueRequests(runtimeOptions, executor);
            auto outputTokens = waitForResponses(runtimeOptions, requestIds, executor);
            runtimeOptions.metrics.endTime = getTimeInMs();

            runtimeOptions.metrics.totalCompleteRequests = requestIds.size();

            // TLLM_LOG_INFO("Writing output tokens to %s", runtimeOptions.outputFilePath.c_str());
            writeOutputTokens(runtimeOptions.outputFilePath, requestIds, outputTokens, runtimeOptions.beamWidth);
        } else {
            exit(2);
        }
    }

    return 0;
}
