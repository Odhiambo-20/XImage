/**
 * @file iximg_sink.h
 * @brief Abstract interface for image event callbacks
 */

#ifndef IXIMG_SINK_H
#define IXIMG_SINK_H

#include <cstdint>

namespace HX {

class XImage;

/**
 * @class IXImgSink
 * @brief Abstract callback interface for image events
 * 
 * Users must derive from this class to handle error and frame-ready
 * callbacks from XGrabber and XFrame objects.
 */
class IXImgSink {
public:
    virtual ~IXImgSink() {}
    
    /**
     * @brief Error callback
     * @param err_id Error code
     * @param err_msg Error message
     */
    virtual void OnXError(uint32_t err_id, const char* err_msg) = 0;
    
    /**
     * @brief Event callback
     * @param event_id Event code
     * @param data Event data
     */
    virtual void OnXEvent(uint32_t event_id, uint32_t data) = 0;
    
    /**
     * @brief Frame ready callback
     * @param image Pointer to frame image data
     */
    virtual void OnFrameReady(XImage* image) = 0;
};

} // namespace HX

#endif // IXIMG_SINK_H
