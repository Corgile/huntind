import matplotlib.pyplot as plt
import datetime
import sys

# 日志文件路径
log_file_path = './process_monitor.log'

# 读取和解析日志文件
timestamps = []
ram_usage = []
cpu_usage = []

def main(tick_ : int):
    with open(log_file_path, 'r') as file:
        for line in file:
            parts = line.split()
            if len(parts) < 4: continue
            # 解析数据
            _ts = f'{parts[0][5:]}-{parts[1]}'
            ram = float(parts[2]) / 1024
            #cpu = float(parts[2])
            timestamps.append(_ts)
            ram_usage.append(ram)
            #cpu_usage.append(cpu)

    # 绘制折线图
    plt.figure(figsize=(16, 8))  # 图表大小
    plt.plot(timestamps, ram_usage, label='RAM Usage (GB)', color='blue')  # RAM使用情况
    #plt.plot(timestamps, cpu_usage, label='CPU Usage (%)', color='red')  # CPU使用情况
    plt.xlabel('Timestamp')
    plt.ylabel('Usage')
    plt.title('System Resource Usage Over Time')
    plt.legend()
    plt.grid(True)

    # 设置横坐标标签间隔
    plt.xticks(range(0, len(timestamps), tick_), rotation=90)  # 每隔tick_个标签绘制一次
    plt.tight_layout()  # 自动调整子图参数,使之填充整个图像区域
    # 保存为PNG
    plt.savefig('system_usage.png')
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"usage: python {sys.argv[0]} ticks")
        exit(-1)
    main(int(sys.argv[1]))