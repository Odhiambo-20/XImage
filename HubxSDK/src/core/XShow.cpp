// ============================================================================
// XShow.cpp - Display functionality (Windows only)
// ============================================================================

/**
 * @file XShow.cpp
 * @brief XShow implementation - Image display (Windows only)
 * @version 2.1.0
 */

#include "XShow.h"
#include "XImage.h"
#include "XDetector.h"
#include <iostream>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

namespace HX {

class XShow::Impl {
public:
    Impl();
    ~Impl();
    
    bool open(uint32_t cols, uint32_t rows, uint32_t pixel_depth, 
              void* hwnd, XColor color);
    bool open(XDetector& det, uint32_t rows, void* hwnd, XColor color);
    void close();
    bool isOpen() const { return m_opened; }
    
    void show(XImage* img_);
    
    void setGama(float gama);
    float getGama() const { return m_gamma; }
    
private:
    void applyColorMap(uint8_t* displayBuffer, const uint8_t* imageData, 
                       uint32_t pixelCount);
    uint8_t applyGamma(uint8_t value);
    
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_pixelDepth;
    void* m_windowHandle;
    XColor m_colorMode;
    float m_gamma;
    bool m_opened;
    
#ifdef _WIN32
    BITMAPINFO m_bitmapInfo;
    uint8_t* m_displayBuffer;
#endif
};

XShow::Impl::Impl()
    : m_width(0)
    , m_height(0)
    , m_pixelDepth(16)
    , m_windowHandle(nullptr)
    , m_colorMode(XCOLOR_GRAY)
    , m_gamma(1.0f)
    , m_opened(false)
#ifdef _WIN32
    , m_displayBuffer(nullptr)
#endif
{
}

XShow::Impl::~Impl() {
    close();
}

bool XShow::Impl::open(uint32_t cols, uint32_t rows, uint32_t pixel_depth,
                       void* hwnd, XColor color) {
    if (m_opened) {
        return true;
    }
    
#ifndef _WIN32
    std::cerr << "[XShow] Only available on Windows" << std::endl;
    return false;
#else
    
    m_width = cols;
    m_height = rows;
    m_pixelDepth = pixel_depth;
    m_windowHandle = hwnd;
    m_colorMode = color;
    
    // Allocate display buffer (24-bit RGB)
    uint32_t bufferSize = m_width * m_height * 3;
    m_displayBuffer = new uint8_t[bufferSize];
    memset(m_displayBuffer, 0, bufferSize);
    
    // Setup bitmap info
    memset(&m_bitmapInfo, 0, sizeof(BITMAPINFO));
    m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bitmapInfo.bmiHeader.biWidth = m_width;
    m_bitmapInfo.bmiHeader.biHeight = -static_cast<int32_t>(m_height); // Top-down
    m_bitmapInfo.bmiHeader.biPlanes = 1;
    m_bitmapInfo.bmiHeader.biBitCount = 24;
    m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    m_opened = true;
    
    std::cout << "[XShow] Opened: " << m_width << "x" << m_height << std::endl;
    
    return true;
#endif
}

bool XShow::Impl::open(XDetector& det, uint32_t rows, void* hwnd, XColor color) {
    return open(det.GetPixelCount(), rows, det.GetPixelDepth(), hwnd, color);
}

void XShow::Impl::close() {
    if (!m_opened) {
        return;
    }
    
#ifdef _WIN32
    if (m_displayBuffer) {
        delete[] m_displayBuffer;
        m_displayBuffer = nullptr;
    }
#endif
    
    m_opened = false;
    
    std::cout << "[XShow] Closed" << std::endl;
}

void XShow::Impl::show(XImage* img_) {
    if (!m_opened || !img_ || !img_->_data_) {
        return;
    }
    
#ifdef _WIN32
    if (!m_displayBuffer || !m_windowHandle) {
        return;
    }
    
    // Convert image data to display buffer
    applyColorMap(m_displayBuffer, img_->_data_, m_width * m_height);
    
    // Display using Windows GDI
    HDC hdc = GetDC(static_cast<HWND>(m_windowHandle));
    if (hdc) {
        SetDIBitsToDevice(
            hdc,
            0, 0, m_width, m_height,
            0, 0, 0, m_height,
            m_displayBuffer,
            &m_bitmapInfo,
            DIB_RGB_COLORS
        );
        ReleaseDC(static_cast<HWND>(m_windowHandle), hdc);
    }
#endif
}

void XShow::Impl::applyColorMap(uint8_t* displayBuffer, const uint8_t* imageData,
                                uint32_t pixelCount) {
    uint32_t bytesPerPixel = (m_pixelDepth + 7) / 8;
    
    for (uint32_t i = 0; i < pixelCount; ++i) {
        // Extract pixel value
        uint32_t pixelValue = 0;
        for (uint32_t j = 0; j < bytesPerPixel && j < 4; ++j) {
            pixelValue |= (imageData[i * bytesPerPixel + j] << (j * 8));
        }
        
        // Normalize to 8-bit
        uint8_t normalized = static_cast<uint8_t>(
            (pixelValue * 255) / ((1 << m_pixelDepth) - 1)
        );
        
        // Apply gamma correction
        normalized = applyGamma(normalized);
        
        // Apply color map
        uint8_t r, g, b;
        
        switch (m_colorMode) {
            case XCOLOR_GRAY:
                r = g = b = normalized;
                break;
                
            case XCOLOR_HOT:
                // Hot color map (black -> red -> yellow -> white)
                if (normalized < 85) {
                    r = normalized * 3;
                    g = 0;
                    b = 0;
                } else if (normalized < 170) {
                    r = 255;
                    g = (normalized - 85) * 3;
                    b = 0;
                } else {
                    r = 255;
                    g = 255;
                    b = (normalized - 170) * 3;
                }
                break;
                
            case XCOLOR_JET:
                // Jet color map (blue -> cyan -> yellow -> red)
                if (normalized < 64) {
                    r = 0;
                    g = 0;
                    b = 128 + normalized * 2;
                } else if (normalized < 128) {
                    r = 0;
                    g = (normalized - 64) * 4;
                    b = 255;
                } else if (normalized < 192) {
                    r = (normalized - 128) * 4;
                    g = 255;
                    b = 255 - (normalized - 128) * 4;
                } else {
                    r = 255;
                    g = 255 - (normalized - 192) * 4;
                    b = 0;
                }
                break;
                
            default:
                r = g = b = normalized;
                break;
        }
        
        // Store RGB values (BGR format for Windows)
        displayBuffer[i * 3 + 0] = b;
        displayBuffer[i * 3 + 1] = g;
        displayBuffer[i * 3 + 2] = r;
    }
}

uint8_t XShow::Impl::applyGamma(uint8_t value) {
    if (m_gamma == 1.0f) {
        return value;
    }
    
    float normalized = value / 255.0f;
    float corrected = std::pow(normalized, m_gamma);
    return static_cast<uint8_t>(corrected * 255.0f);
}

void XShow::Impl::setGama(float gama) {
    if (gama >= 1.0f && gama <= 4.0f) {
        m_gamma = gama;
    }
}

float XShow::Impl::getGama() const {
    return m_gamma;
}

// XShow public interface
XShow::XShow()
    : m_impl(new Impl())
{
}

XShow::~XShow() {
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

bool XShow::Open(uint32_t cols, uint32_t rows, uint32_t pixel_depth,
                 void* hwnd, XColor color) {
    if (!m_impl) {
        return false;
    }
    return m_impl->open(cols, rows, pixel_depth, hwnd, color);
}

bool XShow::Open(XDetector& det, uint32_t rows, void* hwnd, XColor color) {
    if (!m_impl) {
        return false;
    }
    return m_impl->open(det, rows, hwnd, color);
}

void XShow::Close() {
    if (m_impl) {
        m_impl->close();
    }
}

bool XShow::IsOpen() {
    if (!m_impl) {
        return false;
    }
    return m_impl->isOpen();
}

void XShow::Show(XImage* img_) {
    if (m_impl) {
        m_impl->show(img_);
    }
}

void XShow::SetGama(float gama) {
    if (m_impl) {
        m_impl->setGama(gama);
    }
}

float XShow::GetGama() {
    if (!m_impl) {
        return 1.0f;
    }
    return m_impl->getGama();
}

} // namespace HX