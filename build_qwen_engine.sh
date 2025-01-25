#!/bin/bash

cd ${TRTLLM_PATH}/examples/qwen
ls -la

model_dir="${MODELS_PATH}/qwen-0.5b/"
output_dir="${MODELS_PATH}/qwen-0.5b/engine"
checkpoint_dir="${MODELS_PATH}/qwen-0.5b/engine/1-gpu"
weights_quantization=""

mkdir ${output_dir}
mkdir ${checkpoint_dir}

for i in "$@"; do
  case $i in
    -m=*|--model=*)
      model_dir="${i#*=}"
      shift
      ;;
    -cd=*|--checkpoint_dir=*)
      checkpoint_dir="${i#*=}"
      shift
      ;;
    -wq=*|--weights_quantization=*)
      weights_quantization="${i#*=}"
      shift
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

echo "Building Qwen 0.5B engine..."
echo "Model directory path: ${model_dir}"
echo "Checkpoint directory path: ${checkpoint_dir}"
echo "Engine directory path: ${output_dir}"

if [ $(echo ${weights_quantization} | wc -m) -gt 1 ]; then
    echo "Quantization: ${weights_quantization}"
    python3 convert_checkpoint.py --model_dir ${model_dir} \
                                  --output_dir ${checkpoint_dir} \
                                  --dtype float16

    trtllm-build --checkpoint_dir ${checkpoint_dir} \
                 --output_dir ${output_dir} \
                 --gemm_plugin float16
else
    python3 convert_checkpoint.py --model_dir ${model_dir} \
                                  --output_dir ${checkpoint_dir} \
                                  --dtype float16 \
                                  --use_weight_only \
                                  --weight_only_precision ${weights_quantization}

    trtllm-build --checkpoint_dir ${checkpoint_dir} \
                 --output_dir ${output_dir} \
                 --gemm_plugin float16
fi

echo "Successfully builded Qwen 0.5B engine into ${output_dir} directory..."
