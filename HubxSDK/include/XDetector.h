
// ============================================================================

/**
 * @file XDetector.h
 * @brief XDetector class - Detector parameter container
 * @version 2.1.0
 */

#ifndef XDETECTOR_H
#define XDETECTOR_H

#include <cstdint>
#include <string>
#include <cstring>

namespace HX {

/**
 * @class XDetector
 * @brief Container for detector parameters
 * 
 * XDetector encapsulates basic detector information:
 * - Network configuration (IP, MAC, ports)
 * - Device identification (serial number)
 * - Hardware specifications (pixels, modules)
 */
class XDetector {
public:
    XDetector()
        : m_cmdPort(3000)
        , m_imgPort(4001)
        , m_pixelCount(0)
        , m_moduleCount(0)
        , m_cardType(0)
    {
        memset(m_mac, 0, 6);
    }
    
    // Network configuration
    void SetIP(const std::string& ip) { m_ip = ip; }
    std::string GetIP() const { return m_ip; }
    
    void SetCmdPort(uint16_t port) { m_cmdPort = port; }
    uint16_t GetCmdPort() const { return m_cmdPort; }
    
    void SetImgPort(uint16_t port) { m_imgPort = port; }
    uint16_t GetImgPort() const { return m_imgPort; }
    
    void SetMAC(const uint8_t* mac) {
        if (mac) memcpy(m_mac, mac, 6);
    }
    const uint8_t* GetMAC() const { return m_mac; }
    
    // Device identification
    void SetSerialNum(const std::string& sn) { m_serialNum = sn; }
    std::string GetSerialNum() const { return m_serialNum; }
    
    // Hardware specifications
    void SetPixelCount(uint32_t count) { m_pixelCount = count; }
    uint32_t GetPixelCount() const { return m_pixelCount; }
    
    void SetModuleCount(uint8_t count) { m_moduleCount = count; }
    uint8_t GetModuleCount() const { return m_moduleCount; }
    
    void SetCardType(uint8_t type) { m_cardType = type; }
    uint8_t GetCardType() const { return m_cardType; }
    
private:
    std::string m_ip;
    uint16_t m_cmdPort;
    uint16_t m_imgPort;
    uint8_t m_mac[6];
    std::string m_serialNum;
    uint32_t m_pixelCount;
    uint8_t m_moduleCount;
    uint8_t m_cardType;
};

} // namespace HX

#endif // XDETECTOR_H
