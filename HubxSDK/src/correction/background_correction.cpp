/**
 * @file background_correction.cpp
 * @brief Background (offset/bias) correction implementation for X-ray detector images
 * @details Implements background value correction using formula: y = k(x - x₀) + b
 *          where x₀ is the background offset value
 * 
 * FXImage 2.1.0 - HubxSDK
 * Copyright (c) 2025
 */

#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <memory>

// Error codes
#define HUBX_SUCCESS 0
#define HUBX_ERROR_INVALID_PARAM -1
#define HUBX_ERROR_NULL_POINTER -2
#define HUBX_ERROR_BUFFER_SIZE -3
#define HUBX_ERROR_CALCULATION -4

namespace HubxSDK {
namespace Correction {

/**
 * @class BackgroundCorrection
 * @brief Handles background/offset correction for detector images
 */
class BackgroundCorrection {
public:
    BackgroundCorrection() : m_initialized(false), m_width(0), m_height(0) {}
    
    ~BackgroundCorrection() {
        release();
    }

    /**
     * @brief Initialize background correction with image dimensions
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int initialize(int width, int height) {
        if (width <= 0 || height <= 0) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        m_width = width;
        m_height = height;
        m_pixelCount = width * height;
        
        // Allocate background offset buffer
        m_backgroundOffset.resize(m_pixelCount, 0.0f);
        
        m_initialized = true;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Calculate background offset from multiple frames (typically 4096 lines)
     * @param frames Array of frame pointers
     * @param frameCount Number of frames to average
     * @param bitDepth Bit depth of input data (12, 14, or 16)
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int calculateBackgroundOffset(const unsigned short** frames, int frameCount, int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }
        
        if (frames == nullptr || frameCount <= 0) {
            return HUBX_ERROR_NULL_POINTER;
        }

        if (bitDepth != 12 && bitDepth != 14 && bitDepth != 16) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        try {
            // Accumulate pixel values across all frames
            std::vector<double> accumulator(m_pixelCount, 0.0);

            for (int frame = 0; frame < frameCount; ++frame) {
                if (frames[frame] == nullptr) {
                    return HUBX_ERROR_NULL_POINTER;
                }

                for (int i = 0; i < m_pixelCount; ++i) {
                    accumulator[i] += static_cast<double>(frames[frame][i]);
                }
            }

            // Calculate average
            const double invFrameCount = 1.0 / static_cast<double>(frameCount);
            for (int i = 0; i < m_pixelCount; ++i) {
                m_backgroundOffset[i] = static_cast<float>(accumulator[i] * invFrameCount);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Calculate background offset from line data (for line scan detectors)
     * @param lines Array of line data pointers
     * @param lineCount Number of lines (typically 4096)
     * @param lineWidth Width of each line
     * @param bitDepth Bit depth of input data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int calculateBackgroundOffsetFromLines(const unsigned short** lines, int lineCount, 
                                          int lineWidth, int bitDepth) {
        if (!m_initialized || lineWidth != m_width) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (lines == nullptr || lineCount <= 0) {
            return HUBX_ERROR_NULL_POINTER;
        }

        try {
            // For line scan detectors, calculate average per pixel position
            std::vector<double> accumulator(lineWidth, 0.0);

            for (int line = 0; line < lineCount; ++line) {
                if (lines[line] == nullptr) {
                    return HUBX_ERROR_NULL_POINTER;
                }

                for (int col = 0; col < lineWidth; ++col) {
                    accumulator[col] += static_cast<double>(lines[line][col]);
                }
            }

            // Calculate average and replicate across height
            const double invLineCount = 1.0 / static_cast<double>(lineCount);
            for (int row = 0; row < m_height; ++row) {
                for (int col = 0; col < m_width; ++col) {
                    m_backgroundOffset[row * m_width + col] = 
                        static_cast<float>(accumulator[col] * invLineCount);
                }
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Apply background correction to an image
     * @param input Input image data
     * @param output Output corrected image data
     * @param gain Gain coefficient (k in formula y = k(x - x₀) + b)
     * @param bias Bias value (b in formula)
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int applyCorrection(const unsigned short* input, unsigned short* output,
                       float gain, float bias, int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (input == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                // Apply formula: y = k(x - x₀) + b
                float corrected = gain * (static_cast<float>(input[i]) - m_backgroundOffset[i]) + bias;
                
                // Clamp to valid range
                corrected = std::max(0.0f, std::min(static_cast<float>(maxValue), corrected));
                
                output[i] = static_cast<unsigned short>(corrected + 0.5f); // Round to nearest
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Apply background correction with per-pixel gain (for non-uniform correction)
     * @param input Input image data
     * @param output Output corrected image data
     * @param gainMap Per-pixel gain coefficients
     * @param bias Global bias value
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int applyCorrectionWithGainMap(const unsigned short* input, unsigned short* output,
                                   const float* gainMap, float bias, int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (input == nullptr || output == nullptr || gainMap == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                // Apply formula with per-pixel gain: y = k[i](x - x₀[i]) + b
                float corrected = gainMap[i] * (static_cast<float>(input[i]) - m_backgroundOffset[i]) + bias;
                
                // Clamp to valid range
                corrected = std::max(0.0f, std::min(static_cast<float>(maxValue), corrected));
                
                output[i] = static_cast<unsigned short>(corrected + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Set background offset data from file or external source
     * @param offsetData Background offset values
     * @param dataSize Size of offset data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int setBackgroundOffset(const float* offsetData, int dataSize) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (offsetData == nullptr || dataSize != m_pixelCount) {
            return HUBX_ERROR_BUFFER_SIZE;
        }

        std::memcpy(m_backgroundOffset.data(), offsetData, dataSize * sizeof(float));
        return HUBX_SUCCESS;
    }

    /**
     * @brief Get calculated background offset data
     * @param offsetData Output buffer for offset values
     * @param bufferSize Size of output buffer
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int getBackgroundOffset(float* offsetData, int bufferSize) const {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (offsetData == nullptr || bufferSize < m_pixelCount) {
            return HUBX_ERROR_BUFFER_SIZE;
        }

        std::memcpy(offsetData, m_backgroundOffset.data(), m_pixelCount * sizeof(float));
        return HUBX_SUCCESS;
    }

    /**
     * @brief Save background offset to binary file
     * @param filename Output file path
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int saveToFile(const char* filename) const {
        if (!m_initialized || filename == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        FILE* file = fopen(filename, "wb");
        if (file == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        // Write header
        fwrite(&m_width, sizeof(int), 1, file);
        fwrite(&m_height, sizeof(int), 1, file);

        // Write offset data
        fwrite(m_backgroundOffset.data(), sizeof(float), m_pixelCount, file);

        fclose(file);
        return HUBX_SUCCESS;
    }

    /**
     * @brief Load background offset from binary file
     * @param filename Input file path
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int loadFromFile(const char* filename) {
        if (filename == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        FILE* file = fopen(filename, "rb");
        if (file == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        // Read header
        int width, height;
        fread(&width, sizeof(int), 1, file);
        fread(&height, sizeof(int), 1, file);

        // Initialize if needed
        if (!m_initialized || width != m_width || height != m_height) {
            int result = initialize(width, height);
            if (result != HUBX_SUCCESS) {
                fclose(file);
                return result;
            }
        }

        // Read offset data
        fread(m_backgroundOffset.data(), sizeof(float), m_pixelCount, file);

        fclose(file);
        return HUBX_SUCCESS;
    }

    /**
     * @brief Release resources
     */
    void release() {
        m_backgroundOffset.clear();
        m_initialized = false;
        m_width = 0;
        m_height = 0;
        m_pixelCount = 0;
    }

private:
    bool m_initialized;
    int m_width;
    int m_height;
    int m_pixelCount;
    std::vector<float> m_backgroundOffset;
};

// Global instance for C-style API
static BackgroundCorrection g_backgroundCorrection;

} // namespace Correction
} // namespace HubxSDK

// C-style API for compatibility
extern "C" {

/**
 * @brief Initialize background correction module
 */
int hubx_background_init(int width, int height) {
    return HubxSDK::Correction::g_backgroundCorrection.initialize(width, height);
}

/**
 * @brief Calculate background offset from frames
 */
int hubx_background_calculate(const unsigned short** frames, int frameCount, int bitDepth) {
    return HubxSDK::Correction::g_backgroundCorrection.calculateBackgroundOffset(frames, frameCount, bitDepth);
}

/**
 * @brief Calculate background offset from line data
 */
int hubx_background_calculate_lines(const unsigned short** lines, int lineCount, int lineWidth, int bitDepth) {
    return HubxSDK::Correction::g_backgroundCorrection.calculateBackgroundOffsetFromLines(lines, lineCount, lineWidth, bitDepth);
}

/**
 * @brief Apply background correction
 */
int hubx_background_apply(const unsigned short* input, unsigned short* output,
                         float gain, float bias, int bitDepth) {
    return HubxSDK::Correction::g_backgroundCorrection.applyCorrection(input, output, gain, bias, bitDepth);
}

/**
 * @brief Apply background correction with gain map
 */
int hubx_background_apply_gainmap(const unsigned short* input, unsigned short* output,
                                 const float* gainMap, float bias, int bitDepth) {
    return HubxSDK::Correction::g_backgroundCorrection.applyCorrectionWithGainMap(input, output, gainMap, bias, bitDepth);
}

/**
 * @brief Save background offset to file
 */
int hubx_background_save(const char* filename) {
    return HubxSDK::Correction::g_backgroundCorrection.saveToFile(filename);
}

/**
 * @brief Load background offset from file
 */
int hubx_background_load(const char* filename) {
    return HubxSDK::Correction::g_backgroundCorrection.loadFromFile(filename);
}

/**
 * @brief Release background correction resources
 */
void hubx_background_release() {
    HubxSDK::Correction::g_backgroundCorrection.release();
}

} // extern "C"