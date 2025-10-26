/**
 * @file xfactory.h
 * @brief XFactory class - System resource management
 * @version 2.1.0
 */

#ifndef XFACTORY_H
#define XFACTORY_H

#include <cstdint>
#include <string>

namespace HX {

/**
 * @class XFactory
 * @brief Manages system resources and initialization
 * 
 * XFactory is responsible for:
 * - Initializing xlibdll proxy
 * - Memory allocation tracking
 * - Resource registry
 * - System-wide cleanup
 */
class XFactory {
public:
    XFactory();
    ~XFactory();
    
    /**
     * @brief Initialize the factory and xlibdll proxy
     * @return true on success
     */
    bool Initialize();
    
    /**
     * @brief Cleanup all resources
     */
    void Cleanup();
    
    /**
     * @brief Check if factory is initialized
     */
    bool IsInitialized() const;
    
    /**
     * @brief Allocate memory with tracking
     * @param size Size in bytes
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* Allocate(size_t size);
    
    /**
     * @brief Free allocated memory
     * @param ptr Pointer to memory
     */
    void Free(void* ptr);
    
    /**
     * @brief Get total allocated memory
     */
    uint64_t GetTotalAllocatedMemory() const;
    
    /**
     * @brief Get number of active allocations
     */
    uint32_t GetAllocationCount() const;
    
    /**
     * @brief Register a named resource
     */
    void RegisterResource(const std::string& name, void* resource);
    
    /**
     * @brief Unregister a named resource
     */
    void UnregisterResource(const std::string& name);
    
    /**
     * @brief Check if resource exists
     */
    bool HasResource(const std::string& name) const;
    
    /**
     * @brief Get resource by name
     */
    void* GetResource(const std::string& name);
    
    /**
     * @brief Print statistics
     */
    void PrintStatistics() const;
    
    /**
     * @brief Get global factory instance (convenience)
     */
    static XFactory& GetGlobalInstance();
    
    /**
     * @brief Destroy global instance
     */
    static void DestroyGlobalInstance();
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XFactory(const XFactory&) = delete;
    XFactory& operator=(const XFactory&) = delete;
};

} // namespace HX

#endif // XFACTORY_H




