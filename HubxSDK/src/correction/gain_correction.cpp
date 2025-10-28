/**
 * @file gain_correction.cpp
 * @brief Gain correction algorithm implementation for FXImage 2.1.0
 * @details Implements single-gain and multi-gain correction using the formula: y = k(x - x₀) + b
 *          where k is the gain coefficient, x₀ is the offset, and b is the baseline
 * @author FXImage Development Team
 * @date 2025
 */

#include "../../include/xog_correct.h"
#include "../../include/xmg_correct.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace fximage {

struct GainCorrectionParams {
    int bit_depth;
    const unsigned short* offset_data;
    const unsigned short* baseline_data;
    const float* gain_coeffs;
    bool enable_offset;
    bool enable_baseline;
    bool enable_gain;
    unsigned short target_baseline;

    GainCorrectionParams()
        : bit_depth(16),
          offset_data(nullptr),
          baseline_data(nullptr),
          gain_coeffs(nullptr),
          enable_offset(false),
          enable_baseline(false),
          enable_gain(false),
          target_baseline(0) {}
};

/**
 * @brief Calculate gain coefficients from target values
 * @param raw_data Raw calibration data (averaged background-subtracted values)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param target_value Desired output value after correction
 * @param gain_coeffs Output array for gain coefficients (must be pre-allocated)
 * @return true on success, false on failure
 */
bool CalculateGainCoefficients(const unsigned short* raw_data,
                               int width,
                               int height,
                               unsigned short target_value,
                               float* gain_coeffs)
{
    if (!raw_data || !gain_coeffs || width <= 0 || height <= 0) {
        return false;
    }

    const int total_pixels = width * height;
    
    // Calculate gain coefficient for each pixel: k = target / (raw_value)
    for (int i = 0; i < total_pixels; ++i) {
        if (raw_data[i] > 0) {
            gain_coeffs[i] = static_cast<float>(target_value) / static_cast<float>(raw_data[i]);
        } else {
            // Handle zero or invalid values - use neighboring average
            gain_coeffs[i] = 1.0f;
        }
        
        // Clamp gain coefficients to reasonable range to avoid artifacts
        if (gain_coeffs[i] < 0.1f) gain_coeffs[i] = 0.1f;
        if (gain_coeffs[i] > 10.0f) gain_coeffs[i] = 10.0f;
    }

    return true;
}

/**
 * @brief Apply single-gain correction to image data
 * @param input_data Input raw image data
 * @param output_data Output corrected image data
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param offset_data Background offset values (x₀)
 * @param gain_coeffs Gain coefficients (k)
 * @param baseline Baseline value (b) - typically 0
 * @param bit_depth Output bit depth (12 or 14 or 16)
 * @return true on success, false on failure
 */
bool ApplySingleGainCorrection(const unsigned short* input_data,
                               unsigned short* output_data,
                               int width,
                               int height,
                               const unsigned short* offset_data,
                               const float* gain_coeffs,
                               unsigned short baseline,
                               int bit_depth)
{
    if (!input_data || !output_data || !offset_data || !gain_coeffs) {
        return false;
    }

    if (width <= 0 || height <= 0) {
        return false;
    }

    const int total_pixels = width * height;
    const unsigned short max_value = (1 << bit_depth) - 1;

    // Apply correction: y = k(x - x₀) + b
    for (int i = 0; i < total_pixels; ++i) {
        // Subtract offset
        int corrected = static_cast<int>(input_data[i]) - static_cast<int>(offset_data[i]);
        
        // Apply gain
        float result = gain_coeffs[i] * static_cast<float>(corrected) + static_cast<float>(baseline);
        
        // Clamp to valid range
        if (result < 0.0f) {
            output_data[i] = 0;
        } else if (result > max_value) {
            output_data[i] = max_value;
        } else {
            output_data[i] = static_cast<unsigned short>(result + 0.5f);
        }
    }

    return true;
}

/**
 * @brief Apply gain correction with optional offset and baseline correction
 * @param input_data Input raw image data
 * @param output_data Output corrected image data
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param params Correction parameters structure
 * @return true on success, false on failure
 */
bool ApplyGainCorrection(const unsigned short* input_data,
                        unsigned short* output_data,
                        int width,
                        int height,
                        const GainCorrectionParams& params)
{
    if (!input_data || !output_data || width <= 0 || height <= 0) {
        return false;
    }

    const int total_pixels = width * height;
    const unsigned short max_value = (1 << params.bit_depth) - 1;

    for (int i = 0; i < total_pixels; ++i) {
        float corrected = static_cast<float>(input_data[i]);

        // Step 1: Apply offset correction if enabled
        if (params.enable_offset && params.offset_data) {
            corrected -= static_cast<float>(params.offset_data[i]);
        }

        // Step 2: Apply baseline correction if enabled
        if (params.enable_baseline && params.baseline_data) {
            corrected -= static_cast<float>(params.baseline_data[i]);
        }

        // Step 3: Apply gain correction if enabled
        if (params.enable_gain && params.gain_coeffs) {
            corrected *= params.gain_coeffs[i];
        }

        // Step 4: Add target baseline
        corrected += static_cast<float>(params.target_baseline);

        // Step 5: Clamp to valid range
        if (corrected < 0.0f) {
            output_data[i] = 0;
        } else if (corrected > max_value) {
            output_data[i] = max_value;
        } else {
            output_data[i] = static_cast<unsigned short>(corrected + 0.5f);
        }
    }

    return true;
}

/**
 * @brief Validate gain correction data integrity
 * @param gain_coeffs Gain coefficient array
 * @param width Image width
 * @param height Image height
 * @return true if valid, false otherwise
 */
bool ValidateGainData(const float* gain_coeffs, int width, int height)
{
    if (!gain_coeffs || width <= 0 || height <= 0) {
        return false;
    }

    const int total_pixels = width * height;
    int invalid_count = 0;

    for (int i = 0; i < total_pixels; ++i) {
        if (std::isnan(gain_coeffs[i]) || std::isinf(gain_coeffs[i])) {
            invalid_count++;
        }
        if (gain_coeffs[i] <= 0.0f || gain_coeffs[i] > 100.0f) {
            invalid_count++;
        }
    }

    // Allow up to 0.1% invalid pixels
    return invalid_count < (total_pixels / 1000);
}

/**
 * @brief Smooth gain coefficients to reduce noise
 * @param gain_coeffs Input/output gain coefficients
 * @param width Image width
 * @param height Image height
 * @param kernel_size Smoothing kernel size (3, 5, or 7)
 */
void SmoothGainCoefficients(float* gain_coeffs, int width, int height, int kernel_size)
{
    if (!gain_coeffs || width <= 0 || height <= 0) {
        return;
    }

    if (kernel_size != 3 && kernel_size != 5 && kernel_size != 7) {
        kernel_size = 3;
    }

    std::vector<float> temp(width * height);
    std::memcpy(temp.data(), gain_coeffs, width * height * sizeof(float));

    const int half_kernel = kernel_size / 2;

    for (int y = half_kernel; y < height - half_kernel; ++y) {
        for (int x = half_kernel; x < width - half_kernel; ++x) {
            float sum = 0.0f;
            int count = 0;

            for (int ky = -half_kernel; ky <= half_kernel; ++ky) {
                for (int kx = -half_kernel; kx <= half_kernel; ++kx) {
                    int idx = (y + ky) * width + (x + kx);
                    sum += temp[idx];
                    count++;
                }
            }

            gain_coeffs[y * width + x] = sum / count;
        }
    }
}

/**
 * @brief Calculate statistics for gain correction quality assessment
 * @param gain_coeffs Gain coefficients
 * @param width Image width
 * @param height Image height
 * @param mean Output mean value
 * @param std_dev Output standard deviation
 * @param min_val Output minimum value
 * @param max_val Output maximum value
 */
void CalculateGainStatistics(const float* gain_coeffs,
                            int width,
                            int height,
                            float& mean,
                            float& std_dev,
                            float& min_val,
                            float& max_val)
{
    if (!gain_coeffs || width <= 0 || height <= 0) {
        mean = std_dev = 0.0f;
        min_val = max_val = 0.0f;
        return;
    }

    const int total_pixels = width * height;
    
    mean = 0.0f;
    min_val = gain_coeffs[0];
    max_val = gain_coeffs[0];

    // Calculate mean, min, max
    for (int i = 0; i < total_pixels; ++i) {
        mean += gain_coeffs[i];
        if (gain_coeffs[i] < min_val) min_val = gain_coeffs[i];
        if (gain_coeffs[i] > max_val) max_val = gain_coeffs[i];
    }
    mean /= total_pixels;

    // Calculate standard deviation
    std_dev = 0.0f;
    for (int i = 0; i < total_pixels; ++i) {
        float diff = gain_coeffs[i] - mean;
        std_dev += diff * diff;
    }
    std_dev = std::sqrt(std_dev / total_pixels);
}

} // namespace fximage