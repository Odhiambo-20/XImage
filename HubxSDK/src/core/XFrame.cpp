// ============================================================================
// XFrame.cpp
// ============================================================================

/**
 * @file XFrame.cpp
 * @brief XFrame implementation - Frame assembly from line data
 * @version 2.1.0
 */

#include "XFrame.h"
#include "XImage.h"
#include "iximg_sink.h"
#include <iostream>
#include <cstring>
#include <mutex>

namespace HX {

class XFrame::Impl {
public:
    Impl(uint32_t lines);
    ~Impl();
    
    void setLines(uint32_t lines);
    uint32_t getLines() const { return m_linesPerFrame; }
    
    void setSink(IXImgSink* sink) { m_sink = sink; }
    
    bool start(uint32_t width, uint8_t pixelDepth);
    void stop();
    bool isRunning() const { return m_running; }
    
    void addLine(const uint8_t* lineData, uint32_t lineLen, uint32_t lineId);
    
private:
    void assembleFrame();
    void reportError(uint32_t errorId, const char* message);
    void reportEvent(uint32_t eventId, uint32_t data);
    
    uint32_t m_linesPerFrame;
    uint32_t m_imageWidth;
    uint8_t m_pixelDepth;
    
    XImage* m_currentFrame;
    uint32_t m_currentLine;
    bool m_running;
    
    IXImgSink* m_sink;
    mutable std::mutex m_mutex;
};

XFrame::Impl::Impl(uint32_t lines)
    : m_linesPerFrame(lines)
    , m_imageWidth(0)
    , m_pixelDepth(16)
    , m_currentFrame(nullptr)
    , m_currentLine(0)
    , m_running(false)
    , m_sink(nullptr)
{
}

XFrame::Impl::~Impl() {
    stop();
}

void XFrame::Impl::setLines(uint32_t lines) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_running) {
        reportError(32, "Cannot change lines while running");
        return;
    }
    
    m_linesPerFrame = lines;
}

bool XFrame::Impl::start(uint32_t width, uint8_t pixelDepth) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_running) {
        return true;
    }
    
    m_imageWidth = width;
    m_pixelDepth = pixelDepth;
    
    // Allocate frame buffer
    m_currentFrame = new XImage(width, m_linesPerFrame, pixelDepth);
    if (!m_currentFrame || !m_currentFrame->_data_) {
        reportError(33, "Failed to allocate frame buffer");
        delete m_currentFrame;
        m_currentFrame = nullptr;
        return false;
    }
    
    m_currentLine = 0;
    m_running = true;
    
    std::cout << "[XFrame] Started: " << width << "x" << m_linesPerFrame 
              << " @ " << static_cast<int>(pixelDepth) << " bits" << std::endl;
    
    return true;
}

void XFrame::Impl::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_running) {
        return;
    }
    
    if (m_currentFrame) {
        delete m_currentFrame;
        m_currentFrame = nullptr;
    }
    
    m_running = false;
    m_currentLine = 0;
    
    std::cout << "[XFrame] Stopped" << std::endl;
}

void XFrame::Impl::addLine(const uint8_t* lineData, uint32_t lineLen, uint32_t lineId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_running || !m_currentFrame) {
        return;
    }
    
    // Calculate expected line length
    uint32_t bytesPerPixel = (m_pixelDepth + 7) / 8;
    uint32_t expectedLen = m_imageWidth * bytesPerPixel;
    
    if (lineLen != expectedLen) {
        reportError(101, "Line length mismatch");
        return;
    }
    
    // Copy line data to frame buffer
    uint32_t lineOffset = m_currentLine * m_imageWidth * bytesPerPixel;
    if (lineOffset + lineLen <= m_currentFrame->_size) {
        memcpy(m_currentFrame->_data_ + lineOffset, lineData, lineLen);
    }
    
    m_currentLine++;
    
    // Check if frame is complete
    if (m_currentLine >= m_linesPerFrame) {
        assembleFrame();
    }
}

void XFrame::Impl::assembleFrame() {
    if (!m_sink || !m_currentFrame) {
        return;
    }
    
    // Notify frame ready
    m_sink->OnFrameReady(m_currentFrame);
    
    // Reset for next frame
    m_currentLine = 0;
    m_currentFrame->Clear();
}

void XFrame::Impl::reportError(uint32_t errorId, const char* message) {
    std::cerr << "[XFrame] ERROR " << errorId << ": " << message << std::endl;
    
    if (m_sink) {
        m_sink->OnXError(errorId, message);
    }
}

void XFrame::Impl::reportEvent(uint32_t eventId, uint32_t data) {
    if (m_sink) {
        m_sink->OnXEvent(eventId, data);
    }
}

// XFrame public interface
XFrame::XFrame()
    : m_impl(new Impl(1024))
{
}

XFrame::XFrame(uint32_t lines)
    : m_impl(new Impl(lines))
{
}

XFrame::~XFrame() {
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

void XFrame::SetLines(uint32_t lines) {
    if (m_impl) {
        m_impl->setLines(lines);
    }
}

uint32_t XFrame::GetLines() const {
    if (!m_impl) {
        return 0;
    }
    return m_impl->getLines();
}

void XFrame::SetSink(IXImgSink* sink_) {
    if (m_impl) {
        m_impl->setSink(sink_);
    }
}

bool XFrame::Start(uint32_t width, uint8_t pixelDepth) {
    if (!m_impl) {
        return false;
    }
    return m_impl->start(width, pixelDepth);
}

void XFrame::Stop() {
    if (m_impl) {
        m_impl->stop();
    }
}

bool XFrame::IsRunning() const {
    if (!m_impl) {
        return false;
    }
    return m_impl->isRunning();
}

void XFrame::AddLine(const uint8_t* lineData, uint32_t lineLen, uint32_t lineId) {
    if (m_impl) {
        m_impl->addLine(lineData, lineLen, lineId);
    }
}

} // namespace HX