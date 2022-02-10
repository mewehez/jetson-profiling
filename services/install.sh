#!/bin/bash
cp /home/kahanam/experiments/profiling/services/my-profiler.service /etc/systemd/system
cp /home/kahanam/experiments/profiling/services/.my-profiler-conf /etc
systemctl daemon-reload
systemctl enable my-profiler.service