/**
 * @file XControl.cpp
 * @brief Command control interface for X-ray detector
 */

#include "XControl.h"
#include "XDetector.h"
#include "XFactory.h"
#include "ixcmd_sink.h"
#include "xlibdll_interface.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <thread>
#include <atomic>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closesocket close
#endif

namespace HX {

class XControl::Impl {
public:
    XDetector detector;
    XFactory* factory;
    IXCmdSink* sink;
    int cmdSocket;
    bool isOpen;
    uint32_t timeout;  // milliseconds
    bool heartbeatEnabled;
    std::atomic<bool> heartbeatRunning;
    std::thread heartbeatThread;
    
    Impl() 
        : factory(nullptr)
        , sink(nullptr)
        , cmdSocket(-1)
        , isOpen(false)
        , timeout(20000)
        , heartbeatEnabled(true)
        , heartbeatRunning(false)
    {}
    
    ~Impl() {
        close();
    }
    
    bool open(XDetector& det) {
        if (isOpen) {
            return true;
        }
        
        detector = det;
        
        // Create command socket
        cmdSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (cmdSocket < 0) {
            notifyError(XERR_CON_OPEN_FAIL, "Failed to create command socket");
            return false;
        }
        
        // Set timeout
        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        setsockopt(cmdSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        
        // Connect via xlibdll proxy
        std::string config = detector.GetIP() + ":" + std::to_string(detector.GetCmdPort());
        int result = XLibProxy_Connect(config.c_str());
        if (result < 0) {
            notifyError(XERR_CON_OPEN_FAIL, "Failed to connect via xlibdll");
            closesocket(cmdSocket);
            cmdSocket = -1;
            return false;
        }
        
        isOpen = true;
        std::cout << "XControl opened successfully" << std::endl;
        
        return true;
    }
    
    void close() {
        // Stop heartbeat
        if (heartbeatRunning) {
            heartbeatRunning = false;
            if (heartbeatThread.joinable()) {
                heartbeatThread.join();
            }
        }
        
        if (cmdSocket >= 0) {
            closesocket(cmdSocket);
            cmdSocket = -1;
        }
        
        isOpen = false;
    }
    
    int32_t sendCommand(const std::string& cmd, std::string& response) {
        if (!isOpen) {
            notifyError(XERR_CON_ENGINE_NOT_OPEN, "XControl not opened");
            return -1;
        }
        
        // Send via xlibdll proxy
        char respBuffer[1024] = {0};
        int result = XLibProxy_SendCommand(cmd.c_str(), respBuffer, sizeof(respBuffer));
        
        if (result < 0) {
            notifyError(XERR_CON_SEND_FAIL, "Command send failed");
            return -1;
        }
        
        response = respBuffer;
        
        // Parse response for errors
        if (response.find("ERR") != std::string::npos) {
            notifyError(XERR_CON_RECV_ERRCODE, response.c_str());
            return -1;
        }
        
        return 1;
    }
    
    std::string buildCommand(XCode code, uint64_t data, uint8_t index) {
        std::ostringstream oss;
        
        // Convert XCode to command string based on command list
        switch (code) {
            case XINT_TIME:
                oss << "[ST,W,0," << data << "]";
                break;
            case XNON_INTTIME:
                oss << "[NT,W,0," << data << "]";
                break;
            case XOPERATION:
                oss << "[OM,W,0," << data << "]";
                break;
            case XDM_GAIN:
                oss << "[SG,W," << (int)index << "," << data << "]";
                break;
            case XBASE_COR:
                oss << "[EO,W," << (int)index << "," << data << "]";
                break;
            case XBASE_LINE:
                oss << "[BS,W," << (int)index << "," << data << "]";
                break;
            case XLED:
                oss << "[LC,W,0," << data << "]";
                break;
            case XSAVE:
                oss << "[IN,S,0]";
                break;
            case XRESTORE:
                oss << "[IN1,L,0]";
                break;
            default:
                oss << "[UNKNOWN]";
                break;
        }
        
        return oss.str();
    }
    
    void startHeartbeat() {
        if (!heartbeatEnabled || heartbeatRunning) {
            return;
        }
        
        heartbeatRunning = true;
        heartbeatThread = std::thread([this]() {
            int missedHeartbeats = 0;
            
            while (heartbeatRunning) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                
                if (!isOpen) {
                    break;
                }
                
                // Send heartbeat request
                std::string response;
                int result = sendCommand("[GI,R,0]", response);
                
                if (result < 0) {
                    missedHeartbeats++;
                    if (missedHeartbeats >= 10) {
                        notifyError(XERR_HEARTBEAT_FAIL, "10 consecutive heartbeats missed");
                        break;
                    }
                } else {
                    missedHeartbeats = 0;
                    // Parse temperature and humidity from response
                    // notifyEvent(XEVT_HEARTBEAT_TEMPRA, temperature);
                }
            }
        });
    }
    
    void notifyError(uint32_t errorCode, const char* message) {
        if (sink) {
            sink->OnXError(errorCode, message);
        } else {
            std::cerr << "XControl Error " << errorCode << ": " << message << std::endl;
        }
    }
    
    void notifyEvent(uint32_t eventCode, float data) {
        if (sink) {
            sink->OnXEvent(eventCode, data);
        }
    }
};

// ============================================================================
// XControl Public Interface
// ============================================================================

XControl::XControl() 
    : pImpl(new Impl())
{
}

XControl::~XControl() {
    Close();
    delete pImpl;
}

bool XControl::Open(XDetector& det) {
    bool result = pImpl->open(det);
    if (result && pImpl->heartbeatEnabled) {
        pImpl->startHeartbeat();
    }
    return result;
}

void XControl::Close() {
    pImpl->close();
}

bool XControl::IsOpen() {
    return pImpl->isOpen;
}

void XControl::SetFactory(XFactory& fac) {
    pImpl->factory = &fac;
}

void XControl::SetSink(IXCmdSink* sink) {
    pImpl->sink = sink;
}

void XControl::SetTimeout(uint32_t time) {
    pImpl->timeout = time;
}

bool XControl::EnableHeartbeat(bool enable) {
    pImpl->heartbeatEnabled = enable;
    if (enable && pImpl->isOpen && !pImpl->heartbeatRunning) {
        pImpl->startHeartbeat();
        return true;
    }
    return true;
}

int32_t XControl::Operate(XCode code, uint64_t data) {
    std::string cmd = pImpl->buildCommand(code, data, 0);
    std::string response;
    return pImpl->sendCommand(cmd, response);
}

int32_t XControl::Read(XCode code, uint64_t& val, uint8_t index) {
    // Build read command
    std::ostringstream oss;
    switch (code) {
        case XINT_TIME:
            oss << "[ST,R,0]";
            break;
        case XPIXEL_NUM:
            oss << "[PN,R,0]";
            break;
        case XCU_VER:
            oss << "[GF,R,0]";
            break;
        default:
            oss << "[UNKNOWN,R,0]";
            break;
    }
    
    std::string response;
    int result = pImpl->sendCommand(oss.str(), response);
    
    if (result > 0) {
        // Parse response to extract value
        val = std::stoull(response);
    }
    
    return result;
}

int32_t XControl::Read(XCode code, std::string& val, uint8_t index) {
    std::ostringstream oss;
    switch (code) {
        case XCU_SN:
            oss << "[GS,R,0]";
            break;
        case XDM_SN:
            oss << "[DS,R," << (int)index << "]";
            break;
        default:
            oss << "[UNKNOWN,R,0]";
            break;
    }
    
    std::string response;
    int result = pImpl->sendCommand(oss.str(), response);
    
    if (result > 0) {
        val = response;
    }
    
    return result;
}

int32_t XControl::Write(XCode code, uint64_t val, uint8_t index) {
    std::string cmd = pImpl->buildCommand(code, val, index);
    std::string response;
    return pImpl->sendCommand(cmd, response);
}

} // namespace HX
