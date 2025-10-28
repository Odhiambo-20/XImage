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
    XDetector();
    ~XDetector();
    
    // Network configuration
    void SetIP(const std::string& ip);
    std::string GetIP() const;
    
    void SetCmdPort(uint16_t port);
    uint16_t GetCmdPort() const;
    
    void SetImgPort(uint16_t port);
    uint16_t GetImgPort() const;
    
    void SetMAC(const uint8_t* mac);
    const uint8_t* GetMAC() const;
    
    // Device identification
    void SetSerialNum(const std::string& sn);
    std::string GetSerialNum() const;
    
    // Hardware specifications
    void SetPixelCount(uint32_t count);
    uint32_t GetPixelCount() const;
    
    void SetModuleCount(uint8_t count);
    uint8_t GetModuleCount() const;
    
    void SetCardType(uint8_t type);
    uint8_t GetCardType() const;
    
    void SetPixelSize(uint8_t size);
    uint8_t GetPixelSize() const;
    
    void SetPixelDepth(uint8_t depth);
    uint8_t GetPixelDepth() const;
    
    void SetFirmwareVersion(uint16_t version);
    uint16_t GetFirmwareVersion() const;
    
private:
    std::string m_ip;
    uint16_t m_cmdPort;
    uint16_t m_imgPort;
    uint8_t m_mac[6];
    std::string m_serialNum;
    uint32_t m_pixelCount;
    uint8_t m_moduleCount;
    uint8_t m_cardType;
    uint8_t m_pixelSize;
    uint8_t m_pixelDepth;
    uint16_t m_firmwareVersion;
};

} // namespace HX

#endif // XDETECTOR_H