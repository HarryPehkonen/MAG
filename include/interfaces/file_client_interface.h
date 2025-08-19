#pragma once

#include "message.h"

namespace mag {

/**
 * @brief Interface for file operations
 * 
 * This interface abstracts the communication with the file tool,
 * making the Coordinator testable and implementation-agnostic.
 */
class IFileClient {
public:
    virtual ~IFileClient() = default;
    
    /**
     * @brief Perform a dry run of a file operation
     * @param command The file operation to simulate
     * @return DryRunResult containing the operation description and status
     * @throws std::runtime_error on communication failure
     */
    virtual DryRunResult dry_run(const WriteFileCommand& command) = 0;
    
    /**
     * @brief Apply a file operation
     * @param command The file operation to execute
     * @return ApplyResult containing the operation result
     * @throws std::runtime_error on communication failure
     */
    virtual ApplyResult apply(const WriteFileCommand& command) = 0;
};

} // namespace mag