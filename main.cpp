/*
Main application entry point, where we:
1. Initialize the metrics
2. Set up the logging infrastructure
3. Create the producer thread
4. Create the worker threads
5. Wait for the producer to finish and join all threads
6. Print the final metrics
*/