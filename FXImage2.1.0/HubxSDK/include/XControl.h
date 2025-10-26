/**
 * @file XControl.h
 * @brief Command control interface for X-ray detector
 */

#ifndef XCONTROL_H
#define XCONTROL_H

#include <string>
#include <cstdint>

namespace HX {

class XDetector;
class XFactory;
class IXCmdSink;

/**
 * @enum XCode
 * @brief Command codes for detector control
 */
enum XCode {
    // Operation commands
    XINIT = 0x1003,
    XRESTORE = 0x1104,
    XSAVE = 0x1003,
    XFRAME_TR_GEN = 0x5700,
    
    // Configuration parameters
    XINT_TIME = 0x2001,
    XNON_INTTIME = 0x2101,
    XOPERATION = 0x2201,
    XDM_GAIN = 0x2301,
    XHL_MODE = 0x2401,
    XCHANNEL = 0x2501,
    
    // Correction settings
    XBASE_COR = 0x3101,
    XBASE_LINE = 0x3501,
    XBIN = 0x4001,
    XAVERAGE = 0x4101,
    XSUM = 0x4201,
    XSCALE = 0x4301,
    
    // Trigger settings
    XLINE_TR_MODE = 0x5001,
    XLINE_TRIGGER = 0x5101,
    XLINE_TR_FINE_DELAY = 0x5201,
    XLINE_TR_RAW_DELAY = 0x5301,
    XFRAME_TR_MODE = 0x5401,
    XFRAME_TRIGGER = 0x5501,
    XFRAME_TR_DELAY = 0x5601,
    XLINE_TR_PARITY = 0x5A01,
    
    // Device information
    XPIXEL_NUM = 0x6402,
    XPIXEL_SIZE = 0x6501,
    XPIXEL_DEPTH = 0x6601,
    XCU_VER = 0x6802,
    XDM_VER = 0x6902,
    XCU_SN = 0x6202,
    XDM_SN = 0x6302,
    
    // Test modes
    XCU_TEST = 0x6A01,
    XDM_TEST = 0x6B01,
    XLED = 0x7501,
    
    // Advanced
    XDM_PIX_NUM = 0x6C01,
    XDM_TYPE = 0x6E01,
    XCU_TYPE = 0x7001
};

/**
 * @class XControl
 * @brief Detector command control interface
 * 
 * XControl manages:
 * - Command transmission
 * - Parameter read/write
 * - Heartbeat monitoring
 * - Error handling
 */
class XControl {
public:
    /**
     * @brief Constructor
     */
    XControl();
    
    /**
     * @brief Destructor
     */
    ~XControl();
    
    /**
     * @brief Open connection to detector
     * @param det Detector configuration
     * @return true on success
     */
    bool Open(XDetector& det);
    
    /**
     * @brief Close connection
     */
    void Close();
    
    /**
     * @brief Check if connection is open
     * @return true if open
     */
    bool IsOpen();
    
    /**
     * @brief Set factory for resource management
     * @param fac XFactory reference
     */
    void SetFactory(XFactory& fac);
    
    /**
     * @brief Set callback sink
     * @param sink Pointer to callback handler
     */
    void SetSink(IXCmdSink* sink);
    
    /**
     * @brief Set command timeout
     * @param time Timeout in milliseconds
     */
    void SetTimeout(uint32_t time);
    
    /**
     * @brief Enable/disable heartbeat
     * @param enable true to enable
     * @return true on success
     */
    bool EnableHeartbeat(bool enable);
    
    /**
     * @brief Execute operation command
     * @param code Command code
     * @param data Command data
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Operate(XCode code, uint64_t data = 0);
    
    /**
     * @brief Read parameter value
     * @param code Parameter code
     * @param val Output value
     * @param index Module index (0xFF for all)
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Read(XCode code, uint64_t& val, uint8_t index = 0);
    
    /**
     * @brief Read string parameter
     * @param code Parameter code
     * @param val Output string
     * @param index Module index
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Read(XCode code, std::string& val, uint8_t index = 0);
    
    /**
     * @brief Write parameter value
     * @param code Parameter code
     * @param val Value to write
     * @param index Module index (0xFF for all)
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Write(XCode code, uint64_t val, uint8_t index = 0);

private:
    class Impl;
    Impl* pImpl;
    
    // Non-copyable
    XControl(const XControl&) = delete;
    XControl& operator=(const XControl&) = delete;
};

// Error codes
enum {
    XERR_ADP_OPEN_FAIL = 1,
    XERR_ADP_BIND_FAIL = 2,
    XERR_ADP_SEND_FAIL = 3,
    XERR_ADP_RECV_TIMEOUT = 4,
    XERR_ADP_RECV_ERRCMD = 5,
    XERR_ADP_RECV_ERRCODE = 6,
    XERR_ADP_ENGINE_NOT_OPEN = 8,
    XERR_ADP_ALLOCATE_FAIL = 9,
    XERR_CON_OPEN_FAIL = 12,
    XERR_CON_BIND_FAIL = 13,
    XERR_CON_SEND_FAIL = 14,
    XERR_CON_RECV_TIMEOUT = 15,
    XERR_CON_RECV_ERRCMD = 16,
    XERR_CON_RECV_ERRCODE = 17,
    XERR_CON_ENGINE_NOT_OPEN = 19,
    XERR_CON_ALLOCATE_FAIL = 20,
    XERR_HEARTBEAT_FAIL = 39,
    XERR_HEARTBEAT_START_FAIL = 40,
    XERR_HEARTBEAT_STOP_ABNORMAL = 41
};

// Event codes
enum {
    XEVT_HEARTBEAT_TEMPRA = 107,
    XEVT_HEARTBEAT_HUMIDITY = 108
};

} // namespace HX

#endif // XCONTROL_H
