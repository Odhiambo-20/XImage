

// ============================================================================

/**
 * @file iximg_sink.h
 * @brief IXImgSink interface - Image event callbacks
 * @version 2.1.0
 */

#ifndef IXIMG_SINK_H
#define IXIMG_SINK_H

#include <cstdint>

namespace HX {

class XImage;

/**
 * @class IXImgSink
 * @brief Abstract interface for image event callbacks
 * 
 * Users must derive from this class to handle:
 * - Error events
 * - Status events
 * - Frame ready events
 */
class IXImgSink {
public:
    virtual ~IXImgSink() {}
    
    /**
     * @brief Error event callback
     * @param err_id Error code
     * @param err_msg_ Error message
     */
    virtual void OnXError(uint32_t err_id, const char* err_msg_) = 0;
    
    /**
     * @brief Event callback
     * @param event_id Event code
     * @param data Event data
     */
    virtual void OnXEvent(uint32_t event_id, uint32_t data) = 0;
    
    /**
     * @brief Frame ready callback
     * @param image_ Pointer to frame image data
     * 
     * @note This function should return quickly to avoid buffer overflow
     */
    virtual void OnFrameReady(XImage* image_) = 0;
};

} // namespace HX

#endif // IXIMG_SINK_H