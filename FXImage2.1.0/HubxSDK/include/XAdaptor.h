/**
 * @file XAdaptor.h
 * @brief Network adapter for detector discovery and configuration
 */

#ifndef XADAPTOR_H
#define XADAPTOR_H

#include <string>
#include <cstdint>

namespace HX {

class XDetector;
class IXCmdSink;

/**
 * @class XAdaptor
 * @brief Manages network communication and detector discovery
 * 
 * XAdaptor handles:
 * - Detector enumeration via broadcast
 * - IP/MAC configuration
 * - Network adapter binding
 */
class XAdaptor {
public:
    /**
     * @brief Default constructor
     */
    XAdaptor();
    
    /**
     * @brief Constructor with adapter IP
     * @param adpIP Local network adapter IP address
     */
    explicit XAdaptor(const std::string& adpIP);
    
    /**
     * @brief Destructor
     */
    ~XAdaptor();
    
    /**
     * @brief Bind to local network adapter
     * @param adpIP IP address of network adapter
     */
    void Bind(const std::string& adpIP);
    
    /**
     * @brief Open adapter for communication
     * @return true on success
     */
    bool Open();
    
    /**
     * @brief Close adapter
     */
    void Close();
    
    /**
     * @brief Check if adapter is open
     * @return true if open
     */
    bool IsOpen();
    
    /**
     * @brief Discover and connect to detectors
     * @return Number of detectors found, -1 on error
     */
    int32_t Connect();
    
    /**
     * @brief Get detector information by index
     * @param index Detector index (0-based)
     * @return XDetector object
     */
    XDetector GetDetector(uint32_t index);
    
    /**
     * @brief Configure detector settings
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
     * @brief Set error/event callback sink
     * @param sink Pointer to callback handler
     */
    void SetSink(IXCmdSink* sink);

private:
    class Impl;
    Impl* pImpl;
    
    // Non-copyable
    XAdaptor(const XAdaptor&) = delete;
    XAdaptor& operator=(const XAdaptor&) = delete;
};

} // namespace HX

#endif // XADAPTOR_H
