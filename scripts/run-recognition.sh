#!/bin/bash

PROGRAM=${HOME}/experiments/profiling/build/aarch64/bin/recognition
DATA_PATH=${HOME}/experiments/profiling/data/images
OUT_PATH=${HOME}/experiments/profiling

# start service
systemctl start my-profiler.service

echo "Waiting for service to start"
sleep 3 
echo "Service started"
echo ""

${PROGRAM} ${DATA_PATH}/black_bear.jpg --network=resnet-50 --log-level=silent --nb-runs=10000  --profile-out=${OUT_PATH}/layer_out.csv --profile

# wait before closing service
echo ""
echo "Stopping service"
sleep 5

stop service
systemctl stop my-profiler.service
echo "Service stopped"
