import time
from collections import deque

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import psutil


def init():
    line.set_data([], [])
    return line


def get_pid():
    for p in psutil.process_iter(attrs=['pid', 'name']):
        if p.info["name"] == process_name:
            return p.info["name"], int(p.info["pid"])
    return None, None


def update(frame, _pid: int, _pname: str):
    try:
        process = psutil.Process(_pid)
        mem = process.memory_info().rss / 1024  # 转换为 KB
        memory_usage.append(mem)
        timestamps.append(time.time() - start_time)
        if len(memory_usage) > 1:
            diff = mem - memory_usage[-2]
            if diff != 0:
                memory_diffs.append(diff)  # 计算增量
    except psutil.NoSuchProcess:
        _pname, _pid = get_pid()
        process = psutil.Process(_pid)
        mem = process.memory_info().rss / 1024  # 转换为 KB
        memory_usage.append(mem)
        timestamps.append(time.time() - start_time)
        if len(memory_usage) > 1:
            diff = mem - memory_usage[-2]
            if diff != 0:
                memory_diffs.append(diff)  # 计算增量

    line.set_data(timestamps, memory_usage)
    if timestamps[0] == timestamps[-1]:
        ax.set_xlim(max(0, timestamps[0] - 5), timestamps[-1] + 5)
    else:
        ax.set_xlim(max(0, timestamps[0]), timestamps[-1])
    ax.set_ylim(min(memory_usage) - 10, max(memory_usage) + 10)

    # 更新图例，放在标题下方
    ax.legend([f'name:{pname} pid:{_pid}   RAM:{memory_usage[-1] / 1024:.4f} MB  ∆: {memory_diffs[-1]} KB'],
              loc='upper center')

    return line


if __name__ == "__main__":
    process_name = "hd-kfk-live"
    memory_usage = deque(maxlen=60)
    timestamps = deque(maxlen=60)
    memory_diffs = deque(maxlen=60)
    start_time = time.time()
    fig, ax = plt.subplots()
    fig.set_size_inches(10, 4)  # 设置图像尺寸
    line, = ax.plot([], [], lw=2)
    ax.set_ylim(0, 10 * 2 ** 10)  # 设定 y 轴初始范围，单位为 KB
    ax.set_xlim(0, 60)
    ax.set_xlabel('Time (seconds since start)')
    ax.set_ylabel('Memory Usage (KB)')
    ax.grid(True)
    ax.yaxis.set_major_formatter(ticker.StrMethodFormatter('{x:,.0f}'))
    pname, pid = get_pid()
    memory_diffs.append(0)
    ani = animation.FuncAnimation(fig, func=update, fargs=(pid, pname),
                                  init_func=init, blit=True, interval=500,
                                  cache_frame_data=True, save_count=50)
    plt.show()
