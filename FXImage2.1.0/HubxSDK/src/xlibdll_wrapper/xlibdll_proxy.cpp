/**
 * @file xlibdll_proxy.cpp
 * @brief Proxy/wrapper layer for xlibdll.dll - encapsulates and hides direct access
 * @note xlibdll.dll cannot be called directly by external applications
 */

#include "xlibdll_interface.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
    #define LOAD_LIBRARY(name) LoadLibraryA(name)
    #define GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
    #define FREE_LIBRARY(handle) FreeLibrary(handle)
    typedef HMODULE LibHandle;
#else
    #include <dlfcn.h>
    #define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
    #define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
    #define FREE_LIBRARY(handle) dlclose(handle)
    typedef void* LibHandle;
#endif

namespace HX {
namespace Internal {

/**
 * @class XLibDllProxy
 * @brief Internal proxy class for xlibdll.dll - NOT FOR EXTERNAL USE
 */
class XLibDllProxy {
private:
    LibHandle m_handle;
    bool m_loaded;
    
    // Function pointers to xlibdll.dll functions
    typedef int (*InitFunc)(const char*);
    typedef int (*SendCommandFunc)(const char*, char*, int);
    typedef int (*ReceiveDataFunc)(unsigned char*, int);
    typedef int (*CloseFunc)();
    
    InitFunc m_init;
    SendCommandFunc m_sendCommand;
    ReceiveDataFunc m_receiveData;
    CloseFunc m_close;

public:
    XLibDllProxy() 
        : m_handle(nullptr)
        , m_loaded(false)
        , m_init(nullptr)
        , m_sendCommand(nullptr)
        , m_receiveData(nullptr)
        , m_close(nullptr)
    {
    }
    
    ~XLibDllProxy() {
        unload();
    }
    
    /**
     * @brief Load xlibdll.dll and resolve function pointers
     * @param dllPath Path to xlibdll.dll
     * @return true on success, false on failure
     */
    bool load(const char* dllPath = nullptr) {
        if (m_loaded) {
            return true;
        }
        
        const char* libName = dllPath;
        if (!libName) {
#ifdef _WIN32
            libName = "xlibdll.dll";
#else
            libName = "libxlib.so";
#endif
        }
        
        m_handle = LOAD_LIBRARY(libName);
        if (!m_handle) {
            std::cerr << "Failed to load xlibdll: " << libName << std::endl;
            return false;
        }
        
        // Resolve function pointers
        m_init = (InitFunc)GET_PROC_ADDRESS(m_handle, "xlib_init");
        m_sendCommand = (SendCommandFunc)GET_PROC_ADDRESS(m_handle, "xlib_send_command");
        m_receiveData = (ReceiveDataFunc)GET_PROC_ADDRESS(m_handle, "xlib_receive_data");
        m_close = (CloseFunc)GET_PROC_ADDRESS(m_handle, "xlib_close");
        
        if (!m_init || !m_sendCommand || !m_receiveData || !m_close) {
            std::cerr << "Failed to resolve xlibdll functions" << std::endl;
            FREE_LIBRARY(m_handle);
            m_handle = nullptr;
            return false;
        }
        
        m_loaded = true;
        std::cout << "xlibdll loaded successfully (internal use only)" << std::endl;
        return true;
    }
    
    /**
     * @brief Unload xlibdll.dll
     */
    void unload() {
        if (m_handle) {
            if (m_close) {
                m_close();
            }
            FREE_LIBRARY(m_handle);
            m_handle = nullptr;
            m_loaded = false;
        }
    }
    
    /**
     * @brief Initialize connection via xlibdll
     * @param config Configuration string
     * @return Status code
     */
    int initialize(const char* config) {
        if (!m_loaded || !m_init) {
            return -1;
        }
        return m_init(config);
    }
    
    /**
     * @brief Send command via xlibdll
     * @param command Command string
     * @param response Response buffer
     * @param responseSize Response buffer size
     * @return Status code
     */
    int sendCommand(const char* command, char* response, int responseSize) {
        if (!m_loaded || !m_sendCommand) {
            return -1;
        }
        return m_sendCommand(command, response, responseSize);
    }
    
    /**
     * @brief Receive data via xlibdll
     * @param buffer Data buffer
     * @param bufferSize Buffer size
     * @return Number of bytes received or error code
     */
    int receiveData(unsigned char* buffer, int bufferSize) {
        if (!m_loaded || !m_receiveData) {
            return -1;
        }
        return m_receiveData(buffer, bufferSize);
    }
    
    bool isLoaded() const {
        return m_loaded;
    }
};

// Global singleton instance - internal use only
static XLibDllProxy g_xlibProxy;

} // namespace Internal

// ============================================================================
// Public proxy interface - These are the ONLY functions that should be used
// ============================================================================

bool XLibProxy_Initialize(const char* dllPath) {
    return Internal::g_xlibProxy.load(dllPath);
}

void XLibProxy_Shutdown() {
    Internal::g_xlibProxy.unload();
}

int XLibProxy_Connect(const char* config) {
    return Internal::g_xlibProxy.initialize(config);
}

int XLibProxy_SendCommand(const char* command, char* response, int responseSize) {
    return Internal::g_xlibProxy.sendCommand(command, response, responseSize);
}

int XLibProxy_ReceiveData(unsigned char* buffer, int bufferSize) {
    return Internal::g_xlibProxy.receiveData(buffer, bufferSize);
}

bool XLibProxy_IsLoaded() {
    return Internal::g_xlibProxy.isLoaded();
}

} // namespace HX
