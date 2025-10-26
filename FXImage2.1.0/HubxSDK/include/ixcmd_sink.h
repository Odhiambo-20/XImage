/**
 * @file ixcmd_sink.h
 * @brief Abstract interface for command event callbacks
 */

#ifndef IXCMD_SINK_H
#define IXCMD_SINK_H

#include <cstdint>

namespace HX {

/**
 * @class IXCmdSink
 * @brief Abstract callback interface for command events
 * 
 * Users must derive from this class to handle error and event callbacks
 * from XControl and XAdaptor objects.
 */
class IXCmdSink {
public:
    virtual ~IXCmdSink() {}
    
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
    virtual void OnXEvent(uint32_t event_id, float data) = 0;
};

} // namespace HX

#endif // IXCMD_SINK_H
