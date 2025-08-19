#include "http_client.h"
#include <curl/curl.h>
#include <stdexcept>

namespace mag {

HttpClient::HttpClient() {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

HttpClient::~HttpClient() {
    if (curl_) {
        curl_easy_cleanup(static_cast<CURL*>(curl_));
    }
}

size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

HttpResponse HttpClient::post(
    const std::string& url,
    const std::string& payload,
    const std::vector<std::string>& headers
) const {
    HttpResponse response;
    response.success = false;
    response.status_code = 0;
    
    CURL* curl = static_cast<CURL*>(curl_);
    
    // Reset curl for new request
    curl_easy_reset(curl);
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Set POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.length());
    
    // Set headers
    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    
    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.data);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Get response code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
    
    // Cleanup headers
    if (header_list) {
        curl_slist_free_all(header_list);
    }
    
    if (res != CURLE_OK) {
        response.error_message = curl_easy_strerror(res);
        response.success = false;
    } else if (response.status_code >= 200 && response.status_code < 300) {
        response.success = true;
    } else {
        response.success = false;
        response.error_message = "HTTP error: " + std::to_string(response.status_code);
    }
    
    return response;
}

} // namespace mag