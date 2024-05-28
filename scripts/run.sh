#!/bin/bash
# 临时修改 core 大小
ulimit -c unlimited
 # 临时修改允许进程打开的最大文件数量（解决 too many opened files 报错）
ulimit -n 4096
# 临时修改 coredump 文件路径 （不需要每次都执行）
# sudo sysctl -w kernel.core_pattern=core.%e.dump
# sudo sysctl -p

rm -f ./*.dump

# 隐藏cuda内部的警告，没什么暖用
export GLOG_minloglevel=2
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so
#LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so

BROKER=${BROKER},172.22.105.202:9092
BROKER=${BROKER},172.22.105.203:9092
BROKER=${BROKER},172.22.105.146:9092
BROKER=${BROKER},172.22.105.147:9092
BROKER=${BROKER},172.22.105.150:9092
BROKER=${BROKER},172.22.105.38:9092
BROKER=${BROKER},172.22.105.39:9092

MODEL_PATH=/data/project/flow-encode/models

# 只有这个 👇才能同时调用多GPU
MODEL_BASE_NAME=${MODEL_PATH}/encoder_script_cuda_bf16.ptc
# 但是 👆 有 flattern_parameters() 问题(即CUDA OOM)， 这与模型定义、训练有关，部署的时候解决不了

# 这个👇模型文件会直接报内存不足/分配内存失败错误（整个过程很快 ）
#MODEL_BASE_NAME=${MODEL_PATH}/encoder_script_cuda_fp32.ptc

# 这个👇每没有 flattern_parameters() 警告，但是只能在cuda:1上成功运行
# MODEL_BASE_NAME=${MODEL_PATH}/encoder_trace_cuda_fp32.ptc

TOPIC=flow-message-87
INTERFACE=ens8f1
FILTER="not port 9092 and(tcp or udp or vlan and(tcp or udp))"

sudo ./hd-torch-kafka \
	--filter="${FILTER}" \
	--device=${INTERFACE} \
	-J4 \
	-p20 \
	-ITC \
	--model=${MODEL_BASE_NAME} \
	--pool=1 \
	--brokers=${BROKER} \
	--topic=flow-message-87
# kafka连接守护线程数计算：-J参数 x --pool参数 x broker数 = 4 x 2 x 7 = 56

# sleep 1
#if [ -f ./core.*.dump ]; then
#  sudo chown xhl:xhl ./core.*.dump
#  chmod 644 ./core.*.dump
#fi
