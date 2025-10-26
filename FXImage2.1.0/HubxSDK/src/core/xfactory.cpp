/**
 * @file xfactory.cpp
 * @brief Factory class for resource management and object creation
 */

#include "xfactory.h"
#include "xlibdll_interface.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <map>

namespace HX {

class XFactory::Impl {
public:
    std::mutex mutex;
    bool initialized;
    int referenceCount;
    std::map<std::string, void*> resources;
    
    Impl() : initialized(false), referenceCount(0) {}
    
    ~Impl() {
        cleanup();
    }
    
    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (initialized) {
            referenceCount++;
            return true;
        }
        
        // Initialize xlibdll proxy (internal only)
        if (!XLibProxy_Initialize()) {
            std::cerr << "Failed to initialize xlibdll proxy" << std::endl;
            return false;
        }
        
        initialized = true;
        referenceCount = 1;
        
        std::cout << "XFactory initialized successfully" << std::endl;
        return true;
    }
    
    void cleanup() {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (!initialized) {
            return;
        }
        
        referenceCount--;
        if (referenceCount <= 0) {
            // Clean up resources
            for (auto& pair : resources) {
                if (pair.second) {
                    // Resource-specific cleanup would go here
                    delete[] static_cast<char*>(pair.second);
                }
            }
            resources.clear();
            
            // Shutdown xlibdll proxy
            XLibProxy_Shutdown();
            
            initialized = false;
            referenceCount = 0;
            
            std::cout << "XFactory cleaned up" << std::endl;
        }
    }
    
    void* allocateResource(const std::string& name, size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Check if resource already exists
        auto it = resources.find(name);
        if (it != resources.end()) {
            return it->second;
        }
        
        // Allocate new resource
        void* ptr = new char[size];
        if (ptr) {
            resources[name] = ptr;
        }
        
        return ptr;
    }
    
    void freeResource(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto it = resources.find(name);
        if (it != resources.end()) {
            delete[] static_cast<char*>(it->second);
            resources.erase(it);
        }
    }
};

// ============================================================================
// XFactory Implementation
// ============================================================================

XFactory::XFactory() 
    : pImpl(new Impl())
{
    pImpl->initialize();
}

XFactory::~XFactory() {
    pImpl->cleanup();
    delete pImpl;
}

bool XFactory::IsInitialized() const {
    return pImpl->initialized;
}

int XFactory::GetReferenceCount() const {
    return pImpl->referenceCount;
}

void* XFactory::AllocateBuffer(size_t size) {
    static int bufferCounter = 0;
    std::string name = "buffer_" + std::to_string(++bufferCounter);
    return pImpl->allocateResource(name, size);
}

void XFactory::FreeBuffer(void* buffer) {
    if (!buffer) return;
    
    // Find and free the buffer
    for (const auto& pair : pImpl->resources) {
        if (pair.second == buffer) {
            pImpl->freeResource(pair.first);
            break;
        }
    }
}

void XFactory::Reset() {
    pImpl->cleanup();
    pImpl->initialize();
}

} // namespace HX
