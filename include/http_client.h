#pragma once

#include <string>
#include <vector>
#include <memory>

namespace mag {

struct HttpResponse {
    std::string data;
    long status_code;
    bool success;
    std::string error_message;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    HttpResponse post(
        const std::string& url,
        const std::string& payload,
        const std::vector<std::string>& headers
    ) const;
    
private:
    void* curl_; // CURL handle
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response);
};

} // namespace mag