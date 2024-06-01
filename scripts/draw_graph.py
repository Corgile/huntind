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
            ymd, time_1, ram_mb, cpu = parts
            timestamps.append(f'{ymd} {time_1}')
            ram_usage.append(float(ram_mb) / 1024)
    # 计算每隔 sample 分钟的行数
    sample_interval = (sample * 60) // 5  # 每隔 sample 分钟
    # 只绘制每隔 sample 分钟的数据
    x_axis = timestamps[::sample_interval]
    y_axis = ram_usage[::sample_interval]
    # 保留曲线末端点
    if timestamps[-1] not in x_axis: x_axis.append(timestamps[-1])
    if ram_usage[-1] not in y_axis: y_axis.append(ram_usage[-1])

    plt.figure(figsize=(16, 8))  # 图表大小
    plt.plot(x_axis, y_axis, label='RAM Usage', color='green')
    # 标注曲线末端点
    end_label = f'{x_axis[-1][5:]} / {y_axis[-1]:.2f} GB'
    plt.annotate(text=end_label, xy=(x_axis[-1], y_axis[-1]),
                 xytext=(len(x_axis) - 30, y_axis[-1] - 0.4),
                 arrowprops=dict(color='red', shrink=0.05, width=0.5, headwidth=5, joinstyle='miter'),
                 fontsize=12, backgroundcolor='red', color='white', fontweight='bold')
    plt.scatter(x_axis[-1], y_axis[-1], color='red')

    # 设置横坐标标签间隔，确保最后一个标签被绘制
    _tick_interval = (tick_ * 60) // (5 * sample_interval)
    xticks = list(range(0, len(x_axis), _tick_interval))

    date_seen = {}
    for index in xticks:
        date_part, time_part = x_axis[index].split()
        if date_part not in date_seen: date_seen[date_part] = True
        else: x_axis[index] = time_part
    # 确保每天至少有一个完整的日期时间戳
    for i in range(len(x_axis)):
        if ' ' in x_axis[i]:
            date_part, _ = x_axis[i].split()
            if date_part not in date_seen:
                date_seen[date_part] = True
                xticks.append(i)
    xticks = sorted(set(xticks))  # 确保xticks有序且无重复

    plt.xlabel('Timestamp')
    plt.ylabel('Usage (GB)')
    plt.title(f'RAM Usage Over Time (sample: {sample}min)')
    plt.legend()
    plt.grid(True)
    plt.xticks(xticks, [x_axis[i] for i in xticks], rotation=90)
    plt.tight_layout()
    plt.savefig('system_usage.png')
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"""
    usage: python {sys.argv[0]} [sample] [ticks]
    
        sample: 每隔 sample 分钟采样一次ram占用
        ticks:  每隔 ticks 分钟绘制一次x轴label
        """)
        exit(-1)

    sample_count = int(sys.argv[1])
    tick_interval = int(sys.argv[2])
    main(sample_count, tick_interval)
