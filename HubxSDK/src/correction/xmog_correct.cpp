/**
 * @file xmog_correct.cpp
 * @brief Multi-detector Offset and Gain (MOG) correction implementation
 * @details Provides synchronized correction for multiple detector systems
 *          Handles multi-detector calibration, synchronization, and real-time correction
 *          Supports detector stitching and cross-detector normalization
 * @author FXImage Development Team
 * @date 2025
 */

#include "../../include/xmog_correct.h"
#include "../../include/xog_correct.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>

namespace fximage {

/**
 * @brief Structure to hold single detector correction data
 */
struct DetectorCorrectionData {
    int detector_id;
    int width;
    int height;
    int x_offset;                       // Position offset in stitched image
    int y_offset;
    unsigned short* offset_data;
    float* gain_data;
    unsigned short* baseline_data;
    bool is_active;
    float normalization_factor;         // For cross-detector normalization
};

/**
 * @brief XMOGCorrect class for multi-detector correction
 */
class XMOGCorrect {
public:
    XMOGCorrect();
    ~XMOGCorrect();

    // Initialization
    bool Initialize(int num_detectors, const int* widths, const int* heights, int bit_depth = 14);
    bool Release();
    bool IsInitialized() const { return m_initialized; }

    // Detector management
    bool SetDetectorActive(int detector_id, bool active);
    bool SetDetectorPosition(int detector_id, int x_offset, int y_offset);
    bool SetDetectorNormalization(int detector_id, float normalization_factor);
    
    int GetNumDetectors() const { return m_num_detectors; }
    bool GetDetectorInfo(int detector_id, int& width, int& height, 
                        int& x_offset, int& y_offset);

    // Calibration data management
    bool SetDetectorOffsetData(int detector_id, const unsigned short* offset_data);
    bool SetDetectorGainData(int detector_id, const float* gain_data);
    bool SetDetectorBaselineData(int detector_id, const unsigned short* baseline_data);

    const unsigned short* GetDetectorOffsetData(int detector_id) const;
    const float* GetDetectorGainData(int detector_id) const;
    const unsigned short* GetDetectorBaselineData(int detector_id) const;

    // Multi-detector calibration
    bool CalculateMultiDetectorOffset(const unsigned short*** line_data, 
                                     int num_lines);
    bool CalculateMultiDetectorGain(const unsigned short** bright_field_data,
                                   unsigned short target_value);
    bool CalculateCrossDetectorNormalization();

    // Correction operations
    bool ApplyMultiDetectorCorrection(const unsigned short** input_data,
                                     unsigned short** output_data);
    bool ApplyStitchedCorrection(const unsigned short** input_data,
                                unsigned short* stitched_output,
                                int stitched_width,
                                int stitched_height);

    // Configuration
    void SetCorrectionMode(bool enable_offset, bool enable_gain, bool enable_baseline);
    void SetTargetBaseline(unsigned short baseline) { m_target_baseline = baseline; }
    void SetStitchingMode(bool enable) { m_enable_stitching = enable; }
    void SetOverlapBlending(bool enable, int overlap_width = 0);

    // File I/O
    bool SaveMultiDetectorCalibration(const char* filename);
    bool LoadMultiDetectorCalibration(const char* filename);

    // Validation and statistics
    bool ValidateMultiDetectorData();
    bool GetDetectorStatistics(int detector_id, float& offset_mean, float& gain_mean,
                              float& offset_std, float& gain_std);
    bool GetCrossDetectorUniformity(float& uniformity_metric);

private:
    bool m_initialized;
    int m_num_detectors;
    int m_bit_depth;
    unsigned short m_max_value;

    std::vector<DetectorCorrectionData> m_detectors;

    // Correction flags
    bool m_enable_offset;
    bool m_enable_gain;
    bool m_enable_baseline;
    unsigned short m_target_baseline;

    // Stitching parameters
    bool m_enable_stitching;
    bool m_enable_overlap_blending;
    int m_overlap_width;

    // Helper methods
    bool AllocateDetectorMemory(int detector_id);
    void FreeDetectorMemory(int detector_id);
    void FreeAllMemory();
    float CalculateBlendWeight(int position, int overlap_start, int overlap_end);
    bool ValidateDetectorId(int detector_id) const;
};

// Constructor
XMOGCorrect::XMOGCorrect()
    : m_initialized(false)
    , m_num_detectors(0)
    , m_bit_depth(14)
    , m_max_value(16383)
    , m_enable_offset(true)
    , m_enable_gain(true)
    , m_enable_baseline(false)
    , m_target_baseline(0)
    , m_enable_stitching(false)
    , m_enable_overlap_blending(false)
    , m_overlap_width(0)
{
}

// Destructor
XMOGCorrect::~XMOGCorrect()
{
    Release();
}

// Initialize multi-detector correction
bool XMOGCorrect::Initialize(int num_detectors, const int* widths, 
                            const int* heights, int bit_depth)
{
    if (num_detectors <= 0 || num_detectors > 16 || !widths || !heights) {
        return false;
    }

    if (bit_depth < 8 || bit_depth > 16) {
        return false;
    }

    if (m_initialized) {
        Release();
    }

    m_num_detectors = num_detectors;
    m_bit_depth = bit_depth;
    m_max_value = (1 << bit_depth) - 1;

    m_detectors.resize(num_detectors);

    // Initialize each detector
    for (int i = 0; i < num_detectors; ++i) {
        if (widths[i] <= 0 || heights[i] <= 0) {
            Release();
            return false;
        }

        m_detectors[i].detector_id = i;
        m_detectors[i].width = widths[i];
        m_detectors[i].height = heights[i];
        m_detectors[i].x_offset = i * widths[i]; // Default horizontal arrangement
        m_detectors[i].y_offset = 0;
        m_detectors[i].is_active = true;
        m_detectors[i].normalization_factor = 1.0f;
        m_detectors[i].offset_data = nullptr;
        m_detectors[i].gain_data = nullptr;
        m_detectors[i].baseline_data = nullptr;

        if (!AllocateDetectorMemory(i)) {
            Release();
            return false;
        }
    }

    m_initialized = true;
    return true;
}

// Release all resources
bool XMOGCorrect::Release()
{
    FreeAllMemory();
    m_detectors.clear();
    m_initialized = false;
    m_num_detectors = 0;
    return true;
}

// Validate detector ID
bool XMOGCorrect::ValidateDetectorId(int detector_id) const
{
    return detector_id >= 0 && detector_id < m_num_detectors;
}

// Allocate memory for single detector
bool XMOGCorrect::AllocateDetectorMemory(int detector_id)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    DetectorCorrectionData& det = m_detectors[detector_id];
    const size_t total_pixels = det.width * det.height;

    try {
        det.offset_data = new unsigned short[total_pixels];
        det.gain_data = new float[total_pixels];
        det.baseline_data = new unsigned short[total_pixels];

        std::fill_n(det.offset_data, total_pixels, 0);
        std::fill_n(det.gain_data, total_pixels, 1.0f);
        std::fill_n(det.baseline_data, total_pixels, 0);

        return true;
    } catch (const std::bad_alloc&) {
        FreeDetectorMemory(detector_id);
        return false;
    }
}

// Free memory for single detector
void XMOGCorrect::FreeDetectorMemory(int detector_id)
{
    if (!ValidateDetectorId(detector_id)) {
        return;
    }

    DetectorCorrectionData& det = m_detectors[detector_id];

    if (det.offset_data) {
        delete[] det.offset_data;
        det.offset_data = nullptr;
    }
    if (det.gain_data) {
        delete[] det.gain_data;
        det.gain_data = nullptr;
    }
    if (det.baseline_data) {
        delete[] det.baseline_data;
        det.baseline_data = nullptr;
    }
}

// Free all detector memory
void XMOGCorrect::FreeAllMemory()
{
    for (int i = 0; i < m_num_detectors; ++i) {
        FreeDetectorMemory(i);
    }
}

// Set detector active state
bool XMOGCorrect::SetDetectorActive(int detector_id, bool active)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    m_detectors[detector_id].is_active = active;
    return true;
}

// Set detector position in stitched image
bool XMOGCorrect::SetDetectorPosition(int detector_id, int x_offset, int y_offset)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    m_detectors[detector_id].x_offset = x_offset;
    m_detectors[detector_id].y_offset = y_offset;
    return true;
}

// Set detector normalization factor
bool XMOGCorrect::SetDetectorNormalization(int detector_id, float normalization_factor)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    if (normalization_factor <= 0.0f || normalization_factor > 10.0f) {
        return false;
    }

    m_detectors[detector_id].normalization_factor = normalization_factor;
    return true;
}

// Get detector information
bool XMOGCorrect::GetDetectorInfo(int detector_id, int& width, int& height,
                                 int& x_offset, int& y_offset)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    const DetectorCorrectionData& det = m_detectors[detector_id];
    width = det.width;
    height = det.height;
    x_offset = det.x_offset;
    y_offset = det.y_offset;
    return true;
}

// Set detector offset data
bool XMOGCorrect::SetDetectorOffsetData(int detector_id, const unsigned short* offset_data)
{
    if (!ValidateDetectorId(detector_id) || !offset_data) {
        return false;
    }

    DetectorCorrectionData& det = m_detectors[detector_id];
    std::memcpy(det.offset_data, offset_data, 
               det.width * det.height * sizeof(unsigned short));
    return true;
}

// Set detector gain data
bool XMOGCorrect::SetDetectorGainData(int detector_id, const float* gain_data)
{
    if (!ValidateDetectorId(detector_id) || !gain_data) {
        return false;
    }

    DetectorCorrectionData& det = m_detectors[detector_id];
    std::memcpy(det.gain_data, gain_data, 
               det.width * det.height * sizeof(float));
    return true;
}

// Set detector baseline data
bool XMOGCorrect::SetDetectorBaselineData(int detector_id, const unsigned short* baseline_data)
{
    if (!ValidateDetectorId(detector_id) || !baseline_data) {
        return false;
    }

    DetectorCorrectionData& det = m_detectors[detector_id];
    std::memcpy(det.baseline_data, baseline_data, 
               det.width * det.height * sizeof(unsigned short));
    return true;
}

// Get detector offset data
const unsigned short* XMOGCorrect::GetDetectorOffsetData(int detector_id) const
{
    if (!ValidateDetectorId(detector_id)) {
        return nullptr;
    }
    return m_detectors[detector_id].offset_data;
}

// Get detector gain data
const float* XMOGCorrect::GetDetectorGainData(int detector_id) const
{
    if (!ValidateDetectorId(detector_id)) {
        return nullptr;
    }
    return m_detectors[detector_id].gain_data;
}

// Get detector baseline data
const unsigned short* XMOGCorrect::GetDetectorBaselineData(int detector_id) const
{
    if (!ValidateDetectorId(detector_id)) {
        return nullptr;
    }
    return m_detectors[detector_id].baseline_data;
}

// Calculate offset for all detectors from synchronized dark frames
bool XMOGCorrect::CalculateMultiDetectorOffset(const unsigned short*** line_data,
                                              int num_lines)
{
    if (!m_initialized || !line_data || num_lines <= 0) {
        return false;
    }

    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            continue;
        }

        DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        std::vector<unsigned long long> accumulator(total_pixels, 0);

        // Accumulate line data for this detector
        for (int line = 0; line < num_lines; ++line) {
            if (!line_data[det_id] || !line_data[det_id][line]) {
                return false;
            }

            for (int i = 0; i < total_pixels; ++i) {
                accumulator[i] += line_data[det_id][line][i];
            }
        }

        // Calculate average
        for (int i = 0; i < total_pixels; ++i) {
            det.offset_data[i] = static_cast<unsigned short>(
                (accumulator[i] + num_lines / 2) / num_lines
            );
        }
    }

    return true;
}

// Calculate gain for all detectors from synchronized bright field
bool XMOGCorrect::CalculateMultiDetectorGain(const unsigned short** bright_field_data,
                                            unsigned short target_value)
{
    if (!m_initialized || !bright_field_data || target_value == 0) {
        return false;
    }

    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            continue;
        }

        if (!bright_field_data[det_id]) {
            return false;
        }

        DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        for (int i = 0; i < total_pixels; ++i) {
            int corrected = static_cast<int>(bright_field_data[det_id][i]) -
                           static_cast<int>(det.offset_data[i]);

            if (corrected > 0) {
                det.gain_data[i] = static_cast<float>(target_value) /
                                  static_cast<float>(corrected);
            } else {
                det.gain_data[i] = 1.0f;
            }

            // Clamp gain
            if (det.gain_data[i] < 0.1f) det.gain_data[i] = 0.1f;
            if (det.gain_data[i] > 10.0f) det.gain_data[i] = 10.0f;
        }
    }

    return true;
}

// Calculate cross-detector normalization factors
bool XMOGCorrect::CalculateCrossDetectorNormalization()
{
    if (!m_initialized) {
        return false;
    }

    // Calculate mean gain for each detector
    std::vector<float> detector_means(m_num_detectors);

    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            detector_means[det_id] = 1.0f;
            continue;
        }

        DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        double sum = 0.0;
        for (int i = 0; i < total_pixels; ++i) {
            sum += det.gain_data[i];
        }
        detector_means[det_id] = static_cast<float>(sum / total_pixels);
    }

    // Calculate global mean
    float global_mean = 0.0f;
    int active_count = 0;
    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (m_detectors[det_id].is_active) {
            global_mean += detector_means[det_id];
            active_count++;
        }
    }

    if (active_count == 0) {
        return false;
    }

    global_mean /= active_count;

    // Set normalization factors
    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (m_detectors[det_id].is_active && detector_means[det_id] > 0.0f) {
            m_detectors[det_id].normalization_factor = global_mean / detector_means[det_id];
        }
    }

    return true;
}

// Apply correction to multiple detectors independently
bool XMOGCorrect::ApplyMultiDetectorCorrection(const unsigned short** input_data,
                                              unsigned short** output_data)
{
    if (!m_initialized || !input_data || !output_data) {
        return false;
    }

    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            continue;
        }

        if (!input_data[det_id] || !output_data[det_id]) {
            return false;
        }

        DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        for (int i = 0; i < total_pixels; ++i) {
            float corrected = static_cast<float>(input_data[det_id][i]);

            // Apply offset correction
            if (m_enable_offset) {
                corrected -= static_cast<float>(det.offset_data[i]);
            }

            // Apply gain correction
            if (m_enable_gain) {
                corrected *= det.gain_data[i];
            }

            // Apply cross-detector normalization
            corrected *= det.normalization_factor;

            // Apply baseline correction
            if (m_enable_baseline) {
                corrected -= static_cast<float>(det.baseline_data[i]);
            }

            // Add target baseline
            corrected += static_cast<float>(m_target_baseline);

            // Clamp
            if (corrected < 0.0f) {
                output_data[det_id][i] = 0;
            } else if (corrected > m_max_value) {
                output_data[det_id][i] = m_max_value;
            } else {
                output_data[det_id][i] = static_cast<unsigned short>(corrected + 0.5f);
            }
        }
    }

    return true;
}

// Calculate blend weight for overlap regions
float XMOGCorrect::CalculateBlendWeight(int position, int overlap_start, int overlap_end)
{
    if (position < overlap_start) {
        return 1.0f;
    } else if (position > overlap_end) {
        return 0.0f;
    } else {
        // Linear blend
        float t = static_cast<float>(position - overlap_start) / 
                 static_cast<float>(overlap_end - overlap_start);
        return 1.0f - t;
    }
}

// Apply correction and stitch detectors into single image
bool XMOGCorrect::ApplyStitchedCorrection(const unsigned short** input_data,
                                         unsigned short* stitched_output,
                                         int stitched_width,
                                         int stitched_height)
{
    if (!m_initialized || !input_data || !stitched_output) {
        return false;
    }

    if (stitched_width <= 0 || stitched_height <= 0) {
        return false;
    }

    // Initialize output with zeros
    std::fill_n(stitched_output, stitched_width * stitched_height, 0);

    // Process each detector
    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active || !input_data[det_id]) {
            continue;
        }

        DetectorCorrectionData& det = m_detectors[det_id];

        // Check if detector overlaps with next detector
        bool has_overlap = false;
        int overlap_start_x = 0;
        int overlap_width = 0;

        if (m_enable_overlap_blending && det_id < m_num_detectors - 1) {
            int next_det_x = m_detectors[det_id + 1].x_offset;
            int this_det_end = det.x_offset + det.width;
            
            if (next_det_x < this_det_end) {
                has_overlap = true;
                overlap_start_x = next_det_x;
                overlap_width = this_det_end - next_det_x;
            }
        }

        // Process each pixel in detector
        for (int y = 0; y < det.height; ++y) {
            int out_y = det.y_offset + y;
            if (out_y < 0 || out_y >= stitched_height) {
                continue;
            }

            for (int x = 0; x < det.width; ++x) {
                int out_x = det.x_offset + x;
                if (out_x < 0 || out_x >= stitched_width) {
                    continue;
                }

                int in_idx = y * det.width + x;
                int out_idx = out_y * stitched_width + out_x;

                // Apply correction
                float corrected = static_cast<float>(input_data[det_id][in_idx]);

                if (m_enable_offset) {
                    corrected -= static_cast<float>(det.offset_data[in_idx]);
                }

                if (m_enable_gain) {
                    corrected *= det.gain_data[in_idx];
                }

                corrected *= det.normalization_factor;

                if (m_enable_baseline) {
                    corrected -= static_cast<float>(det.baseline_data[in_idx]);
                }

                corrected += static_cast<float>(m_target_baseline);

                // Apply blending if in overlap region
                if (has_overlap && out_x >= overlap_start_x && 
                    out_x < overlap_start_x + overlap_width) {
                    float weight = CalculateBlendWeight(out_x, overlap_start_x,
                                                       overlap_start_x + overlap_width);
                    
                    // Blend with existing value
                    float existing = static_cast<float>(stitched_output[out_idx]);
                    corrected = corrected * weight + existing * (1.0f - weight);
                }

                // Clamp and store
                if (corrected < 0.0f) {
                    stitched_output[out_idx] = 0;
                } else if (corrected > m_max_value) {
                    stitched_output[out_idx] = m_max_value;
                } else {
                    stitched_output[out_idx] = static_cast<unsigned short>(corrected + 0.5f);
                }
            }
        }
    }

    return true;
}

// Set correction mode
void XMOGCorrect::SetCorrectionMode(bool enable_offset, bool enable_gain, bool enable_baseline)
{
    m_enable_offset = enable_offset;
    m_enable_gain = enable_gain;
    m_enable_baseline = enable_baseline;
}

// Set overlap blending
void XMOGCorrect::SetOverlapBlending(bool enable, int overlap_width)
{
    m_enable_overlap_blending = enable;
    if (overlap_width >= 0) {
        m_overlap_width = overlap_width;
    }
}

// Save multi-detector calibration to file
bool XMOGCorrect::SaveMultiDetectorCalibration(const char* filename)
{
    if (!m_initialized || !filename) {
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Write header
    file.write(reinterpret_cast<const char*>(&m_num_detectors), sizeof(int));
    file.write(reinterpret_cast<const char*>(&m_bit_depth), sizeof(int));

    // Write each detector's data
    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        const DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        file.write(reinterpret_cast<const char*>(&det.detector_id), sizeof(int));
        file.write(reinterpret_cast<const char*>(&det.width), sizeof(int));
        file.write(reinterpret_cast<const char*>(&det.height), sizeof(int));
        file.write(reinterpret_cast<const char*>(&det.x_offset), sizeof(int));
        file.write(reinterpret_cast<const char*>(&det.y_offset), sizeof(int));
        file.write(reinterpret_cast<const char*>(&det.is_active), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&det.normalization_factor), sizeof(float));

        file.write(reinterpret_cast<const char*>(det.offset_data),
                  total_pixels * sizeof(unsigned short));
        file.write(reinterpret_cast<const char*>(det.gain_data),
                  total_pixels * sizeof(float));
        file.write(reinterpret_cast<const char*>(det.baseline_data),
                  total_pixels * sizeof(unsigned short));
    }

    file.close();
    return true;
}

// Load multi-detector calibration from file
bool XMOGCorrect::LoadMultiDetectorCalibration(const char* filename)
{
    if (!filename) {
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    int num_detectors, bit_depth;
    file.read(reinterpret_cast<char*>(&num_detectors), sizeof(int));
    file.read(reinterpret_cast<char*>(&bit_depth), sizeof(int));

    // Read detector dimensions
    std::vector<int> widths(num_detectors);
    std::vector<int> heights(num_detectors);

    // First pass: read dimensions
    long pos = file.tellg();
    for (int i = 0; i < num_detectors; ++i) {
        int id;
        file.read(reinterpret_cast<char*>(&id), sizeof(int));
        file.read(reinterpret_cast<char*>(&widths[i]), sizeof(int));
        file.read(reinterpret_cast<char*>(&heights[i]), sizeof(int));
        
        // Skip rest of detector data
        file.seekg(sizeof(int) * 2 + sizeof(bool) + sizeof(float), std::ios::cur);
        int total_pixels = widths[i] * heights[i];
        file.seekg(total_pixels * (sizeof(unsigned short) * 2 + sizeof(float)), std::ios::cur);
    }

    // Reset file position
    file.seekg(pos);

    // Initialize with loaded dimensions
    if (!Initialize(num_detectors, widths.data(), heights.data(), bit_depth)) {
        file.close();
        return false;
    }

    // Second pass: read all data
    for (int det_id = 0; det_id < num_detectors; ++det_id) {
        DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        file.read(reinterpret_cast<char*>(&det.detector_id), sizeof(int));
        file.read(reinterpret_cast<char*>(&det.width), sizeof(int));
        file.read(reinterpret_cast<char*>(&det.height), sizeof(int));
        file.read(reinterpret_cast<char*>(&det.x_offset), sizeof(int));
        file.read(reinterpret_cast<char*>(&det.y_offset), sizeof(int));
        file.read(reinterpret_cast<char*>(&det.is_active), sizeof(bool));
        file.read(reinterpret_cast<char*>(&det.normalization_factor), sizeof(float));

        file.read(reinterpret_cast<char*>(det.offset_data),
                 total_pixels * sizeof(unsigned short));
        file.read(reinterpret_cast<char*>(det.gain_data),
                 total_pixels * sizeof(float));
        file.read(reinterpret_cast<char*>(det.baseline_data),
                 total_pixels * sizeof(unsigned short));
    }

    file.close();
    return file.good();
}

// Validate multi-detector calibration data
bool XMOGCorrect::ValidateMultiDetectorData()
{
    if (!m_initialized) {
        return false;
    }

    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            continue;
        }

        const DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;
        int invalid_count = 0;

        for (int i = 0; i < total_pixels; ++i) {
            if (std::isnan(det.gain_data[i]) || std::isinf(det.gain_data[i]) ||
                det.gain_data[i] <= 0.0f || det.gain_data[i] > 100.0f) {
                invalid_count++;
            }
        }

        // Allow up to 0.1% invalid pixels per detector
        if (invalid_count >= (total_pixels / 1000)) {
            return false;
        }
    }

    return true;
}

// Get statistics for specific detector
bool XMOGCorrect::GetDetectorStatistics(int detector_id, float& offset_mean, float& gain_mean,
                                       float& offset_std, float& gain_std)
{
    if (!ValidateDetectorId(detector_id)) {
        return false;
    }

    const DetectorCorrectionData& det = m_detectors[detector_id];
    const int total_pixels = det.width * det.height;

    // Calculate means
    offset_mean = 0.0f;
    gain_mean = 0.0f;

    for (int i = 0; i < total_pixels; ++i) {
        offset_mean += static_cast<float>(det.offset_data[i]);
        gain_mean += det.gain_data[i];
    }
    offset_mean /= total_pixels;
    gain_mean /= total_pixels;

    // Calculate standard deviations
    offset_std = 0.0f;
    gain_std = 0.0f;

    for (int i = 0; i < total_pixels; ++i) {
        float offset_diff = static_cast<float>(det.offset_data[i]) - offset_mean;
        float gain_diff = det.gain_data[i] - gain_mean;
        offset_std += offset_diff * offset_diff;
        gain_std += gain_diff * gain_diff;
    }
    offset_std = std::sqrt(offset_std / total_pixels);
    gain_std = std::sqrt(gain_std / total_pixels);

    return true;
}

// Calculate cross-detector uniformity metric
bool XMOGCorrect::GetCrossDetectorUniformity(float& uniformity_metric)
{
    if (!m_initialized) {
        return false;
    }

    std::vector<float> detector_means(m_num_detectors);
    int active_count = 0;

    // Calculate mean for each active detector
    for (int det_id = 0; det_id < m_num_detectors; ++det_id) {
        if (!m_detectors[det_id].is_active) {
            continue;
        }

        const DetectorCorrectionData& det = m_detectors[det_id];
        const int total_pixels = det.width * det.height;

        double sum = 0.0;
        for (int i = 0; i < total_pixels; ++i) {
            sum += det.gain_data[i];
        }
        detector_means[active_count++] = static_cast<float>(sum / total_pixels);
    }

    if (active_count < 2) {
        uniformity_metric = 1.0f;
        return true;
    }

    // Calculate global mean
    float global_mean = 0.0f;
    for (int i = 0; i < active_count; ++i) {
        global_mean += detector_means[i];
    }
    global_mean /= active_count;

    // Calculate coefficient of variation
    float variance = 0.0f;
    for (int i = 0; i < active_count; ++i) {
        float diff = detector_means[i] - global_mean;
        variance += diff * diff;
    }
    float std_dev = std::sqrt(variance / active_count);

    // Uniformity metric: 1.0 is perfect, lower is worse
    uniformity_metric = 1.0f - (std_dev / global_mean);
    if (uniformity_metric < 0.0f) uniformity_metric = 0.0f;

    return true;
}

// Global instance management
static XMOGCorrect* g_xmog_instance = nullptr;

XMOGCorrect* CreateXMOGCorrect()
{
    if (!g_xmog_instance) {
        g_xmog_instance = new XMOGCorrect();
    }
    return g_xmog_instance;
}

void DestroyXMOGCorrect()
{
    if (g_xmog_instance) {
        delete g_xmog_instance;
        g_xmog_instance = nullptr;
    }
}

XMOGCorrect* GetXMOGCorrect()
{
    return g_xmog_instance;
}

} // namespace fximage