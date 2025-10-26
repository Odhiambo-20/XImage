// ============================================================================
// XImage.cpp
// ============================================================================

/**
 * @file XImage.cpp
 * @brief XImage implementation - Image data container
 * @version 2.1.0
 */

#include "XImage.h"
#include <cstring>
#include <fstream>
#include <iostream>

namespace HX {

XImage::XImage()
    : _data_(nullptr)
    , _data_offset(0)
    , _height(0)
    , _width(0)
    , _pixel_depth(16)
    , _size(0)
    , m_ownsData(false)
{
}

XImage::XImage(uint32_t width, uint32_t height, uint8_t pixelDepth)
    : _data_(nullptr)
    , _data_offset(0)
    , _height(height)
    , _width(width)
    , _pixel_depth(pixelDepth)
    , _size(0)
    , m_ownsData(true)
{
    allocateMemory();
}

XImage::~XImage() {
    freeMemory();
}

void XImage::allocateMemory() {
    freeMemory();
    
    uint32_t bytesPerPixel = (_pixel_depth + 7) / 8;
    _size = _width * _height * bytesPerPixel;
    
    if (_size > 0) {
        _data_ = new uint8_t[_size];
        memset(_data_, 0, _size);
        m_ownsData = true;
    }
}

void XImage::freeMemory() {
    if (m_ownsData && _data_) {
        delete[] _data_;
        _data_ = nullptr;
    }
    m_ownsData = false;
    _size = 0;
}

void XImage::SetData(uint8_t* data, uint32_t width, uint32_t height, 
                     uint8_t pixelDepth, bool takeOwnership) {
    freeMemory();
    
    _data_ = data;
    _width = width;
    _height = height;
    _pixel_depth = pixelDepth;
    
    uint32_t bytesPerPixel = (_pixel_depth + 7) / 8;
    _size = _width * _height * bytesPerPixel;
    
    m_ownsData = takeOwnership;
}

uint32_t XImage::GetPixelVal(uint32_t row, uint32_t col) const {
    if (!_data_ || row >= _height || col >= _width) {
        return 0;
    }
    
    uint32_t bytesPerPixel = (_pixel_depth + 7) / 8;
    uint32_t offset = _data_offset + (row * _width + col) * bytesPerPixel;
    
    uint32_t value = 0;
    for (uint32_t i = 0; i < bytesPerPixel && i < 4; ++i) {
        value |= (uint32_t(_data_[offset + i]) << (i * 8));
    }
    
    return value;
}

void XImage::SetPixelVal(uint32_t row, uint32_t col, uint32_t pixel_value) {
    if (!_data_ || row >= _height || col >= _width) {
        return;
    }
    
    uint32_t bytesPerPixel = (_pixel_depth + 7) / 8;
    uint32_t offset = _data_offset + (row * _width + col) * bytesPerPixel;
    
    for (uint32_t i = 0; i < bytesPerPixel && i < 4; ++i) {
        _data_[offset + i] = (pixel_value >> (i * 8)) & 0xFF;
    }
}

bool XImage::Save(const char* file_name_) const {
    if (!_data_ || !file_name_) {
        return false;
    }
    
    std::ofstream file(file_name_, std::ios::out);
    if (!file.is_open()) {
        std::cerr << "[XImage] Failed to open file: " << file_name_ << std::endl;
        return false;
    }
    
    // Write header
    file << "Width: " << _width << std::endl;
    file << "Height: " << _height << std::endl;
    file << "PixelDepth: " << static_cast<int>(_pixel_depth) << std::endl;
    file << "Data:" << std::endl;
    
    // Write pixel data
    for (uint32_t row = 0; row < _height; ++row) {
        for (uint32_t col = 0; col < _width; ++col) {
            uint32_t value = GetPixelVal(row, col);
            file << value;
            if (col < _width - 1) {
                file << " ";
            }
        }
        file << std::endl;
    }
    
    file.close();
    std::cout << "[XImage] Saved to " << file_name_ << std::endl;
    
    return true;
}

void XImage::Clear() {
    if (_data_ && _size > 0) {
        memset(_data_, 0, _size);
    }
}

XImage* XImage::Clone() const {
    if (!_data_) {
        return nullptr;
    }
    
    XImage* clone = new XImage(_width, _height, _pixel_depth);
    if (clone->_data_) {
        memcpy(clone->_data_, _data_, _size);
        clone->_data_offset = _data_offset;
    }
    
    return clone;
}

} // namespace HX