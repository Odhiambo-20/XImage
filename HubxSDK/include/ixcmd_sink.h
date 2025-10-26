
// ============================================================================

/**
 * @file ixcmd_sink.h
 * @brief IXCmdSink interface - Command event callbacks
 * @version 2.1.0
 */

#ifndef IXCMD_SINK_H
#define IXCMD_SINK_H

#include <cstdint>

namespace HX {

/**
 * @class IXCmdSink
 * @brief Abstract interface for command event callbacks
 * 
 * Users must derive from this class to handle:
 * - Error events
 * - Status events
 */
class IXCmdSink {
public:
    virtual ~IXCmdSink() {}
    
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
    virtual void OnXEvent(uint32_t event_id, float data) = 0;
};

} // namespace HX

#endif // IXCMD_SINK_H