/**
 * @file xmg_correct.cpp
 * @brief Multi-gain correction algorithm implementation for FXImage 2.1.0
 * @details Implements multi-gain correction for different detector gain modes
 *          Supports multiple gain ranges and automatic gain switching
 * @author FXImage Development Team
 * @date 2025
 */

#include "../../include/xmg_correct.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace fximage {

/**
 * @brief Multi-gain correction parameters structure
 */
struct MultiGainParams {
    int num_gains;                      // Number of gain modes (typically 2-4)
    unsigned short* thresholds;         // Threshold values for gain switching
    float** gain_coeffs;                // Gain coefficients for each mode
    unsigned short** offset_data;       // Offset data for each mode
    unsigned short* baseline_data;      // Baseline correction data
    int bit_depth;                      // Output bit depth
    bool auto_switch;                   // Automatic gain mode switching
};

/**
 * @brief Initialize multi-gain correction structure
 * @param params Multi-gain parameters
 * @param num_gains Number of gain modes
 * @param width Image width
 * @param height Image height
 * @return true on success, false on failure
 */
bool InitMultiGainCorrection(MultiGainParams& params,
                            int num_gains,
                            int width,
                            int height)
{
    if (num_gains <= 0 || num_gains > 8 || width <= 0 || height <= 0) {
        return false;
    }

    params.num_gains = num_gains;
    params.bit_depth = 14; // Default 14-bit
    params.auto_switch = true;

    // Allocate threshold array
    params.thresholds = new unsigned short[num_gains];
    
    // Allocate gain coefficient arrays
    params.gain_coeffs = new float*[num_gains];
    params.offset_data = new unsigned short*[num_gains];
    
    const int total_pixels = width * height;
    
    for (int i = 0; i < num_gains; ++i) {
        params.gain_coeffs[i] = new float[total_pixels];
        params.offset_data[i] = new unsigned short[total_pixels];
        
        // Initialize with default values
        std::fill_n(params.gain_coeffs[i], total_pixels, 1.0f);
        std::fill_n(params.offset_data[i], total_pixels, 0);
    }

    // Allocate baseline data
    params.baseline_data = new unsigned short[total_pixels];
    std::fill_n(params.baseline_data, total_pixels, 0);

    // Set default thresholds (evenly distributed)
    unsigned short max_value = (1 << params.bit_depth) - 1;
    for (int i = 0; i < num_gains; ++i) {
        params.thresholds[i] = (max_value * (i + 1)) / num_gains;
    }

    return true;
}

/**
 * @brief Release multi-gain correction resources
 * @param params Multi-gain parameters to release
 */
void ReleaseMultiGainCorrection(MultiGainParams& params)
{
    if (params.thresholds) {
        delete[] params.thresholds;
        params.thresholds = nullptr;
    }

    if (params.gain_coeffs) {
        for (int i = 0; i < params.num_gains; ++i) {
            if (params.gain_coeffs[i]) {
                delete[] params.gain_coeffs[i];
            }
        }
        delete[] params.gain_coeffs;
        params.gain_coeffs = nullptr;
    }

    if (params.offset_data) {
        for (int i = 0; i < params.num_gains; ++i) {
            if (params.offset_data[i]) {
                delete[] params.offset_data[i];
            }
        }
        delete[] params.offset_data;
        params.offset_data = nullptr;
    }

    if (params.baseline_data) {
        delete[] params.baseline_data;
        params.baseline_data = nullptr;
    }
}

/**
 * @brief Determine appropriate gain mode for a pixel value
 * @param value Input pixel value
 * @param thresholds Array of threshold values
 * @param num_gains Number of gain modes
 * @return Selected gain mode index
 */
inline int SelectGainMode(unsigned short value,
                         const unsigned short* thresholds,
                         int num_gains)
{
    for (int i = 0; i < num_gains - 1; ++i) {
        if (value < thresholds[i]) {
            return i;
        }
    }
    return num_gains - 1; // Highest gain mode
}

/**
 * @brief Apply multi-gain correction to image data
 * @param input_data Input raw image data
 * @param output_data Output corrected image data
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param params Multi-gain correction parameters
 * @param gain_mode Fixed gain mode (-1 for automatic selection)
 * @return true on success, false on failure
 */
bool ApplyMultiGainCorrection(const unsigned short* input_data,
                             unsigned short* output_data,
                             int width,
                             int height,
                             const MultiGainParams& params,
                             int gain_mode)
{
    if (!input_data || !output_data || width <= 0 || height <= 0) {
        return false;
    }

    if (!params.gain_coeffs || !params.offset_data) {
        return false;
    }

    const int total_pixels = width * height;
    const unsigned short max_value = (1 << params.bit_depth) - 1;

    for (int i = 0; i < total_pixels; ++i) {
        int selected_mode;
        
        // Determine gain mode
        if (gain_mode >= 0 && gain_mode < params.num_gains) {
            // Use fixed gain mode
            selected_mode = gain_mode;
        } else if (params.auto_switch) {
            // Automatic gain mode selection based on pixel value
            selected_mode = SelectGainMode(input_data[i], params.thresholds, params.num_gains);
        } else {
            // Default to first gain mode
            selected_mode = 0;
        }

        // Apply correction: y = k(x - x₀ - baseline) + target
        float corrected = static_cast<float>(input_data[i]);
        
        // Subtract offset
        corrected -= static_cast<float>(params.offset_data[selected_mode][i]);
        
        // Subtract baseline if available
        if (params.baseline_data) {
            corrected -= static_cast<float>(params.baseline_data[i]);
        }
        
        // Apply gain
        corrected *= params.gain_coeffs[selected_mode][i];
        
        // Clamp to valid range
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
 * @brief Calculate multi-gain coefficients from calibration data
 * @param calibration_data Array of calibration data for each gain mode
 * @param width Image width
 * @param height Image height
 * @param num_gains Number of gain modes
 * @param target_values Target output values for each gain mode
 * @param gain_coeffs Output gain coefficient arrays
 * @return true on success, false on failure
 */
bool CalculateMultiGainCoefficients(const unsigned short** calibration_data,
                                   int width,
                                   int height,
                                   int num_gains,
                                   const unsigned short* target_values,
                                   float** gain_coeffs)
{
    if (!calibration_data || !target_values || !gain_coeffs) {
        return false;
    }

    if (width <= 0 || height <= 0 || num_gains <= 0) {
        return false;
    }

    const int total_pixels = width * height;

    for (int mode = 0; mode < num_gains; ++mode) {
        if (!calibration_data[mode] || !gain_coeffs[mode]) {
            return false;
        }

        unsigned short target = target_values[mode];

        for (int i = 0; i < total_pixels; ++i) {
            if (calibration_data[mode][i] > 0) {
                gain_coeffs[mode][i] = static_cast<float>(target) / 
                                      static_cast<float>(calibration_data[mode][i]);
            } else {
                gain_coeffs[mode][i] = 1.0f;
            }

            // Clamp to reasonable range
            if (gain_coeffs[mode][i] < 0.1f) gain_coeffs[mode][i] = 0.1f;
            if (gain_coeffs[mode][i] > 10.0f) gain_coeffs[mode][i] = 10.0f;
        }
    }

    return true;
}

/**
 * @brief Apply multi-gain correction with blending at transitions
 * @param input_data Input raw image data
 * @param output_data Output corrected image data
 * @param width Image width
 * @param height Image height
 * @param params Multi-gain parameters
 * @param blend_width Width of transition blending region
 * @return true on success, false on failure
 */
bool ApplyMultiGainWithBlending(const unsigned short* input_data,
                               unsigned short* output_data,
                               int width,
                               int height,
                               const MultiGainParams& params,
                               int blend_width)
{
    if (!input_data || !output_data || width <= 0 || height <= 0) {
        return false;
    }

    if (blend_width <= 0) {
        // No blending, use standard correction
        return ApplyMultiGainCorrection(input_data, output_data, width, height, params, -1);
    }

    const int total_pixels = width * height;
    const unsigned short max_value = (1 << params.bit_depth) - 1;

    for (int i = 0; i < total_pixels; ++i) {
        unsigned short input_val = input_data[i];
        
        // Find appropriate gain mode
        int mode = SelectGainMode(input_val, params.thresholds, params.num_gains);
        
        // Check if we're in a blending region
        float blend_factor = 0.0f;
        int blend_mode = -1;
        
        if (mode > 0) {
            // Check lower threshold
            int dist_to_lower = input_val - params.thresholds[mode - 1];
            if (dist_to_lower < blend_width && dist_to_lower >= 0) {
                blend_factor = static_cast<float>(dist_to_lower) / blend_width;
                blend_mode = mode - 1;
            }
        }
        
        if (mode < params.num_gains - 1 && blend_mode < 0) {
            // Check upper threshold
            int dist_to_upper = params.thresholds[mode] - input_val;
            if (dist_to_upper < blend_width && dist_to_upper >= 0) {
                blend_factor = static_cast<float>(dist_to_upper) / blend_width;
                blend_mode = mode + 1;
            }
        }
        
        float result;
        
        if (blend_mode >= 0 && blend_factor > 0.0f) {
            // Apply correction with two gain modes and blend
            float corrected1 = static_cast<float>(input_val) - 
                              static_cast<float>(params.offset_data[mode][i]);
            if (params.baseline_data) {
                corrected1 -= static_cast<float>(params.baseline_data[i]);
            }
            corrected1 *= params.gain_coeffs[mode][i];
            
            float corrected2 = static_cast<float>(input_val) - 
                              static_cast<float>(params.offset_data[blend_mode][i]);
            if (params.baseline_data) {
                corrected2 -= static_cast<float>(params.baseline_data[i]);
            }
            corrected2 *= params.gain_coeffs[blend_mode][i];
            
            // Linear blend between the two corrections
            result = corrected1 * blend_factor + corrected2 * (1.0f - blend_factor);
        } else {
            // Standard single-mode correction
            result = static_cast<float>(input_val) - 
                    static_cast<float>(params.offset_data[mode][i]);
            if (params.baseline_data) {
                result -= static_cast<float>(params.baseline_data[i]);
            }
            result *= params.gain_coeffs[mode][i];
        }
        
        // Clamp and store
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
 * @brief Optimize gain mode thresholds based on histogram analysis
 * @param histogram Histogram of image data
 * @param histogram_size Size of histogram array
 * @param num_gains Number of gain modes
 * @param thresholds Output optimized threshold values
 * @return true on success, false on failure
 */
bool OptimizeGainThresholds(const unsigned int* histogram,
                           int histogram_size,
                           int num_gains,
                           unsigned short* thresholds)
{
    if (!histogram || !thresholds || histogram_size <= 0 || num_gains <= 1) {
        return false;
    }

    // Calculate cumulative histogram
    std::vector<unsigned long long> cumulative(histogram_size);
    cumulative[0] = histogram[0];
    for (int i = 1; i < histogram_size; ++i) {
        cumulative[i] = cumulative[i-1] + histogram[i];
    }

    unsigned long long total_pixels = cumulative[histogram_size - 1];
    if (total_pixels == 0) {
        return false;
    }

    // Set thresholds at equal percentile intervals
    for (int i = 0; i < num_gains - 1; ++i) {
        unsigned long long target_count = total_pixels * (i + 1) / num_gains;
        
        // Binary search for threshold
        int left = 0, right = histogram_size - 1;
        while (left < right) {
            int mid = (left + right) / 2;
            if (cumulative[mid] < target_count) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        thresholds[i] = static_cast<unsigned short>(left);
    }

    return true;
}

/**
 * @brief Validate multi-gain correction data
 * @param params Multi-gain parameters
 * @param width Image width
 * @param height Image height
 * @return true if valid, false otherwise
 */
bool ValidateMultiGainData(const MultiGainParams& params, int width, int height)
{
    if (params.num_gains <= 0 || params.num_gains > 8) {
        return false;
    }

    if (!params.thresholds || !params.gain_coeffs || !params.offset_data) {
        return false;
    }

    const int total_pixels = width * height;
    
    // Check threshold ordering
    for (int i = 0; i < params.num_gains - 2; ++i) {
        if (params.thresholds[i] >= params.thresholds[i + 1]) {
            return false;
        }
    }

    // Validate gain coefficients
    int invalid_count = 0;
    for (int mode = 0; mode < params.num_gains; ++mode) {
        if (!params.gain_coeffs[mode] || !params.offset_data[mode]) {
            return false;
        }

        for (int i = 0; i < total_pixels; ++i) {
            if (std::isnan(params.gain_coeffs[mode][i]) || 
                std::isinf(params.gain_coeffs[mode][i]) ||
                params.gain_coeffs[mode][i] <= 0.0f ||
                params.gain_coeffs[mode][i] > 100.0f) {
                invalid_count++;
            }
        }
    }

    // Allow up to 0.1% invalid pixels per mode
    return invalid_count < (total_pixels * params.num_gains / 1000);
}

/**
 * @brief Calculate statistics for each gain mode
 * @param params Multi-gain parameters
 * @param width Image width
 * @param height Image height
 * @param mode Gain mode to analyze
 * @param mean Output mean gain coefficient
 * @param std_dev Output standard deviation
 * @param min_val Output minimum value
 * @param max_val Output maximum value
 */
void CalculateGainModeStatistics(const MultiGainParams& params,
                                int width,
                                int height,
                                int mode,
                                float& mean,
                                float& std_dev,
                                float& min_val,
                                float& max_val)
{
    if (mode < 0 || mode >= params.num_gains || !params.gain_coeffs[mode]) {
        mean = std_dev = min_val = max_val = 0.0f;
        return;
    }

    const int total_pixels = width * height;
    const float* gain_data = params.gain_coeffs[mode];

    mean = 0.0f;
    min_val = gain_data[0];
    max_val = gain_data[0];

    for (int i = 0; i < total_pixels; ++i) {
        mean += gain_data[i];
        if (gain_data[i] < min_val) min_val = gain_data[i];
        if (gain_data[i] > max_val) max_val = gain_data[i];
    }
    mean /= total_pixels;

    std_dev = 0.0f;
    for (int i = 0; i < total_pixels; ++i) {
        float diff = gain_data[i] - mean;
        std_dev += diff * diff;
    }
    std_dev = std::sqrt(std_dev / total_pixels);
}

/**
 * @brief Create histogram of gain mode usage
 * @param input_data Input image data
 * @param width Image width
 * @param height Image height
 * @param params Multi-gain parameters
 * @param mode_histogram Output histogram (must be pre-allocated)
 * @return true on success, false on failure
 */
bool CreateGainModeHistogram(const unsigned short* input_data,
                            int width,
                            int height,
                            const MultiGainParams& params,
                            unsigned int* mode_histogram)
{
    if (!input_data || !mode_histogram || width <= 0 || height <= 0) {
        return false;
    }

    // Initialize histogram
    std::fill_n(mode_histogram, params.num_gains, 0);

    const int total_pixels = width * height;

    for (int i = 0; i < total_pixels; ++i) {
        int mode = SelectGainMode(input_data[i], params.thresholds, params.num_gains);
        mode_histogram[mode]++;
    }

    return true;
}

} // namespace fximage