/**
 * @file XAdaptor.cpp
 * @brief XAdaptor implementation - Network discovery and device configuration
 * 
 * XAdaptor handles:
 * - Device discovery via broadcast
 * - Network configuration (IP, MAC, ports)
 * - Device enumeration
 * - Initial connection establishment
 * 
 * @copyright Copyright (c) 2024 FXImage Development Team
 * @version 2.1.0
 */

#include "XAdaptor.h"
#include "XDetector.h"
#include "ixcmd_sink.h"
#include "xlibdll_wrapper/xlibdll_interface.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <mutex>
#include <chrono>
#include <thread>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace HX {

// ============================================================================
// Internal Implementation
// ============================================================================

class XAdaptor::Impl {
public:
    Impl();
    Impl(const std::string& adapterIP);
    ~Impl();
    
    void setAdapterIP(const std::string& ip);
    bool open();
    void close();
    bool isOpen() const { return m_opened; }
    
    int32_t connect();
    XDetector getDetector(uint32_t index);
    int32_t configDetector(const XDetector& det);
    int32_t restore();
    
    void setSink(IXCmdSink* sink) { m_sink = sink; }
    
private:
    void reportError(uint32_t errorId, const char* message);
    void reportEvent(uint32_t eventId, float data);
    
    bool validateIP(const std::string& ip) const;
    bool initializeNetwork();
    void cleanupNetwork();
    
    std::string m_adapterIP;
    bool m_opened;
    bool m_networkInitialized;
    IXCmdSink* m_sink;
    
    // Discovered devices
    std::vector<Internal::XLibDeviceInfo> m_discoveredDevices;
    mutable std::mutex m_mutex;
    
    // Network state
    int m_broadcastSocket;
};

XAdaptor::Impl::Impl()
    : m_opened(false)
    , m_networkInitialized(false)
    , m_sink(nullptr)
    , m_broadcastSocket(-1)
{
}

XAdaptor::Impl::Impl(const std::string& adapterIP)
    : m_adapterIP(adapterIP)
    , m_opened(false)
    , m_networkInitialized(false)
    , m_sink(nullptr)
    , m_broadcastSocket(-1)
{
}

XAdaptor::Impl::~Impl() {
    close();
}

void XAdaptor::Impl::setAdapterIP(const std::string& ip) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_opened) {
        reportError(1, "Cannot change adapter IP while open");
        return;
    }
    
    if (!validateIP(ip)) {
        reportError(4, "Invalid IP address format");
        return;
    }
    
    m_adapterIP = ip;
}

bool XAdaptor::Impl::validateIP(const std::string& ip) const {
    if (ip.empty()) {
        return false;
    }
    
    return Internal::XLib_ValidateIP(ip.c_str());
}

bool XAdaptor::Impl::initializeNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        reportError(2, "Failed to initialize Winsock");
        return false;
    }
#endif
    
    m_networkInitialized = true;
    return true;
}

void XAdaptor::Impl::cleanupNetwork() {
#ifdef _WIN32
    if (m_networkInitialized) {
        WSACleanup();
    }
#endif
    m_networkInitialized = false;
}

bool XAdaptor::Impl::open() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_opened) {
        std::cout << "[XAdaptor] Already opened" << std::endl;
        return true;
    }
    
    std::cout << "[XAdaptor] Opening..." << std::endl;
    
    // Validate adapter IP
    if (m_adapterIP.empty()) {
        reportError(4, "Adapter IP not set");
        return false;
    }
    
    if (!validateIP(m_adapterIP)) {
        reportError(4, "Invalid adapter IP address");
        return false;
    }
    
    // Initialize network stack
    if (!m_networkInitialized && !initializeNetwork()) {
        reportError(2, "Failed to initialize network");
        return false;
    }
    
    // Check if xlibdll proxy is initialized
    if (!Internal::XLibProxy_IsLoaded()) {
        reportError(8, "xlibdll proxy not initialized. Call XFactory::Initialize() first");
        return false;
    }
    
    m_opened = true;
    m_discoveredDevices.clear();
    
    std::cout << "[XAdaptor] Opened successfully on " << m_adapterIP << std::endl;
    
    return true;
}

void XAdaptor::Impl::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        return;
    }
    
    std::cout << "[XAdaptor] Closing..." << std::endl;
    
    m_discoveredDevices.clear();
    cleanupNetwork();
    
    m_opened = false;
    
    std::cout << "[XAdaptor] Closed" << std::endl;
}

int32_t XAdaptor::Impl::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        reportError(8, "XAdaptor not opened");
        return -1;
    }
    
    std::cout << "[XAdaptor] Discovering devices on " << m_adapterIP << "..." << std::endl;
    
    // Clear previous discoveries
    m_discoveredDevices.clear();
    
    // Use xlibdll proxy to discover devices
    int32_t deviceCount = Internal::XLibProxy_DiscoverDevices(m_adapterIP.c_str());
    
    if (deviceCount < 0) {
        const char* errorMsg = Internal::XLibProxy_GetErrorMessage(deviceCount);
        reportError(5, errorMsg);
        return -1;
    }
    
    std::cout << "[XAdaptor] Found " << deviceCount << " device(s)" << std::endl;
    
    // Retrieve information for each discovered device
    for (int32_t i = 0; i < deviceCount; ++i) {
        Internal::XLibDeviceInfo deviceInfo;
        
        if (Internal::XLibProxy_GetDeviceInfo(i, &deviceInfo) == 0) {
            m_discoveredDevices.push_back(deviceInfo);
            
            char macStr[18];
            Internal::XLib_MACToString(deviceInfo.mac, macStr);
            
            std::cout << "[XAdaptor] Device " << (i + 1) << ": " 
                      << deviceInfo.ip << " (MAC: " << macStr << ")" << std::endl;
        }
    }
    
    reportEvent(101, static_cast<float>(deviceCount));
    
    return static_cast<int32_t>(m_discoveredDevices.size());
}

XDetector XAdaptor::Impl::getDetector(uint32_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    XDetector detector;
    
    if (index >= m_discoveredDevices.size()) {
        reportError(5, "Device index out of range");
        return detector;
    }
    
    const Internal::XLibDeviceInfo& info = m_discoveredDevices[index];
    
    // Convert XLibDeviceInfo to XDetector
    detector.SetIP(info.ip);
    detector.SetCmdPort(info.cmdPort);
    detector.SetImgPort(info.imgPort);
    detector.SetMAC(info.mac);
    detector.SetSerialNum(info.serialNumber);
    detector.SetPixelCount(info.pixelCount);
    detector.SetModuleCount(info.moduleCount);
    detector.SetCardType(info.cardType);
    
    return detector;
}

int32_t XAdaptor::Impl::configDetector(const XDetector& det) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        reportError(8, "XAdaptor not opened");
        return -1;
    }
    
    std::cout << "[XAdaptor] Configuring detector..." << std::endl;
    
    // Validate parameters
    if (!validateIP(det.GetIP())) {
        reportError(4, "Invalid detector IP address");
        return -1;
    }
    
    const uint8_t* mac = det.GetMAC();
    if (!mac) {
        reportError(4, "Invalid MAC address");
        return -1;
    }
    
    char macStr[18];
    Internal::XLib_MACToString(mac, macStr);
    
    std::cout << "[XAdaptor] Configuring device:" << std::endl;
    std::cout << "  MAC: " << macStr << std::endl;
    std::cout << "  IP: " << det.GetIP() << std::endl;
    std::cout << "  Command Port: " << det.GetCmdPort() << std::endl;
    std::cout << "  Image Port: " << det.GetImgPort() << std::endl;
    
    // Configure through xlibdll proxy
    int32_t result = Internal::XLibProxy_ConfigureDevice(
        mac,
        det.GetIP().c_str(),
        det.GetCmdPort(),
        det.GetImgPort()
    );
    
    if (result < 0) {
        const char* errorMsg = Internal::XLibProxy_GetErrorMessage(result);
        reportError(6, errorMsg);
        return -1;
    }
    
    std::cout << "[XAdaptor] Device configured successfully" << std::endl;
    std::cout << "[XAdaptor] Please wait for device to reboot..." << std::endl;
    
    // Wait for device reboot
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    return 1;
}

int32_t XAdaptor::Impl::restore() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        reportError(8, "XAdaptor not opened");
        return -1;
    }
    
    if (m_discoveredDevices.empty()) {
        reportError(5, "No devices discovered");
        return -1;
    }
    
    std::cout << "[XAdaptor] Restoring default settings for all devices..." << std::endl;
    
    int32_t successCount = 0;
    
    for (const auto& device : m_discoveredDevices) {
        char macStr[18];
        Internal::XLib_MACToString(device.mac, macStr);
        
        std::cout << "[XAdaptor] Restoring device " << macStr << "..." << std::endl;
        
        int32_t result = Internal::XLibProxy_ResetDevice(device.mac);
        
        if (result == 0) {
            successCount++;
            std::cout << "[XAdaptor] Device restored successfully" << std::endl;
        } else {
            const char* errorMsg = Internal::XLibProxy_GetErrorMessage(result);
            std::cerr << "[XAdaptor] Failed to restore device: " << errorMsg << std::endl;
        }
    }
    
    if (successCount > 0) {
        std::cout << "[XAdaptor] Restored " << successCount << " device(s)" << std::endl;
        std::cout << "[XAdaptor] Default IP: 192.168.1.2" << std::endl;
        std::cout << "[XAdaptor] Default Cmd Port: 3000" << std::endl;
        std::cout << "[XAdaptor] Default Img Port: 4001" << std::endl;
        
        // Wait for devices to reboot
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    
    return (successCount > 0) ? 1 : -1;
}

void XAdaptor::Impl::reportError(uint32_t errorId, const char* message) {
    std::cerr << "[XAdaptor] ERROR " << errorId << ": " << message << std::endl;
    
    if (m_sink) {
        m_sink->OnXError(errorId, message);
    }
}

void XAdaptor::Impl::reportEvent(uint32_t eventId, float data) {
    if (m_sink) {
        m_sink->OnXEvent(eventId, data);
    }
}

// ============================================================================
// XAdaptor Public Interface Implementation
// ============================================================================

XAdaptor::XAdaptor()
    : m_impl(new Impl())
{
}

XAdaptor::XAdaptor(const std::string& adp_ip)
    : m_impl(new Impl(adp_ip))
{
}

XAdaptor::~XAdaptor() {
    if (m_impl) {
        m_impl->close();
        delete m_impl;
        m_impl = nullptr;
    }
}

void XAdaptor::Bind(const std::string& adp_ip) {
    if (m_impl) {
        m_impl->setAdapterIP(adp_ip);
    }
}

bool XAdaptor::Open() {
    if (!m_impl) {
        return false;
    }
    return m_impl->open();
}

void XAdaptor::Close() {
    if (m_impl) {
        m_impl->close();
    }
}

bool XAdaptor::IsOpen() {
    if (!m_impl) {
        return false;
    }
    return m_impl->isOpen();
}

int32_t XAdaptor::Connect() {
    if (!m_impl) {
        return -1;
    }
    return m_impl->connect();
}

XDetector XAdaptor::GetDetector(uint32_t index) {
    if (!m_impl) {
        return XDetector();
    }
    return m_impl->getDetector(index);
}

int32_t XAdaptor::ConfigDetector(const XDetector& det) {
    if (!m_impl) {
        return -1;
    }
    return m_impl->configDetector(det);
}

int32_t XAdaptor::Restore() {
    if (!m_impl) {
        return -1;
    }
    return m_impl->restore();
}

void XAdaptor::SetSink(IXCmdSink* sink_) {
    if (m_impl) {
        m_impl->setSink(sink_);
    }
}

} // namespace HX