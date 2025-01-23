# echo "Resetting GPU..."
# sudo rmmod nvidia_uvm
# sudo modprobe nvidia_uvm

echo "Creating volumes..."
# docker volume create --driver local -o o=bind -o type=none -o device="/home/slivanovich/TRT-LLM/volumes/test" test_volume
# docker volume create --driver local -o o=bind -o type=none -o device="/home/slivanovich/TRT-LLM/volumes/TensorRT-10.7.0.23" trt
docker volume create --driver local -o o=bind -o type=none -o device="/home/slivanovich/TRT-LLM/volumes/TensorRT-LLM" trt-llm
docker volume create --driver local -o o=bind -o type=none -o device="/home/slivanovich/TRT-LLM/volumes/models/gemma-2b" gemma_2B
docker volume create --driver local -o o=bind -o type=none -o device="/home/slivanovich/TRT-LLM/volumes/models/Qwen2.5-Coder-0.5B" qwen_0.5B

# Already installed locally (nvidia container toolkit for docker)
# echo "Installing nvidia-container-toolkit..."
# sudo ubuntu-drivers install
# curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg \
#   && curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
#     sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
#     sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list
# sudo apt-get update
# sudo apt-get install -y nvidia-container-toolkit
# sudo nvidia-ctk runtime configure --runtime=docker
# sudo systemctl restart docker

touch requirements_.txt
echo "" > requirements_.txt
echo "Copying requirements for qwen model (bad due to absolut path to volume dir)..."
cat /home/slivanovich/TRT-LLM/volumes/TensorRT-LLM/examples/qwen/requirements.txt >> requirements_.txt
tail -n +3 requirements_.txt > requirements.txt
rm requirements_.txt

echo "Building docker..."
docker build -t test .

echo "Running docker..."
# --env NVIDIA_DISABLE_REQUIRE=1
# --ulimit memlock=-1 --ulimit stack=67108864
# --ipc=host
docker run -v "trt-llm:/TRT-LLM/TensorRT-LLM" -v "qwen_0.5B:/TRT-LLM/models/qwen-0.5b" --gpus=all --runtime=nvidia --entrypoint /bin/bash --shm-size 16g -it test

# Useful links:
#   1080ti issues:
#       1) https://github.com/NVIDIA/TensorRT-LLM/issues/1515
#       2) NVIDIA Pascal (SM 6.x) devices are deprecated in TensorRT 8.6. (https://forums.developer.nvidia.com/t/unsupported-sm-0x601/289377/4)
#       3) https://forums.developer.nvidia.com/t/tensorrt-install-required-keyring-7fa2af80-that-is-deprecated-as-per-the-cuda-install-instructions/215134
#       4) https://forums.developer.nvidia.com/t/installing-tensorrt-on-older-version/228501
#   New PC issues:
#       1) https://github.com/NVIDIA/nvidia-container-toolkit/issues/305
#       2) https://github.com/NVIDIA/nvidia-container-toolkit/issues/394
#   For perf-tests:
#       1) https://nvidia.github.io/TensorRT-LLM/performance/perf-best-practices.html
#   General:
#       1) https://github.com/NVIDIA/TensorRT-LLM/issues/22
#   Where are my packages??:
#       1) https://forums.developer.nvidia.com/t/where-is-tensorrt-located-in-an-ubuntu-16-04-dpkg-installation/69042
