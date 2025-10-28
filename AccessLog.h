/*
Defines the structured data model, AccessLog, which 
holds the parsed metrics (IP, Method, Path, Latency, etc.)
*/

#pragma once 

#include <string>
#include <chrono>


// Object created by the worker threads after CPU-intensive parsing.

struct AccessLog {
    std::time_t timestamp = 0;
    std::string ip_address;     
    std::string request_method;     // e.g., "GET", "POST", "PUT"
    std::string request_path;       // uniform resource identifier 
    int status_code = 0;            // HTTP status code (200, 404, 503, etc.)
    long response_time_ms = 0;      
    long content_length_bytes = 0;  

    bool is_error() const {
        return status_code >= 400;
    }
};