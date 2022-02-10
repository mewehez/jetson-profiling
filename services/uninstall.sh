#!/bin/bash

systemctl disable my-profiler.service
rm -f /etc/systemd/system/my-profiler.service
rm -f /etc/.my-profiler-conf
systemctl daemon-reload
