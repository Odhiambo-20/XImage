

// ============================================================================

/**
 * @file XGrabber.h
 * @brief XGrabber class - Image data acquisition
 * @version 2.1.0
 */

#ifndef XGRABBER_H
#define XGRABBER_H

#include <cstdint>

namespace HX {

class XDetector;
class XControl;
class XFrame;
class XFactory;
class IXImgSink;

/**
 * @class XGrabber
 * @brief Acquires image data from detector
 */
class XGrabber {
public:
    XGrabber();
    ~XGrabber();
    
    /**
     * @brief Open connection to detector
     * @param dec Detector configuration
     * @param control Control interface
     * @return true on success
     */
    bool Open(XDetector& dec, XControl& control);
    
    /**
     * @brief Close connection
     */
    void Close();
    
    /**
     * @brief Check if connection is open
     * @return true if open
     */
    bool IsOpen();
    
    /**
     * @brief Start image acquisition
     * @param frames Number of frames to grab (0 = continuous)
     * @return true on success
     */
    bool Grab(uint32_t frames);
    
    /**
     * @brief Capture single frame
     * @return true on success
     */
    bool Snap();
    
    /**
     * @brief Stop image acquisition
     * @return true on success
     */
    bool Stop();
    
    /**
     * @brief Check if currently grabbing
     * @return true if grabbing
     */
    bool IsGrabbing();
    
    /**
     * @brief Enable/disable line header mode
     * @param enable true to enable header mode
     */
    void SetHeader(bool enable);
    
    /**
     * @brief Set event callback sink
     * @param sink_ Callback handler
     */
    void SetSink(IXImgSink* sink_);
    
    /**
     * @brief Set frame assembly object
     * @param frame XFrame instance
     */
    void SetFrame(XFrame& frame);
    
    /**
     * @brief Set factory for resource management
     * @param factory XFactory instance
     */
    void SetFactory(XFactory& factory);
    
    /**
     * @brief Set receive timeout
     * @param time Timeout in milliseconds
     */
    void SetTimeout(uint32_t time);
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XGrabber(const XGrabber&) = delete;
    XGrabber& operator=(const XGrabber&) = delete;
};

} // namespace HX

#endif // XGRABBER_H