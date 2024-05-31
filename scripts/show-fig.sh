#!/usr/bin/bash
rm ./process_monitor.log
scp xhl@host87:~/workspace/flow-encode/cmake-build/process_monitor.log ./
python draw_graph.py $1 $2
