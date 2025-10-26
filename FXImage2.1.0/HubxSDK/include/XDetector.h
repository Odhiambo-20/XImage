/**
 * @file XDetector.h
 * @brief Detector parameter encapsulation
 */

#ifndef XDETECTOR_H
#define XDETECTOR_H

#include <string>
#include <cstdint>

namespace HX {

/**
 * @class XDetector
 * @brief Encapsulates detector basic parameters
 */
class XDetector {
public:
    XDetector() 
        : m_cmdPort(3000)
        , m_imgPort(4001)
        , m_ip("192.168.1.2")
    {
        for (int i = 0; i < 6; i++) {
            m_mac[i] = 0;
        }
    }
    
    void SetIP(const std::string& ip) { m_ip = ip; }
    std::string GetIP() const { return m_ip; }
    
    void SetCmdPort(uint16_t port) { m_cmdPort = port; }
    uint16_t GetCmdPort() const { return m_cmdPort; }
    
    void SetImgPort(uint16_t port) { m_imgPort = port; }
    uint16_t GetImgPort() const { return m_imgPort; }
    
    void SetMAC(const uint8_t* mac) {
        for (int i = 0; i < 6; i++) {
            m_mac[i] = mac[i];
        }
    }
    uint8_t* GetMAC() { return m_mac; }
    
    void SetSerialNum(const std::string& sn) { m_serialNum = sn; }
    std::string GetSerialNum() const { return m_serialNum; }

private:
    std::string m_ip;
    uint16_t m_cmdPort;
    uint16_t m_imgPort;
    uint8_t m_mac[6];
    std::string m_serialNum;
};

} // namespace HX

#endif // XDETECTOR_H
