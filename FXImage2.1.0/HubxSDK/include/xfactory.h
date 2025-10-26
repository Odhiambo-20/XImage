/**
 * @file xfactory.h
 * @brief Factory class for system resource management
 */

#ifndef XFACTORY_H
#define XFACTORY_H

#include <cstddef>

namespace HX {

/**
 * @class XFactory
 * @brief Manages system resources and object lifecycle
 * 
 * XFactory is responsible for:
 * - Initializing the SDK
 * - Managing memory allocation
 * - Resource cleanup
 * - Reference counting
 */
class XFactory {
public:
    /**
     * @brief Constructor - initializes factory
     */
    XFactory();
    
    /**
     * @brief Destructor - cleans up resources
     */
    ~XFactory();
    
    /**
     * @brief Check if factory is initialized
     * @return true if initialized
     */
    bool IsInitialized() const;
    
    /**
     * @brief Get current reference count
     * @return Number of active references
     */
    int GetReferenceCount() const;
    
    /**
     * @brief Allocate memory buffer
     * @param size Size in bytes
     * @return Pointer to allocated buffer
     */
    void* AllocateBuffer(size_t size);
    
    /**
     * @brief Free allocated buffer
     * @param buffer Pointer to buffer
     */
    void FreeBuffer(void* buffer);
    
    /**
     * @brief Reset factory state
     */
    void Reset();
    
private:
    class Impl;
    Impl* pImpl;
    
    // Non-copyable
    XFactory(const XFactory&) = delete;
    XFactory& operator=(const XFactory&) = delete;
};

} // namespace HX

#endif // XFACTORY_H
