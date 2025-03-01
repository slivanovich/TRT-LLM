import os
import time
import matplotlib.pyplot as plt
from tensorrt_llm import LLM, SamplingParams

class Metrics:
    def __init__(self):
        self.refresh()

    def refresh(self):
        self.generationTime = 0
        self.startTime = 0
        self.endTime = 1
        self.totalCompleteRequests = 0
        self.totalInputTokens = 0
        self.totalOutputTokens = 0

        self.ITL = 0
        self.TTFT = 0
        self.e2eLatency = 1

        self.TPS = 0
        self.RPS = 0


    def compute(self, exclude_input_from_output):
        if exclude_input_from_output:
            self.totalOutputTokens -= self.totalInputTokens

        self.generationTime = self.endTime - self.startTime - self.TTFT
        self.e2eLatency = self.generationTime + self.TTFT
        self.ITL = abs(self.e2eLatency - self.TTFT) / (self.totalOutputTokens - 1)
        self.TPS = (self.totalOutputTokens - self.totalInputTokens) / (self.e2eLatency / 1000.0)
        self.RPS = self.totalCompleteRequests / (self.e2eLatency / 1000.0)

    def display(self, filePath):
        data = f"Metrics:\n\tTotal requests: {self.totalCompleteRequests}\n\tTotal input tokens: {self.totalInputTokens}\n\tTotal output tokens: {self.totalOutputTokens}\n\tTTFT: {self.TTFT / 1000.0} sec\n\tIT latency: {self.ITL} ms/t\n\tE2E latency: {self.e2eLatency / 1000.0} sec\n\tTPS: {self.TPS} t/sec\n\tRPS: {self.RPS} r/sec"

        os.system(f"touch {filePath}")
        file = open(filePath, "w")
        file.write(data)
        file.close()

        print(f"Saved in file: {filePath}")


def getTimeInMs():
    return round(time.time() * 1000)


def getPromptFromLine(prompt_text : str):
    prompt = []
    for token in prompt_text.split(','):
        if len(token) != 0 and token != ' ':
            prompt.append(int(token))
    return prompt


def getPromptsInTokens(filePath):
    prompts = []

    file = open(filePath, "r")

    prompt_text = file.readline()
    prompt_text = prompt_text[:len(prompt_text) - 2]
    prompt = getPromptFromLine(prompt_text)
    if len(prompt) != 0:
        prompts.append(prompt)
    while prompt_text:
        prompt_text = file.readline()
        prompt_text = prompt_text[:len(prompt_text) - 2]
        prompt = getPromptFromLine(prompt_text)
        if len(prompt) != 0:
            prompts.append(prompt)
            
    file.close()

    return prompts


def main():
    isManyLength = True

    if isManyLength:
        n = 1
        m = 0
    else:
        n = 0
        m = 1024

    metrics = Metrics()
    llm = LLM(model="/TRT-LLM/models/qwen-0.5b/engine", tokenizer="/TRT-LLM/models/qwen-0.5b")
    sampling_params = SamplingParams(min_tokens=1024, max_tokens=1024, exclude_input_from_output=False, beam_width=1, return_perf_metrics=True)

    while n < 10001 and m < 9001:
        metrics.refresh()

        filePath = f"/TRT-LLM/datasets/{max(n, 1)}_{max(m, 1)}.csv"
        prompts = getPromptsInTokens(filePath)

        for prompt in prompts:
            metrics.totalInputTokens += len(prompt)

        ttft = 0
        metrics.startTime = getTimeInMs()
        outputs, ttft = llm.generate(prompts, sampling_params)
        metrics.TTFT = ttft - metrics.startTime
        metrics.endTime = getTimeInMs()
        metrics.totalCompleteRequests += len(prompts)
        for output in outputs:
            metrics.totalOutputTokens += len(output.outputs[0].token_ids)

        metrics.compute(sampling_params.exclude_input_from_output)
        metrics.display(f"/TRT-LLM/plots/py_plots_data/{max(n, 1)}_{max(m, 1)}.txt")
        
        if isManyLength:
            m += 100
        else:
            n += 100


def generatePlots():
    ns = []
    ms = []
    ttft = []
    itl = []
    total_requests = []
    e2e_latency = []
    tps = []
    rps = []
    
    for cnt in range(1, 110):
        n = max(100 * cnt, 1)
        m = 1024
        file_name = f"/TRT-LLM/plots/py_plots_data/{n}_{m}.txt"

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
    plt.savefig(f"/TRT-LLM/plots/py_many_request.png")

    ns = []
    ms = []
    ttft = []
    itl = []
    total_requests = []
    e2e_latency = []
    tps = []
    rps = []

    for cnt in range(0, 1001):
        n = 1
        m = max(10 * cnt, 1)
        file_name = f"/TRT-LLM/plots/py_plots_data/{n}_{m}.txt"

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

    plt.subplot(2, 2, 1)
    plt.plot(ms, ttft, marker='o', label='TTFT (sec)')
    plt.title('TTFT vs Request length')
    plt.xlabel('Request length')
    plt.ylabel('TTFT (sec)')
    plt.grid()
    plt.legend()

    plt.subplot(2, 2, 2)
    plt.plot(ms, e2e_latency, marker='o', color='orange', label='E2E Latency (sec)')
    plt.title('E2E Latency vs Total Requests')
    plt.xlabel('Request length')
    plt.ylabel('E2E Latency (sec)')
    plt.grid()
    plt.legend()

    plt.subplot(2, 2, 3)
    plt.plot(ms, itl, marker='o', label='IT Latency (ms)')
    plt.title('IT Latency vs Total Requests')
    plt.xlabel('Request length')
    plt.ylabel('IT Latency (ms)')
    plt.grid()
    plt.legend()

    plt.subplot(2, 2, 4)
    plt.plot(ms, tps, marker='o', color='orange', label='TPS (t/sec)')
    plt.title('TPS vs Total Requests')
    plt.xlabel('Request length')
    plt.ylabel('TPS (t/sec)')
    plt.grid()
    plt.legend()

    plt.tight_layout()
    plt.savefig(f"/TRT-LLM/plots/py_many_length.png")


if __name__ == '__main__':
    # main()
    generatePlots()