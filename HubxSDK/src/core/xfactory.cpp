/**
 * @file xfactory.cpp
 * @brief XFactory implementation - System resource allocation and management
 * 
 * XFactory manages all system resources including memory pools, thread pools,
 * and ensures proper initialization/cleanup of the xlibdll proxy.
 * 
 * @copyright Copyright (c) 2024 FXImage Development Team
 * @version 2.1.0
 */

#include "xfactory.h"
#include "xlibdll_wrapper/xlibdll_interface.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <cstring>

namespace HX {

// ============================================================================
// Internal Implementation (PIMPL Pattern)
// ============================================================================

class XFactory::Impl {
public:
    Impl();
    ~Impl();
    
    bool initialize();
    void cleanup();
    bool isInitialized() const { return m_initialized; }
    
    // Memory pool management
    void* allocateMemory(size_t size);
    void freeMemory(void* ptr);
    
    // Statistics
    uint64_t getTotalAllocatedMemory() const { return m_totalAllocated; }
    uint32_t getAllocationCount() const { return m_allocationCount; }
    
    // Resource tracking
    void registerResource(const std::string& name, void* resource);
    void unregisterResource(const std::string& name);
    bool hasResource(const std::string& name) const;
    void* getResource(const std::string& name);

private:
    bool m_initialized;
    mutable std::mutex m_mutex;
    
    // Memory tracking
    struct MemoryBlock {
        size_t size;
        void* ptr;
        uint64_t allocTime;
    };
    std::map<void*, MemoryBlock> m_allocations;
    uint64_t m_totalAllocated;
    uint32_t m_allocationCount;
    
    // Resource registry
    std::map<std::string, void*> m_resources;
    
    // Configuration
    size_t m_maxMemoryLimit;
    bool m_enableMemoryTracking;
};

XFactory::Impl::Impl()
    : m_initialized(false)
    , m_totalAllocated(0)
    , m_allocationCount(0)
    , m_maxMemoryLimit(0) // 0 means unlimited
    , m_enableMemoryTracking(true)
{
}

XFactory::Impl::~Impl() {
    cleanup();
}

bool XFactory::Impl::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        std::cout << "[XFactory] Already initialized" << std::endl;
        return true;
    }
    
    std::cout << "[XFactory] Initializing..." << std::endl;
    
    // Initialize xlibdll proxy - THIS IS CRITICAL
    if (!Internal::XLibProxy_Initialize()) {
        std::cerr << "[XFactory] ERROR: Failed to initialize xlibdll proxy" << std::endl;
        std::cerr << "[XFactory] Make sure xlibdll.dll is in the correct path" << std::endl;
        return false;
    }
    
    std::cout << "[XFactory] xlibdll proxy initialized successfully" << std::endl;
    std::cout << "[XFactory] xlibdll.dll is now hidden and encapsulated" << std::endl;
    
    m_initialized = true;
    m_totalAllocated = 0;
    m_allocationCount = 0;
    
    std::cout << "[XFactory] Initialization complete" << std::endl;
    
    return true;
}

void XFactory::Impl::cleanup() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    std::cout << "[XFactory] Cleaning up..." << std::endl;
    
    // Report memory leaks
    if (!m_allocations.empty()) {
        std::cerr << "[XFactory] WARNING: " << m_allocations.size() 
                  << " memory blocks not freed!" << std::endl;
        std::cerr << "[XFactory] Total leaked memory: " 
                  << m_totalAllocated << " bytes" << std::endl;
        
        // Force cleanup of leaked memory
        for (auto& pair : m_allocations) {
            std::cerr << "[XFactory] Freeing leaked block: " 
                      << pair.second.size << " bytes at " 
                      << pair.first << std::endl;
            free(pair.first);
        }
        m_allocations.clear();
    }
    
    // Clear resource registry
    if (!m_resources.empty()) {
        std::cerr << "[XFactory] WARNING: " << m_resources.size() 
                  << " resources still registered" << std::endl;
        m_resources.clear();
    }
    
    // Cleanup xlibdll proxy
    Internal::XLibProxy_Cleanup();
    std::cout << "[XFactory] xlibdll proxy cleaned up" << std::endl;
    
    m_initialized = false;
    m_totalAllocated = 0;
    m_allocationCount = 0;
    
    std::cout << "[XFactory] Cleanup complete" << std::endl;
}

void* XFactory::Impl::allocateMemory(size_t size) {
    if (size == 0) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Check memory limit
    if (m_maxMemoryLimit > 0 && 
        (m_totalAllocated + size) > m_maxMemoryLimit) {
        std::cerr << "[XFactory] ERROR: Memory limit exceeded" << std::endl;
        return nullptr;
    }
    
    // Allocate memory
    void* ptr = malloc(size);
    if (!ptr) {
        std::cerr << "[XFactory] ERROR: Failed to allocate " 
                  << size << " bytes" << std::endl;
        return nullptr;
    }
    
    // Track allocation
    if (m_enableMemoryTracking) {
        MemoryBlock block;
        block.size = size;
        block.ptr = ptr;
        block.allocTime = std::chrono::system_clock::now().time_since_epoch().count();
        
        m_allocations[ptr] = block;
        m_totalAllocated += size;
        m_allocationCount++;
    }
    
    return ptr;
}

void XFactory::Impl::freeMemory(void* ptr) {
    if (!ptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Update tracking
    if (m_enableMemoryTracking) {
        auto it = m_allocations.find(ptr);
        if (it != m_allocations.end()) {
            m_totalAllocated -= it->second.size;
            m_allocations.erase(it);
        } else {
            std::cerr << "[XFactory] WARNING: Freeing untracked memory at " 
                      << ptr << std::endl;
        }
    }
    
    free(ptr);
}

void XFactory::Impl::registerResource(const std::string& name, void* resource) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_resources.find(name) != m_resources.end()) {
        std::cerr << "[XFactory] WARNING: Resource '" << name 
                  << "' already registered, overwriting" << std::endl;
    }
    
    m_resources[name] = resource;
}

void XFactory::Impl::unregisterResource(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_resources.find(name);
    if (it != m_resources.end()) {
        m_resources.erase(it);
    }
}

bool XFactory::Impl::hasResource(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resources.find(name) != m_resources.end();
}

void* XFactory::Impl::getResource(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_resources.find(name);
    if (it != m_resources.end()) {
        return it->second;
    }
    
    return nullptr;
}

// ============================================================================
// XFactory Public Interface Implementation
// ============================================================================

XFactory::XFactory()
    : m_impl(new Impl())
{
}

XFactory::~XFactory() {
    if (m_impl) {
        m_impl->cleanup();
        delete m_impl;
        m_impl = nullptr;
    }
}

bool XFactory::Initialize() {
    if (!m_impl) {
        return false;
    }
    return m_impl->initialize();
}

void XFactory::Cleanup() {
    if (m_impl) {
        m_impl->cleanup();
    }
}

bool XFactory::IsInitialized() const {
    if (!m_impl) {
        return false;
    }
    return m_impl->isInitialized();
}

void* XFactory::Allocate(size_t size) {
    if (!m_impl) {
        return nullptr;
    }
    return m_impl->allocateMemory(size);
}

void XFactory::Free(void* ptr) {
    if (m_impl) {
        m_impl->freeMemory(ptr);
    }
}

uint64_t XFactory::GetTotalAllocatedMemory() const {
    if (!m_impl) {
        return 0;
    }
    return m_impl->getTotalAllocatedMemory();
}

uint32_t XFactory::GetAllocationCount() const {
    if (!m_impl) {
        return 0;
    }
    return m_impl->getAllocationCount();
}

void XFactory::RegisterResource(const std::string& name, void* resource) {
    if (m_impl) {
        m_impl->registerResource(name, resource);
    }
}

void XFactory::UnregisterResource(const std::string& name) {
    if (m_impl) {
        m_impl->unregisterResource(name);
    }
}

bool XFactory::HasResource(const std::string& name) const {
    if (!m_impl) {
        return false;
    }
    return m_impl->hasResource(name);
}

void* XFactory::GetResource(const std::string& name) {
    if (!m_impl) {
        return nullptr;
    }
    return m_impl->getResource(name);
}

void XFactory::PrintStatistics() const {
    if (!m_impl) {
        return;
    }
    
    std::cout << "\n=== XFactory Statistics ===" << std::endl;
    std::cout << "Initialized: " << (IsInitialized() ? "Yes" : "No") << std::endl;
    std::cout << "Total Allocated Memory: " << GetTotalAllocatedMemory() << " bytes" << std::endl;
    std::cout << "Active Allocations: " << GetAllocationCount() << std::endl;
    std::cout << "==========================\n" << std::endl;
}

// ============================================================================
// Global Factory Instance (Optional convenience)
// ============================================================================

namespace {
    XFactory* g_globalFactory = nullptr;
    std::mutex g_globalMutex;
}

XFactory& XFactory::GetGlobalInstance() {
    std::lock_guard<std::mutex> lock(g_globalMutex);
    
    if (!g_globalFactory) {
        g_globalFactory = new XFactory();
        if (!g_globalFactory->Initialize()) {
            delete g_globalFactory;
            g_globalFactory = nullptr;
            throw std::runtime_error("Failed to initialize global XFactory");
        }
    }
    
    return *g_globalFactory;
}

void XFactory::DestroyGlobalInstance() {
    std::lock_guard<std::mutex> lock(g_globalMutex);
    
    if (g_globalFactory) {
        delete g_globalFactory;
        g_globalFactory = nullptr;
    }
}

} // namespace HX