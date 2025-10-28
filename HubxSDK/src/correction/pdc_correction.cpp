/**
 * @file pdc_correction.cpp
 * @brief Pixel Discontinuity Correction (PDC) algorithm implementation
 * @details Corrects discontinuities at X-card boundaries using linear interpolation resampling
 *          Handles gaps between detector modules/X-cards that cause pixel misalignment
 * @author FXImage Development Team
 * @date 2025
 */

#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace fximage {

/**
 * @brief Structure to hold PDC correction parameters
 */
struct PDCCorrectionParams {
    int num_xcards;                     // Number of X-cards in detector
    int pixels_per_xcard;               // Pixels per X-card module
    int gap_width;                      // Gap width in pixels between X-cards
    bool enable_interpolation;          // Enable linear interpolation
    float* gap_positions;               // Positions of gaps (in pixels)
    int num_gaps;                       // Number of gaps
};

/**
 * @brief Linear interpolation between two values
 * @param v0 First value
 * @param v1 Second value
 * @param t Interpolation factor (0.0 to 1.0)
 * @return Interpolated value
 */
inline float LinearInterpolate(float v0, float v1, float t)
{
    return v0 + t * (v1 - v0);
}

/**
 * @brief Bilinear interpolation for 2D data
 * @param data Input data array
 * @param width Image width
 * @param height Image height
 * @param x X-coordinate (can be fractional)
 * @param y Y-coordinate (can be fractional)
 * @return Interpolated value
 */
float BilinearInterpolate(const unsigned short* data,
                          int width,
                          int height,
                          float x,
                          float y)
{
    // Clamp coordinates
    if (x < 0) x = 0;
    if (x >= width - 1) x = width - 1.001f;
    if (y < 0) y = 0;
    if (y >= height - 1) y = height - 1.001f;

    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = x - x0;
    float fy = y - y0;

    // Get four corner values
    float v00 = static_cast<float>(data[y0 * width + x0]);
    float v10 = static_cast<float>(data[y0 * width + x1]);
    float v01 = static_cast<float>(data[y1 * width + x0]);
    float v11 = static_cast<float>(data[y1 * width + x1]);

    // Interpolate in x direction
    float v0 = LinearInterpolate(v00, v10, fx);
    float v1 = LinearInterpolate(v01, v11, fx);

    // Interpolate in y direction
    return LinearInterpolate(v0, v1, fy);
}

/**
 * @brief Detect gap positions automatically from image data
 * @param data Input image data
 * @param width Image width
 * @param height Image height
 * @param gap_positions Output array for gap positions
 * @param max_gaps Maximum number of gaps to detect
 * @return Number of gaps detected
 */
int DetectGapPositions(const unsigned short* data,
                       int width,
                       int height,
                       float* gap_positions,
                       int max_gaps)
{
    if (!data || !gap_positions || width <= 0 || height <= 0) {
        return 0;
    }

    std::vector<float> column_variance(width, 0.0f);

    // Calculate variance for each column
    for (int x = 0; x < width; ++x) {
        float mean = 0.0f;
        for (int y = 0; y < height; ++y) {
            mean += data[y * width + x];
        }
        mean /= height;

        float variance = 0.0f;
        for (int y = 0; y < height; ++y) {
            float diff = data[y * width + x] - mean;
            variance += diff * diff;
        }
        column_variance[x] = variance / height;
    }

    // Smooth variance to reduce noise
    std::vector<float> smoothed(width);
    for (int x = 2; x < width - 2; ++x) {
        smoothed[x] = (column_variance[x-2] + column_variance[x-1] + 
                      column_variance[x] + column_variance[x+1] + 
                      column_variance[x+2]) / 5.0f;
    }

    // Find local minima as gap positions
    int gap_count = 0;
    float threshold = 0.5f; // Relative threshold

    for (int x = 50; x < width - 50; ++x) {
        if (gap_count >= max_gaps) break;

        if (smoothed[x] < threshold * smoothed[x-1] && 
            smoothed[x] < threshold * smoothed[x+1]) {
            gap_positions[gap_count++] = static_cast<float>(x);
        }
    }

    return gap_count;
}

/**
 * @brief Apply PDC correction using linear interpolation resampling
 * @param input_data Input image data with discontinuities
 * @param output_data Output corrected image data
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param params PDC correction parameters
 * @return true on success, false on failure
 */
bool ApplyPDCCorrection(const unsigned short* input_data,
                       unsigned short* output_data,
                       int width,
                       int height,
                       const PDCCorrectionParams& params)
{
    if (!input_data || !output_data || width <= 0 || height <= 0) {
        return false;
    }

    if (params.num_gaps == 0 || !params.gap_positions) {
        // No gaps, just copy data
        std::memcpy(output_data, input_data, width * height * sizeof(unsigned short));
        return true;
    }

    // Calculate total gap width
    int total_gap_width = params.num_gaps * params.gap_width;
    int corrected_width = width - total_gap_width;

    if (corrected_width <= 0) {
        return false;
    }

    // Build mapping from output coordinates to input coordinates
    std::vector<float> x_mapping(corrected_width);
    
    int output_x = 0;
    float accumulated_gap = 0.0f;

    for (int x = 0; x < width && output_x < corrected_width; ++x) {
        // Check if current position is in a gap
        bool in_gap = false;
        for (int g = 0; g < params.num_gaps; ++g) {
            if (x >= params.gap_positions[g] && 
                x < params.gap_positions[g] + params.gap_width) {
                in_gap = true;
                break;
            }
        }

        if (!in_gap) {
            x_mapping[output_x] = static_cast<float>(x);
            output_x++;
        }
    }

    // Apply correction with interpolation
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < corrected_width; ++x) {
            if (params.enable_interpolation) {
                // Use bilinear interpolation
                float value = BilinearInterpolate(input_data, width, height,
                                                 x_mapping[x], static_cast<float>(y));
                output_data[y * corrected_width + x] = 
                    static_cast<unsigned short>(value + 0.5f);
            } else {
                // Nearest neighbor
                int src_x = static_cast<int>(x_mapping[x] + 0.5f);
                output_data[y * corrected_width + x] = 
                    input_data[y * width + src_x];
            }
        }
    }

    return true;
}

/**
 * @brief Apply PDC correction for standard detector configuration
 * @param input_data Input image data
 * @param output_data Output corrected image data
 * @param width Image width
 * @param height Image height
 * @param num_xcards Number of X-card modules
 * @param pixels_per_xcard Pixels per X-card
 * @param gap_pixels Gap width in pixels
 * @return true on success, false on failure
 */
bool ApplyStandardPDCCorrection(const unsigned short* input_data,
                               unsigned short* output_data,
                               int width,
                               int height,
                               int num_xcards,
                               int pixels_per_xcard,
                               int gap_pixels)
{
    if (!input_data || !output_data || width <= 0 || height <= 0) {
        return false;
    }

    if (num_xcards <= 1) {
        // Single X-card, no correction needed
        std::memcpy(output_data, input_data, width * height * sizeof(unsigned short));
        return true;
    }

    // Calculate gap positions
    std::vector<float> gap_positions(num_xcards - 1);
    for (int i = 0; i < num_xcards - 1; ++i) {
        gap_positions[i] = static_cast<float>((i + 1) * pixels_per_xcard + i * gap_pixels);
    }

    PDCCorrectionParams params;
    params.num_xcards = num_xcards;
    params.pixels_per_xcard = pixels_per_xcard;
    params.gap_width = gap_pixels;
    params.enable_interpolation = true;
    params.gap_positions = gap_positions.data();
    params.num_gaps = num_xcards - 1;

    return ApplyPDCCorrection(input_data, output_data, width, height, params);
}

/**
 * @brief Fill gaps with interpolated values instead of removing them
 * @param data Input/output image data
 * @param width Image width
 * @param height Image height
 * @param gap_positions Array of gap center positions
 * @param gap_widths Array of gap widths
 * @param num_gaps Number of gaps
 * @return true on success, false on failure
 */
bool FillGapsWithInterpolation(unsigned short* data,
                               int width,
                               int height,
                               const int* gap_positions,
                               const int* gap_widths,
                               int num_gaps)
{
    if (!data || !gap_positions || !gap_widths || width <= 0 || height <= 0) {
        return false;
    }

    for (int y = 0; y < height; ++y) {
        for (int g = 0; g < num_gaps; ++g) {
            int gap_center = gap_positions[g];
            int gap_width = gap_widths[g];
            int gap_start = gap_center - gap_width / 2;
            int gap_end = gap_center + gap_width / 2;

            if (gap_start < 0 || gap_end >= width) continue;

            // Get left and right boundary values
            unsigned short left_value = data[y * width + gap_start - 1];
            unsigned short right_value = data[y * width + gap_end + 1];

            // Linear interpolation across the gap
            for (int x = gap_start; x <= gap_end; ++x) {
                float t = static_cast<float>(x - gap_start) / (gap_end - gap_start + 1);
                float interpolated = LinearInterpolate(
                    static_cast<float>(left_value),
                    static_cast<float>(right_value),
                    t
                );
                data[y * width + x] = static_cast<unsigned short>(interpolated + 0.5f);
            }
        }
    }

    return true;
}

/**
 * @brief Calculate PDC correction quality metrics
 * @param original_data Original uncorrected data
 * @param corrected_data PDC corrected data
 * @param width Image width
 * @param height Image height
 * @param gap_positions Gap positions
 * @param num_gaps Number of gaps
 * @return Quality metric (0.0 = poor, 1.0 = excellent)
 */
float CalculatePDCQuality(const unsigned short* original_data,
                         const unsigned short* corrected_data,
                         int width,
                         int height,
                         const float* gap_positions,
                         int num_gaps)
{
    if (!original_data || !corrected_data || !gap_positions) {
        return 0.0f;
    }

    // Calculate variance reduction around gap regions
    float original_variance = 0.0f;
    float corrected_variance = 0.0f;
    int sample_count = 0;

    for (int g = 0; g < num_gaps; ++g) {
        int gap_x = static_cast<int>(gap_positions[g]);
        int sample_width = 20; // Sample 20 pixels around gap

        for (int y = 0; y < height; y += 10) {
            for (int x = gap_x - sample_width; x < gap_x + sample_width; ++x) {
                if (x < 0 || x >= width) continue;

                int idx = y * width + x;
                int prev_idx = y * width + std::max(0, x - 1);

                // Calculate gradient
                int orig_grad = std::abs(original_data[idx] - original_data[prev_idx]);
                int corr_grad = std::abs(corrected_data[idx] - corrected_data[prev_idx]);

                original_variance += orig_grad * orig_grad;
                corrected_variance += corr_grad * corr_grad;
                sample_count++;
            }
        }
    }

    if (sample_count == 0 || original_variance == 0.0f) {
        return 0.0f;
    }

    // Quality = variance reduction ratio
    float quality = 1.0f - (corrected_variance / original_variance);
    return std::max(0.0f, std::min(1.0f, quality));
}

} // namespace fximage