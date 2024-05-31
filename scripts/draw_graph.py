import sys
import matplotlib.pyplot as plt

# 日志文件路径
log_file_path = './process_monitor.log'

# 读取和解析日志文件
timestamps = []
ram_usage = []
cpu_usage = []


def main(sample: int = 60, tick_: int = 10):
    with open(log_file_path, 'r') as file:
        for line in file:
            parts = line.split()
            if len(parts) < 4: continue
            _ts = f'{parts[0][5:]}-{parts[1]}'
            timestamps.append(_ts)
            ram = float(parts[2]) / 1024
            ram_usage.append(ram)

    # 计算每隔 sample 分钟的行数
    sample_interval = (sample * 60) // 5  # 每隔 sample 分钟
    # 只绘制每隔 sample 分钟的数据
    sampled_timestamps = timestamps[::sample_interval]
    sampled_ram_usage = ram_usage[::sample_interval]

    # 绘制折线图
    plt.figure(figsize=(16, 8))  # 图表大小
    plt.plot(sampled_timestamps, sampled_ram_usage, label='RAM Usage', color='green')  # RAM使用情况

    plt.xlabel('Timestamp')
    plt.ylabel('Usage (GB)')
    plt.title(f'RAM Usage Over Time (sample: {sample}min)')
    plt.legend()
    plt.grid(True)

    # 设置横坐标标签间隔，确保最后一个标签被绘制
    _tick_interval = (tick_ * 60) // (5 * sample_interval)
    xticks = list(range(0, len(sampled_timestamps), _tick_interval))
    if len(sampled_timestamps) - 1 not in xticks:
        xticks.append(len(sampled_timestamps) - 1)
    plt.xticks(xticks, rotation=90)

    plt.tight_layout()
    plt.savefig('system_usage.png')
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print(f"""
    usage: python {sys.argv[0]} [sample] [ticks]
    
        sample: 每隔 sample 分钟采样一次ram占用
        ticks:  每隔 ticks 分钟绘制一次x轴label
        """)
        exit(-1)

    sample_count = int(sys.argv[1])
    tick_interval = int(sys.argv[2])
    main(sample_count, tick_interval)
