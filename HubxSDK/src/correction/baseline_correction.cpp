/**
 * @file baseline_correction.cpp
 * @brief Baseline value correction (reference value calibration) for X-ray detectors
 * @details Implements baseline correction to establish reference values for detector pixels
 *          Used in conjunction with gain correction for complete calibration
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
#define HUBX_ERROR_NOT_CALIBRATED -5

namespace HubxSDK {
namespace Correction {

/**
 * @class BaselineCorrection
 * @brief Handles baseline/reference value correction for detector calibration
 */
class BaselineCorrection {
public:
    BaselineCorrection() 
        : m_initialized(false), 
          m_calibrated(false),
          m_width(0), 
          m_height(0),
          m_targetBaseline(2048.0f) // Default target for 12-bit (middle of range)
    {}
    
    ~BaselineCorrection() {
        release();
    }

    /**
     * @brief Initialize baseline correction with image dimensions
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
        
        // Allocate buffers
        m_baselineValues.resize(m_pixelCount, 0.0f);
        m_baselineCoefficients.resize(m_pixelCount, 0.0f);
        
        m_initialized = true;
        m_calibrated = false;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Set target baseline value (reference value)
     * @param targetValue Target baseline value (e.g., 2048 for 12-bit, 8192 for 14-bit)
     * @param bitDepth Bit depth of detector
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int setTargetBaseline(float targetValue, int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        const int maxValue = (1 << bitDepth) - 1;
        if (targetValue < 0 || targetValue > maxValue) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        m_targetBaseline = targetValue;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Calculate baseline values from calibration frames (no X-ray exposure)
     * @param frames Array of frame pointers (dark frames)
     * @param frameCount Number of frames to average
     * @param bitDepth Bit depth of input data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int calculateBaseline(const unsigned short** frames, int frameCount, int bitDepth) {
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

            // Calculate average baseline for each pixel
            const double invFrameCount = 1.0 / static_cast<double>(frameCount);
            for (int i = 0; i < m_pixelCount; ++i) {
                m_baselineValues[i] = static_cast<float>(accumulator[i] * invFrameCount);
            }

            // Calculate correction coefficients: coeff = target - measured
            for (int i = 0; i < m_pixelCount; ++i) {
                m_baselineCoefficients[i] = m_targetBaseline - m_baselineValues[i];
            }

            m_calibrated = true;
            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Calculate baseline from line data (for line scan detectors)
     * @param lines Array of line data pointers
     * @param lineCount Number of lines to average
     * @param lineWidth Width of each line
     * @param bitDepth Bit depth of input data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int calculateBaselineFromLines(const unsigned short** lines, int lineCount, 
                                   int lineWidth, int bitDepth) {
        if (!m_initialized || lineWidth != m_width) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (lines == nullptr || lineCount <= 0) {
            return HUBX_ERROR_NULL_POINTER;
        }

        try {
            // Calculate average baseline per pixel column
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
                    int idx = row * m_width + col;
                    m_baselineValues[idx] = static_cast<float>(accumulator[col] * invLineCount);
                    m_baselineCoefficients[idx] = m_targetBaseline - m_baselineValues[idx];
                }
            }

            m_calibrated = true;
            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Apply baseline correction to an image
     * @param input Input image data
     * @param output Output corrected image data
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int applyCorrection(const unsigned short* input, unsigned short* output, int bitDepth) {
        if (!m_initialized || !m_calibrated) {
            return m_calibrated ? HUBX_ERROR_INVALID_PARAM : HUBX_ERROR_NOT_CALIBRATED;
        }

        if (input == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                // Apply baseline correction: y = x + (target - baseline)
                float corrected = static_cast<float>(input[i]) + m_baselineCoefficients[i];
                
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
     * @brief Apply baseline correction in-place
     * @param data Image data (modified in-place)
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int applyCorrectionInPlace(unsigned short* data, int bitDepth) {
        if (!m_initialized || !m_calibrated) {
            return m_calibrated ? HUBX_ERROR_INVALID_PARAM : HUBX_ERROR_NOT_CALIBRATED;
        }

        if (data == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                float corrected = static_cast<float>(data[i]) + m_baselineCoefficients[i];
                corrected = std::max(0.0f, std::min(static_cast<float>(maxValue), corrected));
                data[i] = static_cast<unsigned short>(corrected + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Apply baseline correction with additional scaling
     * @param input Input image data
     * @param output Output corrected image data
     * @param scale Scaling factor
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int applyCorrectionWithScale(const unsigned short* input, unsigned short* output,
                                 float scale, int bitDepth) {
        if (!m_initialized || !m_calibrated) {
            return m_calibrated ? HUBX_ERROR_INVALID_PARAM : HUBX_ERROR_NOT_CALIBRATED;
        }

        if (input == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                // Apply baseline correction with scaling: y = (x + coeff) * scale
                float corrected = (static_cast<float>(input[i]) + m_baselineCoefficients[i]) * scale;
                
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
     * @brief Get baseline statistics
     * @param minBaseline Output: minimum baseline value
     * @param maxBaseline Output: maximum baseline value
     * @param avgBaseline Output: average baseline value
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int getStatistics(float* minBaseline, float* maxBaseline, float* avgBaseline) const {
        if (!m_initialized || !m_calibrated) {
            return m_calibrated ? HUBX_ERROR_INVALID_PARAM : HUBX_ERROR_NOT_CALIBRATED;
        }

        if (minBaseline == nullptr || maxBaseline == nullptr || avgBaseline == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        float minVal = m_baselineValues[0];
        float maxVal = m_baselineValues[0];
        double sum = 0.0;

        for (int i = 0; i < m_pixelCount; ++i) {
            float val = m_baselineValues[i];
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
            sum += val;
        }

        *minBaseline = minVal;
        *maxBaseline = maxVal;
        *avgBaseline = static_cast<float>(sum / m_pixelCount);

        return HUBX_SUCCESS;
    }

    /**
     * @brief Set baseline coefficients from external source
     * @param coefficients Baseline correction coefficients
     * @param dataSize Size of coefficient array
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int setBaselineCoefficients(const float* coefficients, int dataSize) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (coefficients == nullptr || dataSize != m_pixelCount) {
            return HUBX_ERROR_BUFFER_SIZE;
        }

        std::memcpy(m_baselineCoefficients.data(), coefficients, dataSize * sizeof(float));
        m_calibrated = true;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Get baseline coefficients
     * @param coefficients Output buffer for coefficients
     * @param bufferSize Size of output buffer
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int getBaselineCoefficients(float* coefficients, int bufferSize) const {
        if (!m_initialized || !m_calibrated) {
            return m_calibrated ? HUBX_ERROR_INVALID_PARAM : HUBX_ERROR_NOT_CALIBRATED;
        }

        if (coefficients == nullptr || bufferSize < m_pixelCount) {
            return HUBX_ERROR_BUFFER_SIZE;
        }

        std::memcpy(coefficients, m_baselineCoefficients.data(), m_pixelCount * sizeof(float));
        return HUBX_SUCCESS;
    }

    /**
     * @brief Save baseline calibration data to file
     * @param filename Output file path
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int saveToFile(const char* filename) const {
        if (!m_initialized || !m_calibrated || filename == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        FILE* file = fopen(filename, "wb");
        if (file == nullptr) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        // Write header
        fwrite(&m_width, sizeof(int), 1, file);
        fwrite(&m_height, sizeof(int), 1, file);
        fwrite(&m_targetBaseline, sizeof(float), 1, file);

        // Write baseline data
        fwrite(m_baselineValues.data(), sizeof(float), m_pixelCount, file);
        fwrite(m_baselineCoefficients.data(), sizeof(float), m_pixelCount, file);

        fclose(file);
        return HUBX_SUCCESS;
    }

    /**
     * @brief Load baseline calibration data from file
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
        float targetBaseline;
        fread(&width, sizeof(int), 1, file);
        fread(&height, sizeof(int), 1, file);
        fread(&targetBaseline, sizeof(float), 1, file);

        // Initialize if needed
        if (!m_initialized || width != m_width || height != m_height) {
            int result = initialize(width, height);
            if (result != HUBX_SUCCESS) {
                fclose(file);
                return result;
            }
        }

        m_targetBaseline = targetBaseline;

        // Read baseline data
        fread(m_baselineValues.data(), sizeof(float), m_pixelCount, file);
        fread(m_baselineCoefficients.data(), sizeof(float), m_pixelCount, file);

        fclose(file);
        m_calibrated = true;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Check if baseline correction is calibrated
     * @return true if calibrated, false otherwise
     */
    bool isCalibrated() const {
        return m_calibrated;
    }

    /**
     * @brief Release resources
     */
    void release() {
        m_baselineValues.clear();
        m_baselineCoefficients.clear();
        m_initialized = false;
        m_calibrated = false;
        m_width = 0;
        m_height = 0;
        m_pixelCount = 0;
    }

private:
    bool m_initialized;
    bool m_calibrated;
    int m_width;
    int m_height;
    int m_pixelCount;
    float m_targetBaseline;
    std::vector<float> m_baselineValues;
    std::vector<float> m_baselineCoefficients;
};

// Global instance for C-style API
static BaselineCorrection g_baselineCorrection;

} // namespace Correction
} // namespace HubxSDK

// C-style API for compatibility
extern "C" {

/**
 * @brief Initialize baseline correction module
 */
int hubx_baseline_init(int width, int height) {
    return HubxSDK::Correction::g_baselineCorrection.initialize(width, height);
}

/**
 * @brief Set target baseline value
 */
int hubx_baseline_set_target(float targetValue, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.setTargetBaseline(targetValue, bitDepth);
}

/**
 * @brief Calculate baseline from frames
 */
int hubx_baseline_calculate(const unsigned short** frames, int frameCount, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.calculateBaseline(frames, frameCount, bitDepth);
}

/**
 * @brief Calculate baseline from line data
 */
int hubx_baseline_calculate_lines(const unsigned short** lines, int lineCount, int lineWidth, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.calculateBaselineFromLines(lines, lineCount, lineWidth, bitDepth);
}

/**
 * @brief Apply baseline correction
 */
int hubx_baseline_apply(const unsigned short* input, unsigned short* output, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.applyCorrection(input, output, bitDepth);
}

/**
 * @brief Apply baseline correction in-place
 */
int hubx_baseline_apply_inplace(unsigned short* data, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.applyCorrectionInPlace(data, bitDepth);
}

/**
 * @brief Apply baseline correction with scaling
 */
int hubx_baseline_apply_scale(const unsigned short* input, unsigned short* output, float scale, int bitDepth) {
    return HubxSDK::Correction::g_baselineCorrection.applyCorrectionWithScale(input, output, scale, bitDepth);
}

/**
 * @brief Get baseline statistics
 */
int hubx_baseline_statistics(float* minBaseline, float* maxBaseline, float* avgBaseline) {
    return HubxSDK::Correction::g_baselineCorrection.getStatistics(minBaseline, maxBaseline, avgBaseline);
}

/**
 * @brief Save baseline data to file
 */
int hubx_baseline_save(const char* filename) {
    return HubxSDK::Correction::g_baselineCorrection.saveToFile(filename);
}

/**
 * @brief Load baseline data from file
 */
int hubx_baseline_load(const char* filename) {
    return HubxSDK::Correction::g_baselineCorrection.loadFromFile(filename);
}

/**
 * @brief Check if calibrated
 */
int hubx_baseline_is_calibrated() {
    return HubxSDK::Correction::g_baselineCorrection.isCalibrated() ? 1 : 0;
}

/**
 * @brief Release baseline correction resources
 */
void hubx_baseline_release() {
    HubxSDK::Correction::g_baselineCorrection.release();
}

} // extern "C"