/**
 * @file XImage.h
 * @brief XImage class - Image data container
 * @version 2.1.0
 */

#ifndef XIMAGE_H
#define XIMAGE_H

#include <cstdint>

namespace HX {

/**
 * @class XImage
 * @brief Encapsulates frame image data and metadata
 */
class XImage {
public:
    XImage();
    XImage(uint32_t width, uint32_t height, uint8_t pixelDepth);
    ~XImage();
    
    /**
     * @brief Get pixel value at specified coordinates
     * @param row Row index
     * @param col Column index
     * @return Pixel value
     */
    uint32_t GetPixelVal(uint32_t row, uint32_t col) const;
    
    /**
     * @brief Set pixel value at specified coordinates
     * @param row Row index
     * @param col Column index
     * @param pixel_value Pixel value to set
     */
    void SetPixelVal(uint32_t row, uint32_t col, uint32_t pixel_value);
    
    /**
     * @brief Save image to text file
     * @param file_name_ File path
     * @return true on success
     */
    bool Save(const char* file_name_) const;
    
    /**
     * @brief Set image data pointer
     * @param data Data pointer
     * @param width Image width
     * @param height Image height
     * @param pixelDepth Bits per pixel
     * @param takeOwnership If true, XImage will free the data
     */
    void SetData(uint8_t* data, uint32_t width, uint32_t height,
                 uint8_t pixelDepth, bool takeOwnership = false);
    
    /**
     * @brief Clear image data (set to zero)
     */
    void Clear();
    
    /**
     * @brief Clone this image
     * @return Pointer to new XImage instance
     */
    XImage* Clone() const;
    
    // Public members (for direct access)
    uint8_t* _data_;            ///< Image data pointer
    uint32_t _data_offset;      ///< Offset to first pixel
    uint32_t _height;           ///< Number of rows
    uint32_t _width;            ///< Number of columns
    uint8_t _pixel_depth;       ///< Bits per pixel
    uint32_t _size;             ///< Total size in bytes
    
private:
    void allocateMemory();
    void freeMemory();
    
    bool m_ownsData;
};

} // namespace HX

#endif // XIMAGE_H