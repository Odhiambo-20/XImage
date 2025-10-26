/**
 * @file XAdaptor.cpp
 * @brief Network adapter for detector discovery and configuration
 */

#include "XAdaptor.h"
#include "XDetector.h"
#include "ixcmd_sink.h"
#include "xlibdll_interface.h"
#include <iostream>
#include <cstring>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closesocket close
#endif

namespace HX {

class XAdaptor::Impl {
public:
    std::string adapterIP;
    int socket;
    bool isOpen;
    IXCmdSink* sink;
    std::vector<XDetector> detectors;
    
    Impl() : socket(-1), isOpen(false), sink(nullptr) {}
    
    ~Impl() {
        close();
    }
    
    bool open() {
        if (isOpen) {
            return true;
        }
        
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            notifyError(XERR_ADP_OPEN_FAIL, "WSAStartup failed");
            return false;
        }
#endif
        
        socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket < 0) {
            notifyError(XERR_ADP_OPEN_FAIL, "Failed to create socket");
            return false;
        }
        
        // Set socket options for broadcast
        int broadcast = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, 
                      (const char*)&broadcast, sizeof(broadcast)) < 0) {
            notifyError(XERR_ADP_OPEN_FAIL, "Failed to set broadcast option");
            closesocket(socket);
            return false;
        }
        
        isOpen = true;
        std::cout << "XAdaptor opened successfully" << std::endl;
        return true;
    }
    
    void close() {
        if (socket >= 0) {
            closesocket(socket);
            socket = -1;
        }
        
#ifdef _WIN32
        WSACleanup();
#endif
        
        isOpen = false;
    }
    
    int32_t connect() {
        if (!isOpen) {
            notifyError(XERR_ADP_ENGINE_NOT_OPEN, "XAdaptor not opened");
            return -1;
        }
        
        detectors.clear();
        
        // Broadcast discovery packet
        const char* broadcastMsg = "DISCOVER_DETECTOR";
        sockaddr_in broadcastAddr;
        memset(&broadcastAddr, 0, sizeof(broadcastAddr));
        broadcastAddr.sin_family = AF_INET;
        broadcastAddr.sin_port = htons(3000);  // Default command port
        broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
        
        if (sendto(socket, broadcastMsg, strlen(broadcastMsg), 0,
                  (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
            notifyError(XERR_ADP_SEND_FAIL, "Failed to send broadcast");
            return -1;
        }
        
        // Wait for responses (with timeout)
        fd_set readfds;
        timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        
        char buffer[1024];
        sockaddr_in senderAddr;
        socklen_t senderLen = sizeof(senderAddr);
        
        while (true) {
            FD_ZERO(&readfds);
            FD_SET(socket, &readfds);
            
            int result = select(socket + 1, &readfds, nullptr, nullptr, &timeout);
            if (result <= 0) {
                break;  // Timeout or error
            }
            
            int bytesReceived = recvfrom(socket, buffer, sizeof(buffer) - 1, 0,
                                        (sockaddr*)&senderAddr, &senderLen);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                
                // Parse detector information
                XDetector detector;
                detector.SetIP(inet_ntoa(senderAddr.sin_addr));
                detector.SetCmdPort(3000);
                detector.SetImgPort(4001);
                
                detectors.push_back(detector);
                
                std::cout << "Found detector at: " << detector.GetIP() << std::endl;
            }
        }
        
        return static_cast<int32_t>(detectors.size());
    }
    
    void notifyError(uint32_t errorCode, const char* message) {
        if (sink) {
            sink->OnXError(errorCode, message);
        } else {
            std::cerr << "XAdaptor Error " << errorCode << ": " << message << std::endl;
        }
    }
};

// ============================================================================
// XAdaptor Public Interface
// ============================================================================

XAdaptor::XAdaptor() 
    : pImpl(new Impl())
{
}

XAdaptor::XAdaptor(const std::string& adpIP) 
    : pImpl(new Impl())
{
    pImpl->adapterIP = adpIP;
}

XAdaptor::~XAdaptor() {
    Close();
    delete pImpl;
}

void XAdaptor::Bind(const std::string& adpIP) {
    pImpl->adapterIP = adpIP;
}

bool XAdaptor::Open() {
    return pImpl->open();
}

void XAdaptor::Close() {
    pImpl->close();
}

bool XAdaptor::IsOpen() {
    return pImpl->isOpen;
}

int32_t XAdaptor::Connect() {
    return pImpl->connect();
}

XDetector XAdaptor::GetDetector(uint32_t index) {
    if (index < pImpl->detectors.size()) {
        return pImpl->detectors[index];
    }
    return XDetector();
}

int32_t XAdaptor::ConfigDetector(const XDetector& det) {
    if (!pImpl->isOpen) {
        pImpl->notifyError(XERR_ADP_ENGINE_NOT_OPEN, "XAdaptor not opened");
        return -1;
    }
    
    // Send configuration command to detector
    std::string configCmd = "CONFIG:" + det.GetIP();
    
    // Implementation would send actual configuration here
    std::cout << "Configuring detector: " << det.GetIP() << std::endl;
    
    return 1;
}

int32_t XAdaptor::Restore() {
    if (!pImpl->isOpen) {
        pImpl->notifyError(XERR_ADP_ENGINE_NOT_OPEN, "XAdaptor not opened");
        return -1;
    }
    
    // Send restore to default command
    std::cout << "Restoring detector to defaults" << std::endl;
    
    return 1;
}

void XAdaptor::SetSink(IXCmdSink* sink) {
    pImpl->sink = sink;
}

} // namespace HX
