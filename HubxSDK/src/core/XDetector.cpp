/**
 * @file XDetector.cpp
 * @brief XDetector implementation - Detector parameter container
 * @version 2.1.0
 */

#include "XDetector.h"
#include <cstring>

namespace HX {

XDetector::XDetector()
    : m_cmdPort(3000)
    , m_imgPort(4001)
    , m_pixelCount(0)
    , m_moduleCount(0)
    , m_cardType(0)
    , m_pixelSize(0)
    , m_pixelDepth(16)
    , m_firmwareVersion(0)
{
    memset(m_mac, 0, 6);
}

XDetector::~XDetector() {
}

void XDetector::SetIP(const std::string& ip) {
    m_ip = ip;
}

std::string XDetector::GetIP() const {
    return m_ip;
}

void XDetector::SetCmdPort(uint16_t port) {
    m_cmdPort = port;
}

uint16_t XDetector::GetCmdPort() const {
    return m_cmdPort;
}

void XDetector::SetImgPort(uint16_t port) {
    m_imgPort = port;
}

uint16_t XDetector::GetImgPort() const {
    return m_imgPort;
}

void XDetector::SetMAC(const uint8_t* mac) {
    if (mac) {
        memcpy(m_mac, mac, 6);
    }
}

const uint8_t* XDetector::GetMAC() const {
    return m_mac;
}

void XDetector::SetSerialNum(const std::string& sn) {
    m_serialNum = sn;
}

std::string XDetector::GetSerialNum() const {
    return m_serialNum;
}

void XDetector::SetPixelCount(uint32_t count) {
    m_pixelCount = count;
}

uint32_t XDetector::GetPixelCount() const {
    return m_pixelCount;
}

void XDetector::SetModuleCount(uint8_t count) {
    m_moduleCount = count;
}

uint8_t XDetector::GetModuleCount() const {
    return m_moduleCount;
}

void XDetector::SetCardType(uint8_t type) {
    m_cardType = type;
}

uint8_t XDetector::GetCardType() const {
    return m_cardType;
}

void XDetector::SetPixelSize(uint8_t size) {
    m_pixelSize = size;
}

uint8_t XDetector::GetPixelSize() const {
    return m_pixelSize;
}

void XDetector::SetPixelDepth(uint8_t depth) {
    m_pixelDepth = depth;
}

uint8_t XDetector::GetPixelDepth() const {
    return m_pixelDepth;
}

void XDetector::SetFirmwareVersion(uint16_t version) {
    m_firmwareVersion = version;
}

uint16_t XDetector::GetFirmwareVersion() const {
    return m_firmwareVersion;
}

} // namespace HX