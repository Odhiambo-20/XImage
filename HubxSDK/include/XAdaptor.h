// ============================================================================

/**
 * @file XAdaptor.h
 * @brief XAdaptor class - Network discovery and configuration
 * @version 2.1.0
 */

#ifndef XADAPTOR_H
#define XADAPTOR_H

#include <cstdint>
#include <string>

namespace HX {

class XDetector;
class IXCmdSink;

/**
 * @class XAdaptor
 * @brief Network adapter for detector discovery and configuration
 * 
 * XAdaptor provides:
 * - Device discovery via broadcast
 * - IP/MAC/Port configuration
 * - Device enumeration
 */
class XAdaptor {
public:
    XAdaptor();
    explicit XAdaptor(const std::string& adp_ip);
    ~XAdaptor();
    
    /**
     * @brief Bind to local network adapter IP
     * @param adp_ip Local IP address
     */
    void Bind(const std::string& adp_ip);
    
    /**
     * @brief Open the adapter
     * @return true on success
     */
    bool Open();
    
    /**
     * @brief Close the adapter
     */
    void Close();
    
    /**
     * @brief Check if adapter is open
     */
    bool IsOpen();
    
    /**
     * @brief Discover detectors on network
     * @return Number of devices found, or -1 on error
     */
    int32_t Connect();
    
    /**
     * @brief Get discovered detector information
     * @param index Device index (0-based)
     * @return XDetector object
     */
    XDetector GetDetector(uint32_t index);
    
    /**
     * @brief Configure detector network settings
     * @param det Detector configuration
     * @return 1 on success, -1 on error
     */
    int32_t ConfigDetector(const XDetector& det);
    
    /**
     * @brief Restore detector to default settings
     * @return 1 on success, -1 on error
     */
    int32_t Restore();
    
    /**
     * @brief Set event callback sink
     * @param sink_ Callback handler
     */
    void SetSink(IXCmdSink* sink_);
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XAdaptor(const XAdaptor&) = delete;
    XAdaptor& operator=(const XAdaptor&) = delete;
};

} // namespace HX

#endif // XADAPTOR_H