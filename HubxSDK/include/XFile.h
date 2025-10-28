

// ============================================================================

/**
 * @file XFile.h
 * @brief XFile class - TIFF file operations
 * @version 2.1.0
 */

#ifndef XFILE_H
#define XFILE_H

#include <cstdint>
#include <string>

namespace HX {

class XImage;
class XDetector;

/**
 * @class XFile
 * @brief Handles TIFF image file operations with metadata
 */
class XFile {
public:
    /**
     * @enum XFCode
     * @brief File parameter codes
     */
    enum XFCode {
        XF_COLS = 0,        ///< Image columns
        XF_ROWS,            ///< Image rows
        XF_DEPTH,           ///< Pixel depth (bits)
        XF_DM_NUM,          ///< Number of DM modules
        XF_DM_TYPE,         ///< Detector type
        XF_DM_PIX,          ///< Pixels per DM
        XF_OP_MODE,         ///< Operation mode
        XF_INT_TIME,        ///< Integration time
        XF_ENERGY,          ///< Energy mode (single/dual)
        XF_BIN,             ///< Pixel binning mode
        XF_TEMP,            ///< Temperature
        XF_HUM,             ///< Humidity
        XF_DATA,            ///< Image data pointer
        XF_SN,              ///< Serial number
        XF_DATE             ///< Date/time string
    };
    
    XFile();
    XFile(XImage* image_, XDetector& det);
    ~XFile();
    
    /**
     * @brief Read TIFF file
     * @param file File path
     * @return true on success
     */
    bool Read(const std::string& file);
    
    /**
     * @brief Write TIFF file
     * @param file File path
     * @return true on success
     */
    bool Write(const std::string& file);
    
    /**
     * @brief Get parameter value (uint32_t)
     * @param code Parameter code
     * @param data Output value
     * @return true on success
     */
    bool Get(XFCode code, uint32_t& data);
    
    /**
     * @brief Get parameter value (float)
     * @param code Parameter code
     * @param data Output value
     * @return true on success
     */
    bool Get(XFCode code, float& data);
    
    /**
     * @brief Get parameter value (pointer)
     * @param code Parameter code
     * @param data_ Output pointer
     * @return true on success
     */
    bool Get(XFCode code, uint8_t** data_);
    
    /**
     * @brief Set parameter value (uint32_t)
     * @param code Parameter code
     * @param data Value to set
     * @return true on success
     */
    bool Set(XFCode code, uint32_t data);
    
    /**
     * @brief Set parameter value (float)
     * @param code Parameter code
     * @param data Value to set
     * @return true on success
     */
    bool Set(XFCode code, float data);
    
    /**
     * @brief Set parameter value (pointer)
     * @param code Parameter code
     * @param data_ Pointer to set
     * @return true on success
     */
    bool Set(XFCode code, uint8_t* data_);
    
private:
    class Impl;
    Impl* m_impl;
    
    // Non-copyable
    XFile(const XFile&) = delete;
    XFile& operator=(const XFile&) = delete;
};

} // namespace HX

#endif // XFILE_H