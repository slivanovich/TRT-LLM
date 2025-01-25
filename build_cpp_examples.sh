cd ${TRTLLM_PATH}/examples/cpp/executor/
mkdir build
cd build
echo "Compiling with MPI examples isn't working for now (will crash) -- edit: ${TRTLLM_PATH}/examples/cpp/executor/CMakeLists.txt.."
cmake .. && make -j
