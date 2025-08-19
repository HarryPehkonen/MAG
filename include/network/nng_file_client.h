#pragma once

#include "interfaces/file_client_interface.h"

namespace mag {

/**
 * @brief NNG-based implementation of file client interface
 * 
 * This class implements the IFileClient interface using NNG sockets
 * to communicate with the file tool service.
 */
class NNGFileClient : public IFileClient {
public:
    /**
     * @brief Constructor - initializes NNG socket connection
     */
    NNGFileClient();
    
    /**
     * @brief Destructor - cleans up NNG socket
     */
    ~NNGFileClient() override;
    
    // IFileClient interface implementation
    DryRunResult dry_run(const WriteFileCommand& command) override;
    ApplyResult apply(const WriteFileCommand& command) override;
    
private:
    void* file_socket_;
    
    void initialize_socket();
    void cleanup_socket();
};

} // namespace mag