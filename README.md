# CS744 (DECS) Course related: C Web Server Creation and Performance Evaluation

**Indian Institute of Technology Bombay**

This repository contains our implementation and performance evaluation of a C-based HTTP server as part of **Assignment 4 of CS744 (DECS) at IIT Bombay**.

The assignment explores how different server architectures handle concurrent HTTP requests. We implemented and tested three request-processing approaches: a sequential server, a thread-per-request server, and a thread-pool server. We then benchmarked these approaches under increasing client concurrency using `wrk` and compared their throughput and response time.

## 1. Objectives

The main objectives of this assignment are:

- Implement an HTTP server in C.
- Support multiple request-processing models.
- Implement and test different HTTP endpoints.
- Evaluate server performance under varying client concurrency.
- Compare throughput and mean response time across server configurations.
- Understand the effects of thread creation, worker-pool size, CPU allocation, and workload complexity on server performance.

## 2. Server Architectures

The server supports the following execution modes:

| Mode | Description |
|---|---|
| `s` | Sequential server that handles requests sequentially. |
| `m` | Thread-per-request server that creates a separate thread for handling each request. |
| `t N` | Thread-pool server that uses `N` worker threads to process requests. |

For the thread-pool mode, we evaluated pool sizes of 1, 2, 4, and 8 workers.

## 3. HTTP Endpoints

We evaluated the server using three functions with different computational requirements.

| Function | Endpoint | Purpose |
|---|---|---|
| Square | `/square?num=3` | Lightweight arithmetic operation. |
| Hello World | `/helloworld?str=Ashwin` | Lightweight string-processing operation. |
| Prime | `/arithmetic/prime?num=1000000007` | CPU-intensive prime-number computation. |

The square and Hello World endpoints represent lightweight workloads, while the prime endpoint helps evaluate how the server behaves under computationally intensive requests.

## 4. Prerequisites

The following tools are required:

- GCC or another compatible C compiler.
- GNU Make.
- Linux operating system with `taskset` support.
- `wrk` HTTP benchmarking tool.
- `curl` for checking server availability.
- Python 3, NumPy, and Matplotlib if using the plotting notebook.

## 5. Building and Running the Server

Clone the repository and enter its directory:

```bash
git clone https://github.com/AnkitB018/cflask.git
cd cflask
```

Build the server using the Makefile:

```bash
make
```

The following commands assume the compiled executable is located at `./bin/cflask`. If the Makefile produces a different output path, use that path instead.

### Start the server

Run one configuration at a time.

**Sequential server:**

```bash
taskset -c 0-2 ./bin/cflask s 8080
```

**Thread-per-request server:**

```bash
taskset -c 0-2 ./bin/cflask m 8080
```

**Thread pool with one worker:**

```bash
taskset -c 0-2 ./bin/cflask t 8080 1
```

**Thread pool with two workers:**

```bash
taskset -c 0-2 ./bin/cflask t 8080 2
```

**Thread pool with four workers:**

```bash
taskset -c 0-2 ./bin/cflask t 8080 4
```

**Thread pool with eight workers:**

```bash
taskset -c 0-2 ./bin/cflask t 8080 8
```

The server listens on port `8080`. Keep its terminal running while testing and stop it with `Ctrl+C` before starting another configuration.

The CPU IDs shown above are the affinity settings used for the benchmark setup. Verify that these CPU IDs are available on the machine before running the commands.

## 6. Testing the Endpoints

With the server running, use `curl` to send requests.

```bash
curl "http://127.0.0.1:8080/square?num=3"
```

```bash
curl "http://127.0.0.1:8080/helloworld?str=Ashwin"
```

```bash
curl "http://127.0.0.1:8080/arithmetic/prime?num=1000000007"
```

These requests provide a quick check that the server is running and that the endpoints respond before starting the benchmarks.

## 7. Performance Benchmarking Using wrk

We used `wrk` to generate HTTP requests and measure server performance.

### Benchmark parameters

| Parameter | Value |
|---|---|
| Load-generator thread count | 1 |
| Test duration | 30 seconds per test |
| Concurrent connections | 1, 2, 4, 8, 16, 32, 64, 128 |
| Server CPU affinity | CPUs 0–2 |
| Load-generator CPU affinity | CPUs 4–7 |
| Server configurations | Sequential, thread-per-request, pools of 1, 2, 4, and 8 workers |

The server and load generator were pinned to separate CPU sets to reduce interference and maintain a consistent testing environment.

We kept the `wrk` thread count fixed at one and varied the number of concurrent connections. Thus, even when the number of connections exceeded four, the load generator continued to use one thread.

The server was restricted to three CPU cores for all configurations, including the four-worker and eight-worker pools. Therefore, the worker count could exceed the number of CPU cores available to the server.

### Example wrk command

```bash
taskset -c 4-7 ./wrk -t 1 -c 8 -d 30s --latency \
  "http://127.0.0.1:8080/square?num=3"
```

Here:

- `taskset -c 4-7` restricts the load generator to CPUs 4–7.
- `-t 1` specifies one load-generator thread.
- `-c 8` specifies eight concurrent connections.
- `-d 30s` runs the benchmark for 30 seconds.
- `--latency` displays latency statistics.
- The URL selects the endpoint being tested.

### Running the benchmark script

The benchmark script is named `run_loadtest.sh`. It expects the compiled `wrk` executable to be present in the **same directory from which the script is run**, because it invokes `./wrk`.

Therefore, first navigate to the directory containing the `wrk` executable and the script. For example, if both are inside the `wrk` directory:

```bash
cd wrk
chmod +x run_loadtest.sh
./run_loadtest.sh
```

Keep the server running in a separate terminal while the script executes.

The script runs the benchmark across the eight connection counts for the selected endpoint and records the complete `wrk` output, including the command used for each test. Configure the output filename and server-mode label before each run so that the results from different server configurations are stored separately.

The benchmark results can then be used to extract throughput and mean response time for plotting.

### Metrics

We measured two primary metrics:

- **Throughput (requests/sec):** The number of requests completed per second, reported by `wrk` as `Requests/sec`.
- **Mean response time (ms):** The average request latency, reported by `wrk` as `Latency Avg`. Values reported in microseconds were converted to milliseconds for comparison.

## 8. Results and Key Observations

The measurements showed that the best-performing server configuration depended on the computational characteristics of the endpoint.

### Square function

The sequential server achieved a peak measured throughput of approximately **9,020 requests/sec**, at 16 concurrent connections. The thread-per-request server peaked at approximately 2,942 requests/sec, while the thread pools did not surpass the sequential server's peak throughput.

This suggests that thread creation, scheduling, and coordination overhead can outweigh the benefits of multithreading for lightweight operations.

### Hello World function

The sequential server again performed best, reaching approximately **9,122 requests/sec** at 16 concurrent connections. The thread-per-request server peaked at approximately 2,924 requests/sec.

The results followed a pattern similar to the square function, suggesting that lightweight handlers can be served efficiently without introducing additional worker-management overhead.

### Prime function

The CPU-intensive prime endpoint showed a different pattern. The four-worker and eight-worker pools reached approximately **7,099 and 7,424 requests/sec**, respectively, at 128 concurrent connections.

These results suggest that worker pools can improve throughput when requests involve substantial computation and can benefit from parallel processing.

### Overall findings

- The sequential server performed best for the two lightweight functions in the recorded measurements.
- The thread-per-request server did not scale as effectively as the other configurations.
- The four-worker and eight-worker pools achieved the highest measured throughput for the CPU-intensive prime function.
- Increasing concurrency did not always improve throughput or reduce response time.
- More worker threads did not always result in better performance, particularly for lightweight requests.

These conclusions apply to the tested implementation and fixed CPU allocation. They should not be interpreted as universal rankings of server architectures.

## 9. Plots and Report

The performance evaluation uses two plots per function:

1. Throughput versus concurrent connections.
2. Mean response time versus concurrent connections.

Each plot compares the six server configurations. The results are organized by function, allowing the effects of workload complexity and concurrency to be compared.

The benchmark output files (`.data`) contain the recorded `wrk` results and commands. The generated plots and written report should be included alongside these files when submitting the completed assignment.

## 10. Limitations

The server and load generator were assigned fixed CPU sets to keep the testing conditions consistent. However, the server was restricted to three CPU cores, even when the pool contained four or eight worker threads. Consequently, the results do not represent the maximum performance achievable with unrestricted CPU resources.

The load generator also used a single thread throughout the experiments. This controlled the load-generation configuration while varying concurrency, but a separate experiment with multiple `wrk` threads could help determine whether the generator itself becomes a bottleneck.

Some measurements exhibited latency spikes and irregular throughput at higher connection counts. Repeated runs would help establish the stability of the results.

## 11. Conclusion

Assignment 4 provided an opportunity to study the relationship between server architecture, concurrency, and workload complexity through implementation and empirical testing.

The experiments showed that a sequential server can perform very well for lightweight operations, while a thread pool can provide substantial throughput improvements for CPU-intensive work. They also demonstrated that thread creation and worker coordination introduce costs, and that increasing the number of threads does not guarantee better performance.

The key takeaway is that server design should be evaluated against the workload it is intended to handle, using both throughput and response time under controlled testing conditions.
