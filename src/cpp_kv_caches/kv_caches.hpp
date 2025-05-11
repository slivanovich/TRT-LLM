#ifndef KV_CACHES_HPP
#define KV_CACHES_HPP

#include <chrono>
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

#include "logger.hpp"
#include "schemas.hpp"

namespace trt_executor = tensorrt_llm::executor;

uint64_t getTimeInMs();

RuntimeOptions parseArgs(int argc, char *argv[]);

std::vector<trt_executor::IdType> enqueueRequests(RuntimeOptions &runtimeOpts, trt_executor::Executor &executor);

std::vector<trt_executor::VecTokens> readInputTokens(std::string const &path);

std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens>
waitForResponses(RuntimeOptions &runtimeOptions, std::vector<trt_executor::IdType> const &requestIds,
                 trt_executor::Executor &executor);

void writeOutputTokens(std::string const &path, std::vector<trt_executor::IdType> &requestIds,
                       std::unordered_map<trt_executor::IdType, trt_executor::BeamTokens> const &outputTokens,
                       trt_executor::SizeType32 beamWidth);

#endif // KV_CACHES_HPP
