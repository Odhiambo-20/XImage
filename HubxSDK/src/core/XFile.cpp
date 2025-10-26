
// ============================================================================
// XFile.cpp - TIFF file operations
// ============================================================================

/**
 * @file XFile.cpp
 * @brief XFile implementation - TIFF image file operations
 * @version 2.1.0
 */

#include "XFile.h"
#include "XImage.h"
#include "XDetector.h"
#include <fstream>
#include <cstring>
#include <ctime>
#include <iostream>

namespace HX {

class XFile::Impl {
public:
    Impl();
    Impl(XImage* image, XDetector& det);
    ~Impl();
    
    bool read(const std::string& file);
    bool write(const std::string& file);
    
    bool get(XFCode code, uint32_t& data);
    bool get(XFCode code, float& data);
    bool get(XFCode code, uint8_t** data_);
    
    bool set(XFCode code, uint32_t data);
    bool set(XFCode code, float data);
    bool set(XFCode code, uint8_t* data_);
    
private:
    bool writeTIFFHeader(std::ofstream& file);
    bool readTIFFHeader(std::ifstream& file);
    
    XImage* m_image;
    
    // Metadata
    uint32_t m_cols;
    uint32_t m_rows;
    uint32_t m_depth;
    uint32_t m_dmNum;
    uint32_t m_dmType;
    uint32_t m_dmPix;
    uint32_t m_opMode;
    uint32_t m_intTime;
    uint32_t m_energy;
    uint32_t m_bin;
    float m_temp;
    float m_humidity;
    std::string m_serialNum;
    std::string m_dateTime;
};

XFile::Impl::Impl()
    : m_image(nullptr)
    , m_cols(0)
    , m_rows(0)
    , m_depth(16)
    , m_dmNum(0)
    , m_dmType(0)
    , m_dmPix(0)
    , m_opMode(0)
    , m_intTime(0)
    , m_energy(0)
    , m_bin(0)
    , m_temp(0.0f)
    , m_humidity(0.0f)
{
    // Get current date/time
    time_t now = time(nullptr);
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    m_dateTime = timeStr;
}

XFile::Impl::Impl(XImage* image, XDetector& det)
    : Impl()
{
    m_image = image;
    
    if (image) {
        m_cols = image->_width;
        m_rows = image->_height;
        m_depth = image->_pixel_depth;
    }
    
    m_serialNum = det.GetSerialNum();
}

XFile::Impl::~Impl() {
}

bool XFile::Impl::write(const std::string& file) {
    if (!m_image || !m_image->_data_) {
        std::cerr << "[XFile] No image data to write" << std::endl;
        return false;
    }
    
    std::ofstream outFile(file, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[XFile] Failed to open file: " << file << std::endl;
        return false;
    }
    
    // Simple TIFF header (basic implementation)
    // For production, use libtiff library
    
    // Write custom metadata header
    outFile << "FXIMAGE_TIFF" << std::endl;
    outFile << "Width=" << m_cols << std::endl;
    outFile << "Height=" << m_rows << std::endl;
    outFile << "Depth=" << m_depth << std::endl;
    outFile << "DMNum=" << m_dmNum << std::endl;
    outFile << "DMType=" << m_dmType << std::endl;
    outFile << "OpMode=" << m_opMode << std::endl;
    outFile << "IntTime=" << m_intTime << std::endl;
    outFile << "SerialNum=" << m_serialNum << std::endl;
    outFile << "DateTime=" << m_dateTime << std::endl;
    outFile << "Temperature=" << m_temp << std::endl;
    outFile << "Humidity=" << m_humidity << std::endl;
    outFile << "DATA_START" << std::endl;
    
    // Write image data
    outFile.write(reinterpret_cast<const char*>(m_image->_data_), m_image->_size);
    
    outFile.close();
    
    std::cout << "[XFile] Saved to " << file << std::endl;
    
    return true;
}

bool XFile::Impl::read(const std::string& file) {
    std::ifstream inFile(file, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "[XFile] Failed to open file: " << file << std::endl;
        return false;
    }
    
    // Read metadata
    std::string line;
    while (std::getline(inFile, line)) {
        if (line == "DATA_START") {
            break;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "Width") m_cols = std::stoul(value);
            else if (key == "Height") m_rows = std::stoul(value);
            else if (key == "Depth") m_depth = std::stoul(value);
            else if (key == "DMNum") m_dmNum = std::stoul(value);
            else if (key == "SerialNum") m_serialNum = value;
            else if (key == "DateTime") m_dateTime = value;
        }
    }
    
    // Allocate image buffer
    if (!m_image) {
        m_image = new XImage(m_cols, m_rows, static_cast<uint8_t>(m_depth));
    }
    
    // Read image data
    if (m_image && m_image->_data_) {
        inFile.read(reinterpret_cast<char*>(m_image->_data_), m_image->_size);
    }
    
    inFile.close();
    
    std::cout << "[XFile] Loaded from " << file << std::endl;
    
    return true;
}

bool XFile::Impl::get(XFCode code, uint32_t& data) {
    switch (code) {
        case XF_COLS: data = m_cols; return true;
        case XF_ROWS: data = m_rows; return true;
        case XF_DEPTH: data = m_depth; return true;
        case XF_DM_NUM: data = m_dmNum; return true;
        case XF_DM_TYPE: data = m_dmType; return true;
        case XF_DM_PIX: data = m_dmPix; return true;
        case XF_OP_MODE: data = m_opMode; return true;
        case XF_INT_TIME: data = m_intTime; return true;
        case XF_ENERGY: data = m_energy; return true;
        case XF_BIN: data = m_bin; return true;
        default: return false;
    }
}

bool XFile::Impl::get(XFCode code, float& data) {
    switch (code) {
        case XF_TEMP: data = m_temp; return true;
        case XF_HUM: data = m_humidity; return true;
        default: return false;
    }
}

bool XFile::Impl::get(XFCode code, uint8_t** data_) {
    if (!data_) return false;
    
    switch (code) {
        case XF_DATA:
            *data_ = m_image ? m_image->_data_ : nullptr;
            return true;
        case XF_SN:
            *data_ = reinterpret_cast<uint8_t*>(const_cast<char*>(m_serialNum.c_str()));
            return true;
        case XF_DATE:
            *data_ = reinterpret_cast<uint8_t*>(const_cast<char*>(m_dateTime.c_str()));
            return true;
        default:
            return false;
    }
}

bool XFile::Impl::set(XFCode code, uint32_t data) {
    switch (code) {
        case XF_COLS: m_cols = data; return true;
        case XF_ROWS: m_rows = data; return true;
        case XF_DEPTH: m_depth = data; return true;
        case XF_DM_NUM: m_dmNum = data; return true;
        case XF_DM_TYPE: m_dmType = data; return true;
        case XF_DM_PIX: m_dmPix = data; return true;
        case XF_OP_MODE: m_opMode = data; return true;
        case XF_INT_TIME: m_intTime = data; return true;
        case XF_ENERGY: m_energy = data; return true;
        case XF_BIN: m_bin = data; return true;
        default: return false;
    }
}

bool XFile::Impl::set(XFCode code, float data) {
    switch (code) {
        case XF_TEMP: m_temp = data; return true;
        case XF_HUM: m_humidity = data; return true;
        default: return false;
    }
}

bool XFile::Impl::set(XFCode code, uint8_t* data_) {
    if (!data_) return false;
    
    switch (code) {
        case XF_DATA:
            if (m_image) {
                m_image->_data_ = data_;
            }
            return true;
        case XF_SN:
            m_serialNum = reinterpret_cast<char*>(data_);
            return true;
        case XF_DATE:
            m_dateTime = reinterpret_cast<char*>(data_);
            return true;
        default:
            return false;
    }
}

// XFile public interface
XFile::XFile()
    : m_impl(new Impl())
{
}

XFile::XFile(XImage* image_, XDetector& det)
    : m_impl(new Impl(image_, det))
{
}

XFile::~XFile() {
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

bool XFile::Read(const std::string& file) {
    if (!m_impl) {
        return false;
    }
    return m_impl->read(file);
}

bool XFile::Write(const std::string& file) {
    if (!m_impl) {
        return false;
    }
    return m_impl->write(file);
}

bool XFile::Get(XFCode code, uint32_t& data) {
    if (!m_impl) {
        return false;
    }
    return m_impl->get(code, data);
}

bool XFile::Get(XFCode code, float& data) {
    if (!m_impl) {
        return false;
    }
    return m_impl->get(code, data);
}

bool XFile::Get(XFCode code, uint8_t** data_) {
    if (!m_impl) {
        return false;
    }
    return m_impl->get(code, data_);
}

bool XFile::Set(XFCode code, uint32_t data) {
    if (!m_impl) {
        return false;
    }
    return m_impl->set(code, data);
}

bool XFile::Set(XFCode code, float data) {
    if (!m_impl) {
        return false;
    }
    return m_impl->set(code, data);
}

bool XFile::Set(XFCode code, uint8_t* data_) {
    if (!m_impl) {
        return false;
    }
    return m_impl->set(code, data_);
}

} // namespace HX