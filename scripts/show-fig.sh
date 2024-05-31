#!/usr/bin/bash

scp xhl@host87:~/workspace/flow-encode/cmake-build/process_monitor.log ./
python draw_graph.py $1