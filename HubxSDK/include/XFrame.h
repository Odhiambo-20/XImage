/**
 * @file XFrame.h
 * @brief XFrame class - Frame assembly from line data
 * @version 2.1.0
 */

#ifndef XFRAME_H
#define XFRAME_H

#include <cstdint>

namespace HX {

class IXImgSink;

/**
 * @class XFrame
 * @brief Assembles line data into complete frames
 */
class XFrame {
public:
    XFrame();
    explicit XFrame(uint32_t lines);
    ~XFrame();
    
    /**
     * @brief Set number of lines per frame
     * @param lines Lines per frame
     */
    void SetLines(uint32_t lines);
    
    /**
     * @brief Get number of lines per frame
     * @return Lines per frame
     */
    uint32_t GetLines() const;
    
    /**
     * @brief Set event callback sink
     * @param sink_ Callback handler
     */
    void SetSink(IXImgSink* sink_);
    
    /**
     * @brief Start frame assembly
     * @param width Image width (pixels)
     * @param pixelDepth Bits per pixel
     * @return true on success
     */
    bool Start(uint32_t width, uint8_t pixelDepth);
    
    /**
     * @brief Stop frame assembly
     */
    void Stop();
    
    /**
     * @brief Check if frame assembly is running
     * @return true if running
     */
    bool IsRunning() const;
    
    /**
     * @brief Add line data to frame
     * @param lineData Line data pointer
     * @param lineLen Line data length
     * @param lineId Line identifier
     */
    void AddLine(const uint8_t* lineData, uint32_t lineLen, uint32_t lineId);
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XFrame(const XFrame&) = delete;
    XFrame& operator=(const XFrame&) = delete;
};

} // namespace HX

#endif // XFRAME_H