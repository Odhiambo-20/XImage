/**
 * @file xog_correct.cpp
 * @brief Single-detector Offset and Gain (OG) correction implementation
 * @details Provides optimized single-gain correction for single detector systems
 *          Implements complete calibration workflow: offset calculation, gain calculation,
 *          and real-time correction with baseline adjustment
 * @author FXImage Development Team
 * @date 2025
 */

#include "../../include/xog_correct.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>

namespace fximage {

/**
 * @brief XOGCorrect class implementation for single-detector correction
 */
class XOGCorrect {
public:
    XOGCorrect();
    ~XOGCorrect();

    // Initialization and configuration
    bool Initialize(int width, int height, int bit_depth = 14);
    bool Release();
    bool IsInitialized() const { return m_initialized; }

    // Calibration data management
    bool SetOffsetData(const unsigned short* offset_data);
    bool SetGainData(const float* gain_data);
    bool SetBaselineData(const unsigned short* baseline_data);
    
    const unsigned short* GetOffsetData() const { return m_offset_data; }
    const float* GetGainData() const { return m_gain_data; }
    const unsigned short* GetBaselineData() const { return m_baseline_data; }

    // Calibration calculation
    bool CalculateOffset(const unsigned short** line_data, int num_lines);
    bool CalculateGain(const unsigned short* bright_field_data, 
                       unsigned short target_value);
    bool CalculateBaseline(const unsigned short** line_data, int num_lines);

    // Correction operations
    bool ApplyCorrection(const unsigned short* input_data,
                        unsigned short* output_data);
    bool ApplyCorrectionLine(const unsigned short* input_line,
                            unsigned short* output_line,
                            int line_index = 0);

    // Configuration
    void SetCorrectionMode(bool enable_offset, bool enable_gain, bool enable_baseline);
    void SetTargetBaseline(unsigned short baseline) { m_target_baseline = baseline; }
    void SetBitDepth(int bit_depth);

    // File I/O
    bool SaveCalibrationData(const char* filename);
    bool LoadCalibrationData(const char* filename);

    // Statistics and validation
    bool GetOffsetStatistics(float& mean, float& std_dev, float& min_val, float& max_val);
    bool GetGainStatistics(float& mean, float& std_dev, float& min_val, float& max_val);
    bool ValidateCalibrationData();

private:
    bool m_initialized;
    int m_width;
    int m_height;
    int m_bit_depth;
    unsigned short m_max_value;

    // Calibration data
    unsigned short* m_offset_data;
    float* m_gain_data;
    unsigned short* m_baseline_data;

    // Correction flags
    bool m_enable_offset;
    bool m_enable_gain;
    bool m_enable_baseline;
    unsigned short m_target_baseline;

    // Helper methods
    void ClampValue(float& value);
    bool AllocateMemory();
    void FreeMemory();
};

// Constructor
XOGCorrect::XOGCorrect()
    : m_initialized(false)
    , m_width(0)
    , m_height(0)
    , m_bit_depth(14)
    , m_max_value(16383)
    , m_offset_data(nullptr)
    , m_gain_data(nullptr)
    , m_baseline_data(nullptr)
    , m_enable_offset(true)
    , m_enable_gain(true)
    , m_enable_baseline(false)
    , m_target_baseline(0)
{
}

// Destructor
XOGCorrect::~XOGCorrect()
{
    Release();
}

// Initialize correction object
bool XOGCorrect::Initialize(int width, int height, int bit_depth)
{
    if (width <= 0 || height <= 0 || bit_depth < 8 || bit_depth > 16) {
        return false;
    }

    if (m_initialized) {
        Release();
    }

    m_width = width;
    m_height = height;
    m_bit_depth = bit_depth;
    m_max_value = (1 << bit_depth) - 1;

    if (!AllocateMemory()) {
        Release();
        return false;
    }

    m_initialized = true;
    return true;
}

// Release resources
bool XOGCorrect::Release()
{
    FreeMemory();
    m_initialized = false;
    m_width = 0;
    m_height = 0;
    return true;
}

// Allocate memory for calibration data
bool XOGCorrect::AllocateMemory()
{
    const size_t total_pixels = m_width * m_height;

    try {
        m_offset_data = new unsigned short[total_pixels];
        m_gain_data = new float[total_pixels];
        m_baseline_data = new unsigned short[total_pixels];

        // Initialize with default values
        std::fill_n(m_offset_data, total_pixels, 0);
        std::fill_n(m_gain_data, total_pixels, 1.0f);
        std::fill_n(m_baseline_data, total_pixels, 0);

        return true;
    } catch (const std::bad_alloc&) {
        FreeMemory();
        return false;
    }
}

// Free allocated memory
void XOGCorrect::FreeMemory()
{
    if (m_offset_data) {
        delete[] m_offset_data;
        m_offset_data = nullptr;
    }
    if (m_gain_data) {
        delete[] m_gain_data;
        m_gain_data = nullptr;
    }
    if (m_baseline_data) {
        delete[] m_baseline_data;
        m_baseline_data = nullptr;
    }
}

// Set offset data
bool XOGCorrect::SetOffsetData(const unsigned short* offset_data)
{
    if (!m_initialized || !offset_data || !m_offset_data) {
        return false;
    }

    std::memcpy(m_offset_data, offset_data, m_width * m_height * sizeof(unsigned short));
    return true;
}

// Set gain data
bool XOGCorrect::SetGainData(const float* gain_data)
{
    if (!m_initialized || !gain_data || !m_gain_data) {
        return false;
    }

    std::memcpy(m_gain_data, gain_data, m_width * m_height * sizeof(float));
    return true;
}

// Set baseline data
bool XOGCorrect::SetBaselineData(const unsigned short* baseline_data)
{
    if (!m_initialized || !baseline_data || !m_baseline_data) {
        return false;
    }

    std::memcpy(m_baseline_data, baseline_data, m_width * m_height * sizeof(unsigned short));
    return true;
}

// Calculate offset from multiple dark frames (typically 4096 lines)
bool XOGCorrect::CalculateOffset(const unsigned short** line_data, int num_lines)
{
    if (!m_initialized || !line_data || num_lines <= 0) {
        return false;
    }

    const int total_pixels = m_width * m_height;

    // Accumulate line data
    std::vector<unsigned long long> accumulator(total_pixels, 0);

    for (int line = 0; line < num_lines; ++line) {
        if (!line_data[line]) {
            return false;
        }

        for (int i = 0; i < total_pixels; ++i) {
            accumulator[i] += line_data[line][i];
        }
    }

    // Calculate average and store as offset
    for (int i = 0; i < total_pixels; ++i) {
        m_offset_data[i] = static_cast<unsigned short>(
            (accumulator[i] + num_lines / 2) / num_lines
        );
    }

    return true;
}

// Calculate gain coefficients from bright field data
bool XOGCorrect::CalculateGain(const unsigned short* bright_field_data,
                              unsigned short target_value)
{
    if (!m_initialized || !bright_field_data || target_value == 0) {
        return false;
    }

    const int total_pixels = m_width * m_height;

    for (int i = 0; i < total_pixels; ++i) {
        // Subtract offset first
        int corrected = static_cast<int>(bright_field_data[i]) - 
                       static_cast<int>(m_offset_data[i]);

        if (corrected > 0) {
            m_gain_data[i] = static_cast<float>(target_value) / 
                            static_cast<float>(corrected);
        } else {
            m_gain_data[i] = 1.0f;
        }

        // Clamp gain to reasonable range
        if (m_gain_data[i] < 0.1f) m_gain_data[i] = 0.1f;
        if (m_gain_data[i] > 10.0f) m_gain_data[i] = 10.0f;
    }

    return true;
}

// Calculate baseline from multiple reference frames
bool XOGCorrect::CalculateBaseline(const unsigned short** line_data, int num_lines)
{
    if (!m_initialized || !line_data || num_lines <= 0) {
        return false;
    }

    const int total_pixels = m_width * m_height;

    // Accumulate corrected line data
    std::vector<unsigned long long> accumulator(total_pixels, 0);

    for (int line = 0; line < num_lines; ++line) {
        if (!line_data[line]) {
            return false;
        }

        for (int i = 0; i < total_pixels; ++i) {
            // Apply offset and gain correction
            int corrected = static_cast<int>(line_data[line][i]) - 
                           static_cast<int>(m_offset_data[i]);
            float gained = corrected * m_gain_data[i];
            
            if (gained < 0.0f) gained = 0.0f;
            if (gained > m_max_value) gained = m_max_value;
            
            accumulator[i] += static_cast<unsigned long long>(gained + 0.5f);
        }
    }

    // Calculate average baseline
    for (int i = 0; i < total_pixels; ++i) {
        m_baseline_data[i] = static_cast<unsigned short>(
            (accumulator[i] + num_lines / 2) / num_lines
        );
    }

    return true;
}

// Apply full correction to frame
bool XOGCorrect::ApplyCorrection(const unsigned short* input_data,
                                unsigned short* output_data)
{
    if (!m_initialized || !input_data || !output_data) {
        return false;
    }

    const int total_pixels = m_width * m_height;

    for (int i = 0; i < total_pixels; ++i) {
        float corrected = static_cast<float>(input_data[i]);

        // Step 1: Subtract offset
        if (m_enable_offset) {
            corrected -= static_cast<float>(m_offset_data[i]);
        }

        // Step 2: Apply gain
        if (m_enable_gain) {
            corrected *= m_gain_data[i];
        }

        // Step 3: Subtract baseline
        if (m_enable_baseline) {
            corrected -= static_cast<float>(m_baseline_data[i]);
        }

        // Step 4: Add target baseline
        corrected += static_cast<float>(m_target_baseline);

        // Step 5: Clamp to valid range
        ClampValue(corrected);
        output_data[i] = static_cast<unsigned short>(corrected + 0.5f);
    }

    return true;
}

// Apply correction to single line (for line-scan mode)
bool XOGCorrect::ApplyCorrectionLine(const unsigned short* input_line,
                                    unsigned short* output_line,
                                    int line_index)
{
    if (!m_initialized || !input_line || !output_line) {
        return false;
    }

    if (line_index < 0 || line_index >= m_height) {
        line_index = 0;
    }

    const int line_offset = line_index * m_width;

    for (int x = 0; x < m_width; ++x) {
        int i = line_offset + x;
        float corrected = static_cast<float>(input_line[x]);

        if (m_enable_offset) {
            corrected -= static_cast<float>(m_offset_data[i]);
        }

        if (m_enable_gain) {
            corrected *= m_gain_data[i];
        }

        if (m_enable_baseline) {
            corrected -= static_cast<float>(m_baseline_data[i]);
        }

        corrected += static_cast<float>(m_target_baseline);

        ClampValue(corrected);
        output_line[x] = static_cast<unsigned short>(corrected + 0.5f);
    }

    return true;
}

// Set correction mode flags
void XOGCorrect::SetCorrectionMode(bool enable_offset, bool enable_gain, bool enable_baseline)
{
    m_enable_offset = enable_offset;
    m_enable_gain = enable_gain;
    m_enable_baseline = enable_baseline;
}

// Set bit depth
void XOGCorrect::SetBitDepth(int bit_depth)
{
    if (bit_depth >= 8 && bit_depth <= 16) {
        m_bit_depth = bit_depth;
        m_max_value = (1 << bit_depth) - 1;
    }
}

// Clamp value to valid range
void XOGCorrect::ClampValue(float& value)
{
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > m_max_value) {
        value = static_cast<float>(m_max_value);
    }
}

// Save calibration data to file
bool XOGCorrect::SaveCalibrationData(const char* filename)
{
    if (!m_initialized || !filename) {
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    const int total_pixels = m_width * m_height;

    // Write header
    file.write(reinterpret_cast<const char*>(&m_width), sizeof(int));
    file.write(reinterpret_cast<const char*>(&m_height), sizeof(int));
    file.write(reinterpret_cast<const char*>(&m_bit_depth), sizeof(int));

    // Write offset data
    file.write(reinterpret_cast<const char*>(m_offset_data), 
              total_pixels * sizeof(unsigned short));

    // Write gain data
    file.write(reinterpret_cast<const char*>(m_gain_data), 
              total_pixels * sizeof(float));

    // Write baseline data
    file.write(reinterpret_cast<const char*>(m_baseline_data), 
              total_pixels * sizeof(unsigned short));

    file.close();
    return true;
}

// Load calibration data from file
bool XOGCorrect::LoadCalibrationData(const char* filename)
{
    if (!filename) {
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    int width, height, bit_depth;

    // Read header
    file.read(reinterpret_cast<char*>(&width), sizeof(int));
    file.read(reinterpret_cast<char*>(&height), sizeof(int));
    file.read(reinterpret_cast<char*>(&bit_depth), sizeof(int));

    // Initialize with loaded dimensions
    if (!Initialize(width, height, bit_depth)) {
        file.close();
        return false;
    }

    const int total_pixels = m_width * m_height;

    // Read offset data
    file.read(reinterpret_cast<char*>(m_offset_data), 
             total_pixels * sizeof(unsigned short));

    // Read gain data
    file.read(reinterpret_cast<char*>(m_gain_data), 
             total_pixels * sizeof(float));

    // Read baseline data
    file.read(reinterpret_cast<char*>(m_baseline_data), 
             total_pixels * sizeof(unsigned short));

    file.close();
    return file.good();
}

// Get offset statistics
bool XOGCorrect::GetOffsetStatistics(float& mean, float& std_dev, 
                                    float& min_val, float& max_val)
{
    if (!m_initialized || !m_offset_data) {
        return false;
    }

    const int total_pixels = m_width * m_height;
    
    mean = 0.0f;
    min_val = static_cast<float>(m_offset_data[0]);
    max_val = static_cast<float>(m_offset_data[0]);

    for (int i = 0; i < total_pixels; ++i) {
        float val = static_cast<float>(m_offset_data[i]);
        mean += val;
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }
    mean /= total_pixels;

    std_dev = 0.0f;
    for (int i = 0; i < total_pixels; ++i) {
        float diff = static_cast<float>(m_offset_data[i]) - mean;
        std_dev += diff * diff;
    }
    std_dev = std::sqrt(std_dev / total_pixels);

    return true;
}

// Get gain statistics
bool XOGCorrect::GetGainStatistics(float& mean, float& std_dev, 
                                  float& min_val, float& max_val)
{
    if (!m_initialized || !m_gain_data) {
        return false;
    }

    const int total_pixels = m_width * m_height;
    
    mean = 0.0f;
    min_val = m_gain_data[0];
    max_val = m_gain_data[0];

    for (int i = 0; i < total_pixels; ++i) {
        mean += m_gain_data[i];
        if (m_gain_data[i] < min_val) min_val = m_gain_data[i];
        if (m_gain_data[i] > max_val) max_val = m_gain_data[i];
    }
    mean /= total_pixels;

    std_dev = 0.0f;
    for (int i = 0; i < total_pixels; ++i) {
        float diff = m_gain_data[i] - mean;
        std_dev += diff * diff;
    }
    std_dev = std::sqrt(std_dev / total_pixels);

    return true;
}

// Validate calibration data
bool XOGCorrect::ValidateCalibrationData()
{
    if (!m_initialized) {
        return false;
    }

    const int total_pixels = m_width * m_height;
    int invalid_count = 0;

    // Check gain data
    for (int i = 0; i < total_pixels; ++i) {
        if (std::isnan(m_gain_data[i]) || std::isinf(m_gain_data[i]) ||
            m_gain_data[i] <= 0.0f || m_gain_data[i] > 100.0f) {
            invalid_count++;
        }
    }

    // Allow up to 0.1% invalid pixels
    return invalid_count < (total_pixels / 1000);
}

// Global instance management functions
static XOGCorrect* g_xog_instance = nullptr;

XOGCorrect* CreateXOGCorrect()
{
    if (!g_xog_instance) {
        g_xog_instance = new XOGCorrect();
    }
    return g_xog_instance;
}

void DestroyXOGCorrect()
{
    if (g_xog_instance) {
        delete g_xog_instance;
        g_xog_instance = nullptr;
    }
}

XOGCorrect* GetXOGCorrect()
{
    return g_xog_instance;
}

} // namespace fximage