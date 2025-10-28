

// ============================================================================

/**
 * @file XShow.h
 * @brief XShow class - Image display (Windows only)
 * @version 2.1.0
 */

#ifndef XSHOW_H
#define XSHOW_H

#include <cstdint>

namespace HX {

class XImage;
class XDetector;

/**
 * @class XShow
 * @brief Displays image data (Windows only)
 * 
 * @note This class is only available on Windows systems
 */
class XShow {
public:
    /**
     * @enum XColor
     * @brief Color map modes
     */
    enum XColor {
        XCOLOR_GRAY = 0,    ///< Grayscale
        XCOLOR_SIN,         ///< Sin color map
        XCOLOR_COS,         ///< Cos color map
        XCOLOR_HOT,         ///< Hot color map (black->red->yellow->white)
        XCOLOR_JET          ///< Jet color map (blue->cyan->yellow->red)
    };
    
    XShow();
    ~XShow();
    
    /**
     * @brief Open display window
     * @param cols Image width
     * @param rows Image height
     * @param pixel_depth Bits per pixel
     * @param hwnd Window handle (HWND on Windows)
     * @param color Color map mode
     * @return true on success
     */
    bool Open(uint32_t cols, uint32_t rows, uint32_t pixel_depth,
              void* hwnd, XColor color = XCOLOR_GRAY);
    
    /**
     * @brief Open display window using detector parameters
     * @param det Detector configuration
     * @param rows Number of rows to display
     * @param hwnd Window handle
     * @param color Color map mode
     * @return true on success
     */
    bool Open(XDetector& det, uint32_t rows, void* hwnd,
              XColor color = XCOLOR_GRAY);
    
    /**
     * @brief Close display
     */
    void Close();
    
    /**
     * @brief Check if display is open
     * @return true if open
     */
    bool IsOpen();
    
    /**
     * @brief Display image
     * @param img_ Image to display
     */
    void Show(XImage* img_);
    
    /**
     * @brief Set gamma correction value
     * @param gama Gamma value (1.0 - 4.0)
     */
    void SetGama(float gama);
    
    /**
     * @brief Get current gamma value
     * @return Current gamma value
     */
    float GetGama();
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XShow(const XShow&) = delete;
    XShow& operator=(const XShow&) = delete;
};

} // namespace HX

#endif // XSHOW_H