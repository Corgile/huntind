import psutil
import time
import datetime
import sys
def main(target_pid: int):
    log_file_path = "process_monitor.log"
    if not psutil.pid_exists(target_pid):
        print(f"PID {target_pid} does not exist.")
        exit(1)
    # 打开日志文件，准备写入
    with open(log_file_path, "a") as log_file:
        process = psutil.Process(target_pid)
        while True:
            memory_info = process.memory_info()
            memory_mb = memory_info.rss / 1024 / 1024  # 将字节转换为MB
            cpu_percent = process.cpu_percent(interval=5)  # 获取CPU使用百分比
            # 获取当前时间
            current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            # 将数据写入日志文件
            _line = f"{current_time} {memory_mb:.2f} {cpu_percent:.2f}\n"
            log_file.write(_line)
            log_file.flush()  # 确保数据被写入文件

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"usage: python {sys.argv[0]} PID")
        exit(-1)
    main(int(sys.argv[1]))
