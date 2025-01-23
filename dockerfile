FROM nvidia/cuda:12.6.1-devel-ubuntu22.04

WORKDIR /TRT-LLM

COPY requirements.txt .

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

ENV CUDA_HOME=/usr/local/cuda-12.6
ENV PATH=${CUDA_HOME}/bin${PATH:+:${PATH}}
ENV LD_LIBRARY_PATH=${CUDA_HOME}/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
ENV CPLUS_INCLUDE_PATH=${CUDA_HOME}/include

# RUN apt-get update
# RUN apt-get install -y software-properties-common
# RUN add-apt-repository -y ppa:nilarimogard/webupd8
# RUN apt-get update
# RUN apt-get install -y launchpad-getkeys
# RUN launchpad-getkeys

# problem with updating -- while updating trying to fetch https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/InRelease (failed cuz no VPN)
RUN apt-get update --fix-missing; exit 0

RUN apt-get install -y build-essential zlib1g-dev \
libncurses5-dev libgdbm-dev libnss3-dev \
libssl-dev libreadline-dev libffi-dev curl software-properties-common

# Python 3.9.0 (breakin nvidia image after that)
# COPY Python-3.9.0.tar.xz .
# RUN tar -xf Python-3.9.0.tar.xz
# RUN cd Python-3.9.0 && ./configure && make altinstall
# RUN rm /usr/bin/python3
# RUN ln -s /usr/local/bin/python3.9 /usr/bin/python3
# RUN ln -s /usr/share/pyshared/lsb_release.py /usr/local/lib/python3.9/site-packages/lsb_release.py
# RUN echo "alias python3='/usr/local/bin/python3.9'" >> ~/.bashrc_aliases
# RUN alias python3='/usr/local/bin/python3.9'

COPY .cache/cmake-3.31.4.tar.gz .
RUN apt-get purge cmake; exit 0
RUN tar -xf cmake-3.31.4.tar.gz
RUN cd cmake-3.31.4 && ./bootstrap && make && make install
RUN hash -r

RUN apt-get update --fix-missing; exit 0
RUN apt-get install -y git git-lfs vim wget
RUN apt-get install -y python3.10 python3-pip pciutils libucx-dev

RUN python3 -m pip install --upgrade pip
RUN python3 -m pip install wheel

RUN wget https://pypi.nvidia.com/libucxx-cu12/libucxx_cu12-0.41.0-py3-none-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl
RUN python3 -m pip install libucxx_cu12-0.41.0-py3-none-manylinux_2_24_x86_64.manylinux_2_28_x86_64.whl

COPY .cache/nv-tensorrt-local-repo-ubuntu2204-10.7.0-cuda-12.6_1.0-1_amd64.deb .
RUN dpkg -i nv-tensorrt-local-repo-ubuntu2204-10.7.0-cuda-12.6_1.0-1_amd64.deb
RUN cp /var/nv-tensorrt-local-repo-ubuntu2204-10.7.0-cuda-12.6/*-keyring.gpg /usr/share/keyrings/
RUN apt-get update
RUN apt-get install -y tensorrt
RUN hash -r

# COPY TensorRT-10.7.0.23.Linux.x86_64-gnu.cuda-12.6.tar.gz .
# RUN tar -xvzf TensorRT-10.7.0.23.Linux.x86_64-gnu.cuda-12.6.tar.gz
# RUN cd TensorRT-10.7.0.23/python && python3 -m pip install tensorrt-*-cp310-none-linux_x86_64.whl

RUN apt-get update --fix-missing; exit 0
RUN apt-get install -y openmpi-bin libopenmpi-dev
RUN apt-get clean

RUN python3 -m pip install -r requirements.txt
RUN rm requirements.txt

RUN python3 -m pip install tensorrt_llm -U --pre --extra-index-url https://pypi.nvidia.com

RUN git clone https://github.com/NVIDIA/TensorRT-LLM.git
WORKDIR /TRT-LLM/TensorRT-LLM
RUN git submodule update --init --recursive
RUN git lfs install
RUN git lfs pull

RUN rm cpp/build -r; exit 0
RUN python3 scripts/build_wheel.py --cuda_architectures "80-real;86-real" --clean
