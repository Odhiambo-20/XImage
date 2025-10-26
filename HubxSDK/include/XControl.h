// ============================================================================

/**
 * @file XControl.h
 * @brief XControl class - Detector command and control
 * @version 2.1.0
 */

#ifndef XCONTROL_H
#define XCONTROL_H

#include <cstdint>
#include <string>

namespace HX {

class XDetector;
class XFactory;
class IXCmdSink;

/**
 * @class XControl
 * @brief Command control interface for X-ray detectors
 * 
 * XControl provides:
 * - Parameter read/write operations
 * - Command execution
 * - Heartbeat monitoring
 * - Error handling
 */
class XControl {
public:
    /**
     * @enum XCode
     * @brief Command codes for detector control
     */
    enum XCode {
        // System operations
        XINIT = 0,              ///< Initialize from flash
        XRESTORE,               ///< Restore defaults
        XSAVE,                  ///< Save settings
        XFRAME_TR_GEN,          ///< Generate frame trigger
        
        // Basic parameters
        XINT_TIME,              ///< Integration time (μs)
        XNON_INTTIME,           ///< Non-continuous integration time
        XOPERATION,             ///< Operation mode
        XDM_GAIN,               ///< DM module gain
        XHL_MODE,               ///< High/Low energy mode
        XCHANNEL,               ///< Channel configuration
        
        // Correction parameters
        XBASE_COR,              ///< Baseline correction enable
        XBASE_LINE,             ///< Baseline value
        XBIN,                   ///< Pixel binning mode
        XAVERAGE,               ///< Row average filter
        XSUM,                   ///< Row sum filter
        XSCALE,                 ///< Output scale
        XOFFSET_AVG,            ///< Offset average filter
        
        // Trigger parameters
        XLINE_TR_MODE,          ///< Line trigger mode
        XLINE_TRIGGER,          ///< Line trigger enable
        XLINE_TR_FINE_DELAY,    ///< Line trigger fine delay
        XLINE_TR_RAW_DELAY,     ///< Line trigger coarse delay
        XFRAME_TR_MODE,         ///< Frame trigger mode
        XFRAME_TRIGGER,         ///< Frame trigger enable
        XFRAME_TR_DELAY,        ///< Frame trigger delay
        XLINE_TR_PARITY,        ///< Line trigger parity
        
        // Device information
        XPIXEL_NUM,             ///< Total pixel count
        XPIXEL_SIZE,            ///< Pixel size (mm * 10)
        XPIXEL_DEPTH,           ///< Bits per pixel
        XCU_VER,                ///< Firmware version
        XDM_VER,                ///< DM firmware version
        XCU_TEST,               ///< Test pattern mode
        XDM_TEST,               ///< DM test mode
        XDM_PIX_NUM,            ///< Pixels per DM
        XDM_TYPE,               ///< DM type
        XLED,                   ///< LED control
        XCU_TYPE,               ///< Communication type
        XCU_SN,                 ///< Serial number (string)
        XDM_SN                  ///< DM serial number (string)
    };
    
    XControl();
    ~XControl();
    
    /**
     * @brief Open connection to detector
     * @param dec Detector configuration
     * @return true on success
     */
    bool Open(XDetector& dec);
    
    /**
     * @brief Close connection
     */
    void Close();
    
    /**
     * @brief Check if connection is open
     */
    bool IsOpen();
    
    /**
     * @brief Execute operation command
     * @param code Command code
     * @param data Command data (optional)
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Operate(XCode code, uint64_t data = 0);
    
    /**
     * @brief Read parameter value
     * @param code Parameter code
     * @param val Output value
     * @param index DM index (0xFF for all, 0 for none)
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Read(XCode code, uint64_t& val, uint8_t index = 0);
    
    /**
     * @brief Read string parameter
     * @param code Parameter code
     * @param val Output string
     * @param index DM index
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Read(XCode code, std::string& val, uint8_t index = 0);
    
    /**
     * @brief Write parameter value
     * @param code Parameter code
     * @param val Value to write
     * @param index DM index (0xFF for all, 0 for none)
     * @return 1 on success, -1 on error, 0 if unsupported
     */
    int32_t Write(XCode code, uint64_t val, uint8_t index = 0);
    
    /**
     * @brief Set event callback sink
     * @param sink_ Callback handler
     */
    void SetSink(IXCmdSink* sink_);
    
    /**
     * @brief Set factory for resource management
     * @param fac XFactory instance
     */
    void SetFactory(XFactory& fac);
    
    /**
     * @brief Set command timeout
     * @param time Timeout in milliseconds
     */
    void SetTimeout(uint32_t time);
    
    /**
     * @brief Enable/disable heartbeat monitoring
     * @param enable true to enable, false to disable
     * @return true on success
     */
    bool EnableHeartbeat(bool enable);
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XControl(const XControl&) = delete;
    XControl& operator=(const XControl&) = delete;
};

} // namespace HX

#endif // XCONTROL_H