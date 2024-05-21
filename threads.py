import psutil
import time
import os

# 进程PID
PID = 2853101  # 替换为您的进程PID

try:
    while True:
        # 初始化计数器字典
        status_counts = {'R': 0, 'S': 0, 'D': 0, 'T': 0, 'Z': 0, 'X': 0, 'x': 0}
        status_names  = {'R': 'Running', 'S': 'Sleeping', 'D': 'sleep', 'T': 'Stopped', 't': 'tracing', 'Z': 'Zombie', 'X': 'Dead', 'x': 'dead lock', 'K': 'killed', 'W': 'waking', 'P': 'parked'}
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
                        status = f.read().split()[1]
                        if status in status_counts:
                            status_counts[status] += 1
                except FileNotFoundError:
                    # 线程可能已经结束
                    pass

            # 打印信息
            status_line = f"Total threads: {thread_count}"
            for status, count in status_counts.items():
                status_line += f", {status_names[status]}: {count:<5}"
            print(status_line, end='\r')
        else:
            print(f"\nProcess with PID {PID} does not exist.")

        # 等待一秒后更新
        time.sleep(1)
except KeyboardInterrupt:
    # 用户中断，打印退出消息
    print("\nMonitoring interrupted by user.")
except Exception as e:
    # 其他异常
    print(f"\nAn error occurred: {e}")

