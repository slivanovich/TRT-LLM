import matplotlib.pyplot as plt

ns = []
ms = []
ttft = []
itl = []
total_requests = []
e2e_latency = []
tps = []
rps = []

for cnt in range(0, 110):
    n = max(100 * cnt, 1)
    m = 1024
    file_name = f"/home/slivanovich/TRT-LLM/data/plots/plots_data/{n}_{m}.txt"

    data = []
    try:
        file = open(file_name, "r")
    except FileNotFoundError as e:
        continue
    for line in file:
        if line.startswith('Metrics:'):
            metric_data = {}
            for _ in range(8):
                metric_line = next(file).strip()
                key, value = metric_line.split(': ')
                metric_data[key] = float(value.split()[0])
            data.append(metric_data)

    ns.append(n)
    ms.append(m)
    total_requests.append([metric['Total requests'] for metric in data])
    ttft.append([metric['TTFT'] for metric in data])
    itl.append([metric['IT latency'] for metric in data])
    e2e_latency.append([metric['E2E latency'] for metric in data])
    tps.append([metric['TPS'] for metric in data])
    rps.append([metric['RPS'] for metric in data])

plt.figure(figsize=(16, 8))

plt.subplot(3, 3, 1)
plt.plot(total_requests, ttft, marker='o', label='TTFT (sec)')
plt.title('TTFT vs Total Requests')
plt.xlabel('Total Requests')
plt.ylabel('TTFT (sec)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 2)
plt.plot(total_requests, e2e_latency, marker='o', color='orange', label='E2E Latency (sec)')
plt.title('E2E Latency vs Total Requests')
plt.xlabel('Total Requests')
plt.ylabel('E2E Latency (sec)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 3)
plt.plot(total_requests, itl, marker='o', label='IT Latency (ms)')
plt.title('IT Latency vs Total Requests')
plt.xlabel('Total Requests')
plt.ylabel('IT Latency (ms)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 4)
plt.plot(total_requests, tps, marker='o', color='orange', label='TPS (t/sec)')
plt.title('TPS vs Total Requests')
plt.xlabel('Total Requests')
plt.ylabel('TPS (t/sec)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 5)
plt.plot(total_requests, rps, marker='o', label='RPS (r/sec)')
plt.title('RPS vs Total Requests')
plt.xlabel('Total Requests')
plt.ylabel('RPS (r/sec)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 6)
plt.plot(ttft, tps, marker='o', color='orange', label='TPS (t/sec) vs TTFT (sec)')
plt.title('TPS')
plt.xlabel('TTFT (s)')
plt.ylabel('TPS (t/sec)')
plt.grid()
plt.legend()

plt.subplot(3, 3, 8)
plt.plot(itl, tps, marker='o', color='orange', label='TPS (sec) vs IT Latency (ms)')
plt.title('TPS')
plt.xlabel('IT Latency (ms)')
plt.ylabel('TPS (t/sec)')
plt.grid()
plt.legend()

plt.tight_layout()
plt.savefig(f"/home/slivanovich/TRT-LLM/data/plots/many_request.png")

ns = []
ms = []
ttft = []
itl = []
total_requests = []
e2e_latency = []
tps = []
rps = []

for cnt in range(0, 110):
    n = 1
    m = max(10 * cnt, 1)
    file_name = f"/home/slivanovich/TRT-LLM/data/plots/plots_data/{n}_{m}.txt"

    data = []
    try:
        file = open(file_name, "r")
    except FileNotFoundError as e:
        continue
    for line in file:
        if line.startswith('Metrics:'):
            metric_data = {}
            for _ in range(8):
                metric_line = next(file).strip()
                key, value = metric_line.split(': ')
                metric_data[key] = float(value.split()[0])
            data.append(metric_data)

    ns.append(n)
    ms.append(m)
    total_requests.append([metric['Total requests'] for metric in data])
    ttft.append([metric['TTFT'] for metric in data])
    itl.append([metric['IT latency'] for metric in data])
    e2e_latency.append([metric['E2E latency'] for metric in data])
    tps.append([metric['TPS'] for metric in data])
    rps.append([metric['RPS'] for metric in data])

if len(ns) == 0:
    exit()

plt.figure(figsize=(16, 8))

plt.subplot(3, 2, 1)
plt.plot(m, ttft, marker='o', label='TTFT (sec)')
plt.title('TTFT vs Request length')
plt.xlabel('Request length')
plt.ylabel('TTFT (sec)')
plt.grid()
plt.legend()

plt.subplot(3, 2, 2)
plt.plot(m, e2e_latency, marker='o', color='orange', label='E2E Latency (sec)')
plt.title('E2E Latency vs Total Requests')
plt.xlabel('Request length')
plt.ylabel('E2E Latency (sec)')
plt.grid()
plt.legend()

plt.subplot(3, 2, 3)
plt.plot(m, itl, marker='o', label='IT Latency (ms)')
plt.title('IT Latency vs Total Requests')
plt.xlabel('Request length')
plt.ylabel('IT Latency (ms)')
plt.grid()
plt.legend()

plt.subplot(3, 2, 4)
plt.plot(m, tps, marker='o', color='orange', label='TPS (t/sec)')
plt.title('TPS vs Total Requests')
plt.xlabel('Request length')
plt.ylabel('TPS (t/sec)')
plt.grid()
plt.legend()

plt.subplot(3, 2, 6)
plt.plot(m, rps, marker='o', color='orange', label='RPS (r/sec)')
plt.title('RPS vs Total Requests')
plt.xlabel('Request length')
plt.ylabel('RPS (r/sec)')
plt.grid()
plt.legend()

plt.tight_layout()
plt.savefig(f"/home/slivanovich/TRT-LLM/data/plots/many_length.png")
