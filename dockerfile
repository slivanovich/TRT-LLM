FROM nvidia/cuda:12.6.1-devel-ubuntu22.04

WORKDIR /TRT-LLM

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# RUN apt-get update
# RUN apt-get install -y software-properties-common
# RUN add-apt-repository -y ppa:nilarimogard/webupd8
# RUN apt-get update
# RUN apt-get install -y launchpad-getkeys

# RUN launchpad-getkeys
RUN apt-get update --fix-missing

RUN apt-get install -y build-essential git git-lfs cmake wget vim
RUN apt-get install -y python3.10 python3-pip openmpi-bin libopenmpi-dev pciutils
RUN apt-get clean

RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install wheel
RUN python3 -m pip install numpy onnx

# RUN pip cache remove "tensorrt*"
# RUN python3 -m pip install --upgrade tensorrt
# RUN python3 -m pip install tensorrt_llm
# RUN pip3 install tensorrt_llm -U --pre --extra-index-url https://pypi.nvidia.com

# COPY nv_tensorrt_repo_ubuntu2004_cuda11_6_trt8_4_2_4_ga_20220720_1_1.deb .
# RUN dpkg -i nv_tensorrt_repo_ubuntu2004_cuda11_6_trt8_4_2_4_ga_20220720_1_1.deb
# RUN cp /var/nv_tensorrt_repo_ubuntu2004_cuda11_6_trt8_4_2_4_ga_20220720_1_1/*-keyring.gpg /usr/share/keyrings/
# RUN apt-key add /var/nv-tensorrt-repo-ubuntu2004-cuda11.6-trt8.4.2.4-ga-20220720/7fd57a00.pub
# RUN apt-get update
# RUN apt-get install tensorrt

COPY TensorRT_8_4_3_1_Ubuntu_20_04_aarch64_gnu_cuda_11_6_cudnn8_4_tar.gz .
RUN tar -xzvf TensorRT_8_4_3_1_Ubuntu_20_04_aarch64_gnu_cuda_11_6_cudnn8_4_tar.gz
RUN export LD_LIBRARY_PATH=/TRT-LLM/TensorRT-8.4.3.1/lib:/usr/local/cuda-11.6/lib64

# RUN git clone https://github.com/NVIDIA/TensorRT-LLM.git
# RUN cd TensorRT-LLM && git submodule update --init --recursive
# RUN cd TensorRT-LLM && git lfs install
# RUN cd TensorRT-LLM && git lfs pull

# COPY ./CMakeLists.txt ./TensorRT-LLM/examples/cpp/executor/

COPY requirements.txt .
# RUN python3 -m pip install -r requirements.txt
# RUN rm requirements.txt

# RUN cd TensorRT-LLM && ls -la cpp/build && rm cpp/build -r
# RUN export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/lib/x86_64-linux-gnu"
# RUN cd TensorRT-LLM && ./scripts/build_wheel.py --cuda_architectures "80-real" --clean
