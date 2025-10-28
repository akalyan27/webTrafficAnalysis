Project Goal

To build an ultra-efficient, concurrent processing pipeline for ingesting and analyzing high-volume web server access logs (e.g., Apache Common Log Format or similar). The system separates the fast I/O reading (Producer) from the slow CPU parsing/analysis (Consumers) using a Thread Pool and a SafeQueue (Bounded Buffer).

Web Analysis Metrics Focus

The project is focused on calculating real-time metrics, including:

- Request Counts: Total requests handled.
- Error Rate: Count of 4xx and 5xx status codes.
- Latency: Average and maximum request response times.
- Endpoint Popularity: Tracking hits per unique URL path.

The Concurrent Pipeline Architecture

Component               Role in Web Analysis            Concurrency Mechanism  
Producer (main thread)  Reads raw log lines quickly         std::ifstream
                        from disk (simulating 
                        high-speed I/O).

SafeQueue (SafeQueue.h) Holds raw log lines awaiting     std::mutex & 
                        processing. This is the         std::condition_variable
                        synchronization point. 

Consumers (ThreadPool)  CPU-Intensive Task: Each thread       std::thread
                        pulls a line, performs heavy          std::function
                        parsing (regex, string manipulation), 
                        extracts fields (IP, Status, Path),
                        and updates global metrics.

This design ensures the I/O thread is never blocked by complex data manipulation, maximizing the ingestion rate.