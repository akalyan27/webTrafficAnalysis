Project Goal

To build an ultra-efficient, concurrent processing pipeline for ingesting and analyzing high-volume web server access logs (e.g., Apache Common Log Format or similar). The system separates the fast I/O reading (Producer) from the slow CPU parsing/analysis (Consumers) using a Thread Pool and a SafeQueue (Bounded Buffer).

Web Analysis Metrics Focus

The project is focused on calculating real-time metrics, including:

- Request Counts: Total requests handled.
- Error Rate: Count of 4xx and 5xx status codes.
- Latency: Average and maximum request response times.
- Endpoint Popularity: Tracking hits per unique URL path.

The Concurrent Pipeline Architecture

Component 
  - Producer (main thread): Reads raw log lines quickly from disk,  std::ifstream
  - SafeQueue (SafeQueue.h): Holds raw log lines awaiting processing. This is the synchronization point, using std::mutex & std::condition_variable
  - Consumers (ThreadPool): CPU-Intensive Task: Each thread pulls a line, parses (regex, string manipulation), extracts fields (IP, Status, Path), and updates global metrics       

This design ensures the I/O thread is never blocked by complex data manipulation, maximizing the ingestion rate.
