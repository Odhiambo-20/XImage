// ============================================================================
// XGrabber.cpp
// ============================================================================

/**
 * @file XGrabber.cpp
 * @brief XGrabber implementation - Image data acquisition
 * @version 2.1.0
 */

#include "XGrabber.h"
#include "XDetector.h"
#include "XControl.h"
#include "XFrame.h"
#include "xfactory.h"
#include "iximg_sink.h"
#include "xlibdll_wrapper/xlibdll_interface.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace HX {

class XGrabber::Impl {
public:
    Impl();
    ~Impl();
    
    bool open(XDetector& det, XControl& control);
    void close();
    bool isOpen() const { return m_opened; }
    
    bool grab(uint32_t frames);
    bool snap();
    bool stop();
    bool isGrabbing() const { return m_grabbing; }
    
    void setHeader(bool enable) { m_headerMode = enable; }
    void setSink(IXImgSink* sink) { m_sink = sink; }
    void setFrame(XFrame& frame);
    void setFactory(XFactory& factory) { m_factory = &factory; }
    void setTimeout(uint32_t time) { m_timeout = time; }
    
private:
    void grabThread();
    void processPacket(const uint8_t* packetData, uint32_t packetLen);
    void reportError(uint32_t errorId, const char* message);
    void reportEvent(uint32_t eventId, uint32_t data);
    
    XDetector m_detector;
    XControl* m_control;
    XFrame* m_frame;
    XFactory* m_factory;
    IXImgSink* m_sink;
    
    bool m_opened;
    std::atomic<bool> m_grabbing;
    std::atomic<bool> m_stopRequested;
    uint32_t m_framesToGrab;
    uint32_t m_framesGrabbed;
    
    bool m_headerMode;
    uint32_t m_timeout;
    
    std::thread m_grabThread;
    mutable std::mutex m_mutex;
    
    // Statistics
    uint32_t m_packetsReceived;
    uint32_t m_packetsLost;
    uint32_t m_linesReceived;
};

XGrabber::Impl::Impl()
    : m_control(nullptr)
    , m_frame(nullptr)
    , m_factory(nullptr)
    , m_sink(nullptr)
    , m_opened(false)
    , m_grabbing(false)
    , m_stopRequested(false)
    , m_framesToGrab(0)
    , m_framesGrabbed(0)
    , m_headerMode(false)
    , m_timeout(20000)
    , m_packetsReceived(0)
    , m_packetsLost(0)
    , m_linesReceived(0)
{
}

XGrabber::Impl::~Impl() {
    close();
}

bool XGrabber::Impl::open(XDetector& det, XControl& control) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_opened) {
        std::cout << "[XGrabber] Already opened" << std::endl;
        return true;
    }
    
    std::cout << "[XGrabber] Opening..." << std::endl;
    
    // Validate parameters
    if (!m_frame) {
        reportError(25, "XFrame not set");
        return false;
    }
    
    // Check if xlibdll proxy is initialized
    if (!Internal::XLibProxy_IsLoaded()) {
        reportError(25, "xlibdll proxy not initialized");
        return false;
    }
    
    m_detector = det;
    m_control = &control;
    
    // Initialize network for image reception
    int32_t result = Internal::XLibProxy_InitNetwork(
        m_detector.GetIP().c_str(),
        m_detector.GetImgPort()
    );
    
    if (result < 0) {
        const char* errorMsg = Internal::XLibProxy_GetErrorMessage(result);
        reportError(21, errorMsg);
        return false;
    }
    
    m_opened = true;
    m_packetsReceived = 0;
    m_packetsLost = 0;
    m_linesReceived = 0;
    
    std::cout << "[XGrabber] Opened successfully" << std::endl;
    
    return true;
}

void XGrabber::Impl::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        return;
    }
    
    std::cout << "[XGrabber] Closing..." << std::endl;
    
    // Stop grabbing if running
    if (m_grabbing) {
        m_stopRequested = true;
        if (m_grabThread.joinable()) {
            m_grabThread.join();
        }
    }
    
    m_opened = false;
    m_control = nullptr;
    
    std::cout << "[XGrabber] Closed" << std::endl;
    std::cout << "[XGrabber] Statistics:" << std::endl;
    std::cout << "  Packets received: " << m_packetsReceived << std::endl;
    std::cout << "  Packets lost: " << m_packetsLost << std::endl;
    std::cout << "  Lines received: " << m_linesReceived << std::endl;
}

bool XGrabber::Impl::grab(uint32_t frames) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_opened) {
        reportError(25, "XGrabber not opened");
        return false;
    }
    
    if (m_grabbing) {
        reportError(26, "Already grabbing");
        return false;
    }
    
    std::cout << "[XGrabber] Starting acquisition..." << std::endl;
    
    m_framesToGrab = frames;
    m_framesGrabbed = 0;
    m_grabbing = true;
    m_stopRequested = false;
    
    // Start frame assembly
    uint32_t pixelCount = m_detector.GetPixelCount();
    uint8_t pixelDepth = m_detector.GetPixelDepth();
    
    if (!m_frame->Start(pixelCount, pixelDepth)) {
        reportError(26, "Failed to start frame assembly");
        m_grabbing = false;
        return false;
    }
    
    // Start grab thread
    m_grabThread = std::thread(&Impl::grabThread, this);
    
    std::cout << "[XGrabber] Acquisition started" << std::endl;
    
    return true;
}

void XGrabber::Impl::grabThread() {
    std::cout << "[XGrabber] Grab thread started" << std::endl;
    
    const uint32_t BUFFER_SIZE = 65536; // 64KB buffer
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    
    while (m_grabbing && !m_stopRequested) {
        // Receive image data packet
        int32_t bytesReceived = Internal::XLibProxy_ReceiveImageData(
            buffer.data(),
            BUFFER_SIZE,
            m_timeout
        );
        
        if (bytesReceived < 0) {
            if (bytesReceived == Internal::XLIB_ERROR_TIMEOUT) {
                // Timeout is normal, continue
                continue;
            } else {
                const char* errorMsg = Internal::XLibProxy_GetErrorMessage(bytesReceived);
                reportError(23, errorMsg);
                break;
            }
        }
        
        if (bytesReceived > 0) {
            m_packetsReceived++;
            processPacket(buffer.data(), static_cast<uint32_t>(bytesReceived));
        }
        
        // Check if we've grabbed enough frames
        if (m_framesToGrab > 0 && m_framesGrabbed >= m_framesToGrab) {
            break;
        }
    }
    
    // Stop frame assembly
    m_frame->Stop();
    
    m_grabbing = false;
    
    std::cout << "[XGrabber] Grab thread stopped" << std::endl;
}

void XGrabber::Impl::processPacket(const uint8_t* packetData, uint32_t packetLen) {
    // Extract packet header if in header mode
    if (m_headerMode && packetLen >= 8) {
        Internal::XLibPacketHeader header;
        if (Internal::XLibProxy_ExtractPacketHeader(packetData, &header) == 0) {
            // Process line data with header info
            const uint8_t* lineData = packetData + 8; // Skip header
            uint32_t lineLen = packetLen - 8;
            
            m_frame->AddLine(lineData, lineLen, header.lineId);
            m_linesReceived++;
        }
    } else {
        // Process raw line data without header
        m_frame->AddLine(packetData, packetLen, m_linesReceived);
        m_linesReceived++;
    }
}

bool XGrabber::Impl::snap() {
    if (!grab(1)) {
        return false;
    }
    
    // Wait for frame to complete
    while (m_grabbing && m_framesGrabbed < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    stop();
    
    return true;
}

bool XGrabber::Impl::stop() {
    if (!m_grabbing) {
        return true;
    }
    
    std::cout << "[XGrabber] Stopping acquisition..." << std::endl;
    
    m_stopRequested = true;
    
    if (m_grabThread.joinable()) {
        m_grabThread.join();
    }
    
    m_grabbing = false;
    
    std::cout << "[XGrabber] Acquisition stopped" << std::endl;
    
    return true;
}

void XGrabber::Impl::setFrame(XFrame& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_grabbing) {
        reportError(25, "Cannot set frame while grabbing");
        return;
    }
    
    m_frame = &frame;
}

void XGrabber::Impl::reportError(uint32_t errorId, const char* message) {
    std::cerr << "[XGrabber] ERROR " << errorId << ": " << message << std::endl;
    
    if (m_sink) {
        m_sink->OnXError(errorId, message);
    }
}

void XGrabber::Impl::reportEvent(uint32_t eventId, uint32_t data) {
    if (m_sink) {
        m_sink->OnXEvent(eventId, data);
    }
}

// XGrabber public interface
XGrabber::XGrabber()
    : m_impl(new Impl())
{
}

XGrabber::~XGrabber() {
    if (m_impl) {
        delete m_impl;
        m_impl = nullptr;
    }
}

bool XGrabber::Open(XDetector& dec, XControl& control) {
    if (!m_impl) {
        return false;
    }
    return m_impl->open(dec, control);
}

void XGrabber::Close() {
    if (m_impl) {
        m_impl->close();
    }
}

bool XGrabber::IsOpen() {
    if (!m_impl) {
        return false;
    }
    return m_impl->isOpen();
}

bool XGrabber::Grab(uint32_t frames) {
    if (!m_impl) {
        return false;
    }
    return m_impl->grab(frames);
}

bool XGrabber::Snap() {
    if (!m_impl) {
        return false;
    }
    return m_impl->snap();
}

bool XGrabber::Stop() {
    if (!m_impl) {
        return false;
    }
    return m_impl->stop();
}

bool XGrabber::IsGrabbing() {
    if (!m_impl) {
        return false;
    }
    return m_impl->isGrabbing();
}

void XGrabber::SetHeader(bool enable) {
    if (m_impl) {
        m_impl->setHeader(enable);
    }
}

void XGrabber::SetSink(IXImgSink* sink_) {
    if (m_impl) {
        m_impl->setSink(sink_);
    }
}

void XGrabber::SetFrame(XFrame& frame) {
    if (m_impl) {
        m_impl->setFrame(frame);
    }
}

void XGrabber::SetFactory(XFactory& factory) {
    if (m_impl) {
        m_impl->setFactory(factory);
    }
}

void XGrabber::SetTimeout(uint32_t time) {
    if (m_impl) {
        m_impl->setTimeout(time);
    }
}

} // namespace HX