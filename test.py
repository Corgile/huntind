import psutil
import time
import os

# 进程PID
PID = 2853101  # 替换为您的进程PID

try:
    while True:
        # 初始化计数器
        running_count = 0
        sleeping_count = 0
        thread_count = 0

        # 获取进程的所有线程ID
        if psutil.pid_exists(PID):
            process = psutil.Process(PID)
            threads = process.threads()
            thread_count = len(threads)

            # 遍历每个线程，检查其状态
            for thread in threads:
                try:
                    with open(f'/proc/{PID}/task/{thread.id}/stat', 'r') as f:
                        status = f.read().split()[2]
                        if status == 'R':
                            running_count += 1
                        elif status == 'S':
                            sleeping_count += 1
                except FileNotFoundError:
                    # 线程可能已经结束
                    pass

            # 打印信息
            print(f"Total threads: {thread_count:<5}, Running: {running_count:<5}, Sleeping: {sleeping_count:<5}", end='\r')
        else:
            print(f"Process with PID {PID} does not exist.")

        # 等待一秒后更新
        time.sleep(1)
except KeyboardInterrupt:
    # 用户中断，打印退出消息
    print("\nMonitoring interrupted by user.")
except Exception as e:
    print(f"An error occurred: {e}")

