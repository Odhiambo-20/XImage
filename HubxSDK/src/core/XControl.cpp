/**
 * @file XControl.cpp
 * @brief XControl implementation - Detector command and control
 * 
 * XControl manages:
 * - Command communication with detector
 * - Parameter read/write operations
 * - Heartbeat monitoring
 * - Error handling and reporting
 * 
 * Based on TDI04_8S_Command_List.docx specifications
 * 
 * @copyright Copyright (c) 2024 FXImage Development Team
 * @version 2.1.0
 */

#include "XControl.h"
#include "XDetector.h"
#include "xfactory.h"
#include "ixcmd_sink.h"
#include "xlibdll_wrapper/xlibdll_interface.h"
#include <iostream>
#include <cstring>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

namespace HX {

// ============================================================================
// Command Codes (from TDI04_8S_Command_List)
// ============================================================================

namespace CommandCode {
    // System commands
    const uint8_t SAVE_SETTINGS = 0x10;
    const uint8_t LOAD_SETTINGS = 0x10;
    const uint8_t SAVE_DEFAULT = 0x11;
    const uint8_t LOAD_DEFAULT = 0x11;
    
    // Basic parameters
    const uint8_t INTEGRATION_TIME = 0x20;
    const uint8_t NON_INT_TIME = 0x21;
    const uint8_t OPERATION_MODE = 0x22;
    const uint8_t DM_GAIN = 0x23;
    const uint8_t CHANNEL_CONFIG = 0x25;
    const uint8_t SCAN_CONTROL = 0x27;
    
    // Correction parameters
    const uint8_t ENABLE_GAIN = 0x30;
    const uint8_t ENABLE_OFFSET = 0x31;
    const uint8_t ENABLE_BASELINE = 0x32;
    const uint8_t LOAD_GAIN = 0x33;
    const uint8_t LOAD_OFFSET = 0x34;
    const uint8_t BASELINE_VALUE = 0x35;
    const uint8_t RESET_GAIN = 0x37;
    const uint8_t RESET_OFFSET = 0x38;
    const uint8_t LOAD_PDC_POS = 0x39;
    const uint8_t LOAD_PDC_COEF = 0x3A;
    const uint8_t ENABLE_PDC = 0x3B;
    const uint8_t PDC_POSITION = 0x3C;
    
    // Output parameters
    const uint8_t OUTPUT_SCALE = 0x43;
    
    // Trigger parameters
    const uint8_t LINE_TRIGGER_MODE = 0x50;
    const uint8_t ENABLE_LINE_TRIGGER = 0x51;
    const uint8_t LINE_TRIGGER_FINE_DELAY = 0x52;
    const uint8_t LINE_TRIGGER_RAW_DELAY = 0x53;
    const uint8_t FRAME_TRIGGER_MODE = 0x54;
    const uint8_t ENABLE_FRAME_TRIGGER = 0x55;
    const uint8_t FRAME_TRIGGER_DELAY = 0x56;
    const uint8_t SEND_FRAME_TRIGGER = 0x57;
    const uint8_t TRIGGER_PARITY = 0x5A;
    
    // Device info
    const uint8_t HEARTBEAT_PERIOD = 0x60;
    const uint8_t GCU_SERIAL = 0x62;
    const uint8_t DM_SERIAL = 0x63;
    const uint8_t PIXEL_NUMBER = 0x64;
    const uint8_t PIXEL_SIZE = 0x65;
    const uint8_t INTEGRATION_RANGE = 0x67;
    const uint8_t GCU_FIRMWARE = 0x68;
    const uint8_t DM_FIRMWARE = 0x69;
    const uint8_t TEST_PATTERN = 0x6A;
    const uint8_t DM_TEST_MODE = 0x6B;
    const uint8_t DM_PIXEL_NUM = 0x6C;
    const uint8_t CARD_NUM_PER_DFE = 0x6D;
    const uint8_t CARD_TYPE = 0x6E;
    const uint8_t GCU_INFO = 0x72;
    const uint8_t DM_INFO = 0x73;
    const uint8_t LED_CONTROL = 0x75;
    const uint8_t ENERGY_MODE = 0x7B;
    const uint8_t GAIN_TABLE_ID = 0x7C;
    const uint8_t MTU_SIZE = 0x7E;
}

namespace Operation {
    const uint8_t WRITE = 0x01;
    const uint8_t READ = 0x02;
    const uint8_t EXECUTE = 0x00;
    const uint8_t LOAD = 0x04;
}

// ============================================================================
// Internal Implementation
// ============================================================================

class XControl::Impl {
public:
    Impl();
    ~Impl();
    
    bool open(XDetector& det);
    void close();
    bool isOpen() const { return m_opened; }
    
    int32_t operate(XCode code, uint64_t data);
    int32_t read(XCode code, uint64_t& val, uint8_t index);
    int32_t read(XCode code, std::string& val, uint8_t index);
    int32_t write(XCode code, uint64_t val, uint8_t index);
    
    void setSink(IXCmdSink* sink) { m_sink = sink; }
    void setFactory(XFactory& fac) { m_factory = &fac; }
    void setTimeout(uint32_t time) { m_timeout = time; }
    bool enableHeartbeat(bool enable);
    
private:
    int32_t sendCommand(uint8_t cmd, uint8_t op, uint8_t dmId, 
                       const uint8_t* data, uint8_t dataLen,
                       uint8_t* response, uint32_t* responseLen);
    
    void reportError(uint32_t errorId, const char* message);
    void reportEvent(uint32_t eventId, float data);
    
    void heartbeatThread();
    void startHeartbeat();
    void stopHeartbeat();
    
    XDetector m_detector;
    bool m_opened;
    IXCmdSink* m_sink;
    XFactory* m_factory;
    uint32_t m_timeout;
    
    // Heartbeat monitoring
    bool m_heartbeatEnabled;
    std::atomic<bool> m_heartbeatRunning;
    std::thread m_heartbeatThread;
    std::atomic<int32_t> m_missedHeartbeats;
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_cmdMutex; // Separate mutex for commands
};

XControl::Impl::Impl()
    : m_opened(false)
    , m_sink(nullptr)
    , m_factory(nullptr)
    , m_timeout(20000) // 20 seconds default
    , m_heartbeatEnabled(true)
    , m_heartbeatRunning(false)
    , m_missedHeartbeats(0)
{
}

XControl::Impl::~Impl() {
    close();
}

bool XControl::Impl::open(XDetector& det) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_opened) {
        std::cout << "[XControl] Already opened" << std::endl;
        return true;
    }
    
    std::cout << "[XControl] Opening connection to " << det.GetIP() << std::endl;
    
    // Validate detector info
    if (det.GetIP().empty()) {
        reportError(4, "Invalid detector IP address");
        return false;
    }
    
    // Check if xlibdll proxy is initialized
    if (!Internal::XLibProxy_IsLoaded()) {
        reportError(8, "xlibdll proxy not initialized");
        return false;
    }
    
    // Initialize network connection through proxy
    int32_t result = Internal::XLibProxy_InitNetwork(
        det.GetIP().c_str(),
        det.GetCmdPort()
    );
    
    if (result < 0) {
        const char* errorMsg = Internal::XLibProxy_GetErrorMessage(result);
        reportError(12, errorMsg);
        return false;
    }
    
    m_detector = det;
    m_opened = true;
    m_missedHeartbeats = 0;
    
    std::cout << "[XControl] Connection opened successfully" << std::endl;
    
    // Start heartbeat monitoring if enabled
    if (m_heartbeatEnabled) {
        startHeartbeat();
    }
    
    return true;
}

void XControl::Impl::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        return;
    }
    
    std::cout << "[XControl] Closing connection..." << std::endl;
    
    // Stop heartbeat monitoring
    stopHeartbeat();
    
    // Close network connection
    Internal::XLibProxy_CloseNetwork();
    
    m_opened = false;
    
    std::cout << "[XControl] Connection closed" << std::endl;
}

int32_t XControl::Impl::sendCommand(uint8_t cmd, uint8_t op, uint8_t dmId,
                                     const uint8_t* data, uint8_t dataLen,
                                     uint8_t* response, uint32_t* responseLen) {
    std::lock_guard<std::mutex> lock(m_cmdMutex);
    
    if (!m_opened) {
        reportError(19, "XControl not opened");
        return -1;
    }
    
    // Build command packet
    uint8_t cmdPacket[512];
    uint32_t packetLen = 0;
    
    cmdPacket[packetLen++] = cmd;
    cmdPacket[packetLen++] = op;
    cmdPacket[packetLen++] = dmId;
    cmdPacket[packetLen++] = dataLen;
    
    if (data && dataLen > 0) {
        memcpy(&cmdPacket[packetLen], data, dataLen);
        packetLen += dataLen;
    }
    
    // Send command through proxy
    int32_t result = Internal::XLibProxy_SendCommand(
        cmdPacket, packetLen,
        response, responseLen,
        m_timeout
    );
    
    if (result < 0) {
        const char* errorMsg = Internal::XLibProxy_GetErrorMessage(result);
        reportError(15, errorMsg);
        return -1;
    }
    
    // Verify response
    if (*responseLen < 4) {
        reportError(16, "Invalid response length");
        return -1;
    }
    
    // Check for error code in response
    if (response[2] != 0) { // Error code is at byte 2
        reportError(17, "Device returned error code");
        return -1;
    }
    
    return result;
}

int32_t XControl::Impl::operate(XCode code, uint64_t data) {
    uint8_t response[256];
    uint32_t responseLen = sizeof(response);
    
    switch (code) {
        case XControl::XINIT:
            return sendCommand(CommandCode::LOAD_SETTINGS, Operation::LOAD, 0x00,
                             nullptr, 0, response, &responseLen);
            
        case XControl::XRESTORE:
            return sendCommand(CommandCode::LOAD_DEFAULT, Operation::LOAD, 0x00,
                             nullptr, 0, response, &responseLen);
            
        case XControl::XSAVE:
            return sendCommand(CommandCode::SAVE_SETTINGS, Operation::EXECUTE, 0x00,
                             nullptr, 0, response, &responseLen);
            
        case XControl::XFRAME_TR_GEN:
            return sendCommand(CommandCode::SEND_FRAME_TRIGGER, Operation::EXECUTE, 0x00,
                             nullptr, 0, response, &responseLen);
            
        default:
            reportError(11, "Unsupported operation code");
            return 0;
    }
}

int32_t XControl::Impl::read(XCode code, uint64_t& val, uint8_t index) {
    uint8_t response[256];
    uint32_t responseLen = sizeof(response);
    int32_t result;
    
    switch (code) {
        case XControl::XINT_TIME:
            result = sendCommand(CommandCode::INTEGRATION_TIME, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 8) {
                val = (uint64_t(response[4]) << 24) | (uint64_t(response[5]) << 16) |
                      (uint64_t(response[6]) << 8) | uint64_t(response[7]);
            }
            return result;
            
        case XControl::XNON_INTTIME:
            result = sendCommand(CommandCode::NON_INT_TIME, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 6) {
                val = (uint64_t(response[4]) << 8) | uint64_t(response[5]);
            }
            return result;
            
        case XControl::XOPERATION:
            result = sendCommand(CommandCode::OPERATION_MODE, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                val = response[4];
            }
            return result;
            
        case XControl::XDM_GAIN:
            if (index == 0xFF) {
                reportError(4, "DM index cannot be 0xFF for read");
                return -1;
            }
            result = sendCommand(CommandCode::DM_GAIN, Operation::READ, index,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 6) {
                val = (uint64_t(response[4]) << 8) | uint64_t(response[5]);
            }
            return result;
            
        case XControl::XCHANNEL:
            result = sendCommand(CommandCode::CHANNEL_CONFIG, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 9) {
                val = (uint64_t(response[4]) << 24) | (uint64_t(response[5]) << 16) |
                      (uint64_t(response[6]) << 8) | uint64_t(response[7]);
            }
            return result;
            
        case XControl::XPIXEL_NUM:
            result = sendCommand(CommandCode::PIXEL_NUMBER, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 6) {
                val = (uint64_t(response[4]) << 8) | uint64_t(response[5]);
            }
            return result;
            
        case XControl::XPIXEL_SIZE:
            result = sendCommand(CommandCode::PIXEL_SIZE, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                val = response[4];
            }
            return result;
            
        case XControl::XPIXEL_DEPTH:
            // Pixel depth is typically 16, 18, or 20 bits
            val = 16; // Default, should be read from device
            return 1;
            
        case XControl::XCU_VER:
            result = sendCommand(CommandCode::GCU_FIRMWARE, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 6) {
                val = (uint64_t(response[4]) << 8) | uint64_t(response[5]);
            }
            return result;
            
        case XControl::XLED:
            result = sendCommand(CommandCode::LED_CONTROL, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                val = response[4];
            }
            return result;
            
        case XControl::XLINE_TRIGGER:
            result = sendCommand(CommandCode::ENABLE_LINE_TRIGGER, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                val = response[4];
            }
            return result;
            
        case XControl::XFRAME_TRIGGER:
            result = sendCommand(CommandCode::ENABLE_FRAME_TRIGGER, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 6) {
                val = (uint64_t(response[4]) << 8) | uint64_t(response[5]);
            }
            return result;
            
        default:
            reportError(11, "Unsupported read code");
            return 0;
    }
}

int32_t XControl::Impl::read(XCode code, std::string& val, uint8_t index) {
    uint8_t response[256];
    uint32_t responseLen = sizeof(response);
    int32_t result;
    
    switch (code) {
        case XControl::XCU_SN:
            result = sendCommand(CommandCode::GCU_SERIAL, Operation::READ, 0x00,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                uint8_t strLen = response[3]; // Data length
                if (strLen > 0 && responseLen >= (4 + strLen)) {
                    val = std::string(reinterpret_cast<char*>(&response[4]), strLen);
                }
            }
            return result;
            
        case XControl::XDM_SN:
            if (index == 0xFF) {
                reportError(4, "DM index cannot be 0xFF for read");
                return -1;
            }
            result = sendCommand(CommandCode::DM_SERIAL, Operation::READ, index,
                               nullptr, 0, response, &responseLen);
            if (result > 0 && responseLen >= 5) {
                uint8_t strLen = response[3];
                if (strLen > 0 && responseLen >= (4 + strLen)) {
                    val = std::string(reinterpret_cast<char*>(&response[4]), strLen);
                }
            }
            return result;
            
        default:
            reportError(11, "Unsupported string read code");
            return 0;
    }
}

int32_t XControl::Impl::write(XCode code, uint64_t val, uint8_t index) {
    uint8_t data[8];
    uint8_t dataLen = 0;
    uint8_t response[256];
    uint32_t responseLen = sizeof(response);
    
    switch (code) {
        case XControl::XINT_TIME:
            data[0] = (val >> 24) & 0xFF;
            data[1] = (val >> 16) & 0xFF;
            data[2] = (val >> 8) & 0xFF;
            data[3] = val & 0xFF;
            dataLen = 4;
            return sendCommand(CommandCode::INTEGRATION_TIME, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XNON_INTTIME:
            data[0] = (val >> 8) & 0xFF;
            data[1] = val & 0xFF;
            dataLen = 2;
            return sendCommand(CommandCode::NON_INT_TIME, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XOPERATION:
            data[0] = val & 0xFF;
            dataLen = 1;
            return sendCommand(CommandCode::OPERATION_MODE, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XDM_GAIN:
            if (index == 0xFF) {
                reportError(4, "DM index cannot be 0xFF for write");
                return -1;
            }
            data[0] = (val >> 8) & 0xFF;
            data[1] = val & 0xFF;
            dataLen = 2;
            return sendCommand(CommandCode::DM_GAIN, Operation::WRITE, index,
                             data, dataLen, response, &responseLen);
            
        case XControl::XBASE_LINE:
            data[0] = (val >> 8) & 0xFF;
            data[1] = val & 0xFF;
            dataLen = 2;
            return sendCommand(CommandCode::BASELINE_VALUE, Operation::WRITE, index,
                             data, dataLen, response, &responseLen);
            
        case XControl::XLED:
            data[0] = val & 0xFF;
            dataLen = 1;
            return sendCommand(CommandCode::LED_CONTROL, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XLINE_TRIGGER:
            data[0] = val & 0xFF;
            dataLen = 1;
            return sendCommand(CommandCode::ENABLE_LINE_TRIGGER, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XFRAME_TRIGGER:
            data[0] = (val >> 8) & 0xFF;
            data[1] = val & 0xFF;
            dataLen = 2;
            return sendCommand(CommandCode::ENABLE_FRAME_TRIGGER, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XLINE_TR_MODE:
            data[0] = val & 0xFF;
            dataLen = 1;
            return sendCommand(CommandCode::LINE_TRIGGER_MODE, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        case XControl::XFRAME_TR_MODE:
            data[0] = val & 0xFF;
            dataLen = 1;
            return sendCommand(CommandCode::FRAME_TRIGGER_MODE, Operation::WRITE, 0x00,
                             data, dataLen, response, &responseLen);
            
        default:
            reportError(11, "Unsupported write code");
            return 0;
    }
}

bool XControl::Impl::enableHeartbeat(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_heartbeatEnabled == enable) {
        return true;
    }
    
    m_heartbeatEnabled = enable;
    
    if (m_opened) {
        if (enable) {
            startHeartbeat();
        } else {
            stopHeartbeat();
        }
    }
    
    return true;
}

void XControl::Impl::startHeartbeat() {
    if (m_heartbeatRunning) {
        return;
    }
    
    m_heartbeatRunning = true;
    m_missedHeartbeats = 0;
    
    m_heartbeatThread = std::thread(&Impl::heartbeatThread, this);
    
    std::cout << "[XControl] Heartbeat monitoring started" << std::endl;
}

void XControl::Impl::stopHeartbeat() {
    if (!m_heartbeatRunning) {
        return;
    }
    
    m_heartbeatRunning = false;
    
    if (m_heartbeatThread.joinable()) {
        m_heartbeatThread.join();
    }
    
    std::cout << "[XControl] Heartbeat monitoring stopped" << std::endl;
}

void XControl::Impl::heartbeatThread() {
    std::cout << "[XControl] Heartbeat thread started" << std::endl;
    
    while (m_heartbeatRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!m_heartbeatRunning) {
            break;
        }
        
        // Send heartbeat request (read GCU info)
        uint8_t response[256];
        uint32_t responseLen = sizeof(response);
        
        int32_t result = sendCommand(CommandCode::GCU_INFO, Operation::READ, 0x00,
                                     nullptr, 0, response, &responseLen);
        
        if (result > 0) {
            // Heartbeat successful
            m_missedHeartbeats = 0;
            
            // Parse temperature and humidity if available
            if (responseLen >= 10) {
                float temp = static_cast<float>(response[4] | (response[5] << 8)) / 10.0f;
                float humidity = static_cast<float>(response[6] | (response[7] << 8)) / 10.0f;
                
                reportEvent(107, temp);    // Temperature event
                reportEvent(108, humidity); // Humidity event
            }
        } else {
            // Heartbeat failed
            m_missedHeartbeats++;
            
            if (m_missedHeartbeats >= 10) {
                reportError(39, "Heartbeat failed - 10 consecutive misses");
                std::cerr << "[XControl] WARNING: Connection may be lost" << std::endl;
                m_missedHeartbeats = 0; // Reset to avoid spam
            }
        }
    }
    
    std::cout << "[XControl] Heartbeat thread stopped" << std::endl;
}

void XControl::Impl::reportError(uint32_t errorId, const char* message) {
    std::cerr << "[XControl] ERROR " << errorId << ": " << message << std::endl;
    
    if (m_sink) {
        m_sink->OnXError(errorId, message);
    }
}

void XControl::Impl::reportEvent(uint32_t eventId, float data) {
    if (m_sink) {
        m_sink->OnXEvent(eventId, data);
    }
}

// ============================================================================
// XControl Public Interface Implementation
// ============================================================================

XControl::XControl()
    : m_impl(new Impl())
{
}

XControl::~XControl() {
    if (m_impl) {
        m_impl->close();
        delete m_impl;
        m_impl = nullptr;
    }
}

bool XControl::Open(XDetector& dec) {
    if (!m_impl) {
        return false;
    }
    return m_impl->open(dec);
}

void XControl::Close() {
    if (m_impl) {
        m_impl->close();
    }
}

bool XControl::IsOpen() {
    if (!m_impl) {
        return false;
    }
    return m_impl->isOpen();
}

int32_t XControl::Operate(XCode code, uint64_t data) {
    if (!m_impl) {
        return -1;
    }
    return m_impl->operate(code, data);
}

int32_t XControl::Read(XCode code, uint64_t& val, uint8_t index) {
    if (!m_impl) {
        return -1;
    }
    return m_impl->read(code, val, index);
}

int32_t XControl::Read(XCode code, std::string& val, uint8_t index) {
    if (!m_impl) {
        return -1;
    }
    return m_impl->read(code, val, index);
}

int32_t XControl::Write(XCode code, uint64_t val, uint8_t index) {
    if (!m_impl) {
        return -1;
    }
    return m_impl->write(code, val, index);
}

void XControl::SetSink(IXCmdSink* sink_) {
    if (m_impl) {
        m_impl->setSink(sink_);
    }
}

void XControl::SetFactory(XFactory& fac) {
    if (m_impl) {
        m_impl->setFactory(fac);
    }
}

void XControl::SetTimeout(uint32_t time) {
    if (m_impl) {
        m_impl->setTimeout(time);
    }
}

bool XControl::EnableHeartbeat(bool enable) {
    if (!m_impl) {
        return false;
    }
    return m_impl->enableHeartbeat(enable);
}

} // namespace HX