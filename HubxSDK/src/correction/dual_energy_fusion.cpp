/**
 * @file dual_energy_fusion.cpp
 * @brief Dual-energy image fusion for X-ray imaging systems
 * @details Combines high-energy and low-energy X-ray images to enhance material discrimination
 *          and image quality. Implements weighted fusion based on bit depth.
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
#define HUBX_ERROR_DIMENSION_MISMATCH -5

namespace HubxSDK {
namespace Correction {

/**
 * @enum FusionMode
 * @brief Fusion modes for dual-energy processing
 */
enum FusionMode {
    FUSION_WEIGHTED_AVERAGE = 0,    // Simple weighted average
    FUSION_MATERIAL_DECOMPOSITION,   // Material decomposition
    FUSION_ADAPTIVE,                 // Adaptive fusion based on local statistics
    FUSION_LOGARITHMIC,              // Logarithmic fusion
    FUSION_CUSTOM                    // Custom user-defined fusion
};

/**
 * @class DualEnergyFusion
 * @brief Handles dual-energy X-ray image fusion
 */
class DualEnergyFusion {
public:
    DualEnergyFusion() 
        : m_initialized(false),
          m_width(0),
          m_height(0),
          m_highEnergyWeight(0.5f),
          m_lowEnergyWeight(0.5f),
          m_fusionMode(FUSION_WEIGHTED_AVERAGE)
    {}
    
    ~DualEnergyFusion() {
        release();
    }

    /**
     * @brief Initialize dual-energy fusion with image dimensions
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
        
        // Allocate temporary buffers for intermediate processing
        m_tempBuffer.resize(m_pixelCount, 0.0f);
        
        m_initialized = true;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Set fusion weights for high and low energy images
     * @param highWeight Weight for high-energy image (0.0 to 1.0)
     * @param lowWeight Weight for low-energy image (0.0 to 1.0)
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int setFusionWeights(float highWeight, float lowWeight) {
        if (highWeight < 0.0f || highWeight > 1.0f || 
            lowWeight < 0.0f || lowWeight > 1.0f) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        // Normalize weights to sum to 1.0
        float sum = highWeight + lowWeight;
        if (sum > 0.0f) {
            m_highEnergyWeight = highWeight / sum;
            m_lowEnergyWeight = lowWeight / sum;
        } else {
            m_highEnergyWeight = 0.5f;
            m_lowEnergyWeight = 0.5f;
        }

        return HUBX_SUCCESS;
    }

    /**
     * @brief Set fusion mode
     * @param mode Fusion mode to use
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int setFusionMode(FusionMode mode) {
        m_fusionMode = mode;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Perform weighted average fusion (default mode)
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param output Output fused image
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int fuseWeightedAverage(const unsigned short* highEnergy, 
                           const unsigned short* lowEnergy,
                           unsigned short* output,
                           int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                // Weighted average: fused = w_h * I_h + w_l * I_l
                float fused = m_highEnergyWeight * static_cast<float>(highEnergy[i]) +
                             m_lowEnergyWeight * static_cast<float>(lowEnergy[i]);
                
                // Clamp to valid range
                fused = std::max(0.0f, std::min(static_cast<float>(maxValue), fused));
                output[i] = static_cast<unsigned short>(fused + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Perform material decomposition fusion
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param output Output fused image
     * @param bitDepth Bit depth of data
     * @param materialCoeff Material decomposition coefficient
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int fuseMaterialDecomposition(const unsigned short* highEnergy,
                                  const unsigned short* lowEnergy,
                                  unsigned short* output,
                                  int bitDepth,
                                  float materialCoeff = 1.0f) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                float high = static_cast<float>(highEnergy[i]);
                float low = static_cast<float>(lowEnergy[i]);
                
                // Material decomposition: emphasize differences
                // fused = (I_h + materialCoeff * (I_h - I_l))
                float difference = high - low;
                float fused = high + materialCoeff * difference;
                
                // Clamp to valid range
                fused = std::max(0.0f, std::min(static_cast<float>(maxValue), fused));
                output[i] = static_cast<unsigned short>(fused + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Perform logarithmic fusion (for transmission imaging)
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param output Output fused image
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int fuseLogarithmic(const unsigned short* highEnergy,
                       const unsigned short* lowEnergy,
                       unsigned short* output,
                       int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;
        const float epsilon = 1.0f; // Avoid log(0)

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                float high = static_cast<float>(highEnergy[i]) + epsilon;
                float low = static_cast<float>(lowEnergy[i]) + epsilon;
                
                // Logarithmic fusion: fused = exp(w_h * log(I_h) + w_l * log(I_l))
                float logFused = m_highEnergyWeight * std::log(high) +
                                m_lowEnergyWeight * std::log(low);
                float fused = std::exp(logFused) - epsilon;
                
                // Clamp to valid range
                fused = std::max(0.0f, std::min(static_cast<float>(maxValue), fused));
                output[i] = static_cast<unsigned short>(fused + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Perform adaptive fusion based on local statistics
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param output Output fused image
     * @param bitDepth Bit depth of data
     * @param windowSize Window size for local statistics (odd number)
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int fuseAdaptive(const unsigned short* highEnergy,
                    const unsigned short* lowEnergy,
                    unsigned short* output,
                    int bitDepth,
                    int windowSize = 5) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr || output == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        if (windowSize % 2 == 0 || windowSize < 3) {
            windowSize = 5; // Default to 5x5 window
        }

        const int maxValue = (1 << bitDepth) - 1;
        const int halfWindow = windowSize / 2;

        try {
            // Calculate local variance and adapt weights
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    int idx = y * m_width + x;
                    
                    // Calculate local variance for both images
                    float varHigh = 0.0f, varLow = 0.0f;
                    float meanHigh = 0.0f, meanLow = 0.0f;
                    int count = 0;

                    for (int wy = -halfWindow; wy <= halfWindow; ++wy) {
                        for (int wx = -halfWindow; wx <= halfWindow; ++wx) {
                            int ny = y + wy;
                            int nx = x + wx;
                            
                            if (ny >= 0 && ny < m_height && nx >= 0 && nx < m_width) {
                                int nidx = ny * m_width + nx;
                                meanHigh += highEnergy[nidx];
                                meanLow += lowEnergy[nidx];
                                count++;
                            }
                        }
                    }

                    meanHigh /= count;
                    meanLow /= count;

                    // Calculate variance
                    for (int wy = -halfWindow; wy <= halfWindow; ++wy) {
                        for (int wx = -halfWindow; wx <= halfWindow; ++wx) {
                            int ny = y + wy;
                            int nx = x + wx;
                            
                            if (ny >= 0 && ny < m_height && nx >= 0 && nx < m_width) {
                                int nidx = ny * m_width + nx;
                                float diffHigh = highEnergy[nidx] - meanHigh;
                                float diffLow = lowEnergy[nidx] - meanLow;
                                varHigh += diffHigh * diffHigh;
                                varLow += diffLow * diffLow;
                            }
                        }
                    }

                    varHigh /= count;
                    varLow /= count;

                    // Adaptive weight based on local variance
                    float totalVar = varHigh + varLow + 1e-6f;
                    float adaptiveHighWeight = varHigh / totalVar;
                    float adaptiveLowWeight = varLow / totalVar;

                    // Fuse with adaptive weights
                    float fused = adaptiveHighWeight * highEnergy[idx] +
                                 adaptiveLowWeight * lowEnergy[idx];
                    
                    fused = std::max(0.0f, std::min(static_cast<float>(maxValue), fused));
                    output[idx] = static_cast<unsigned short>(fused + 0.5f);
                }
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Perform fusion with automatic mode selection
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param output Output fused image
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int fuse(const unsigned short* highEnergy,
            const unsigned short* lowEnergy,
            unsigned short* output,
            int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        switch (m_fusionMode) {
            case FUSION_WEIGHTED_AVERAGE:
                return fuseWeightedAverage(highEnergy, lowEnergy, output, bitDepth);
            
            case FUSION_MATERIAL_DECOMPOSITION:
                return fuseMaterialDecomposition(highEnergy, lowEnergy, output, bitDepth);
            
            case FUSION_ADAPTIVE:
                return fuseAdaptive(highEnergy, lowEnergy, output, bitDepth);
            
            case FUSION_LOGARITHMIC:
                return fuseLogarithmic(highEnergy, lowEnergy, output, bitDepth);
            
            default:
                return fuseWeightedAverage(highEnergy, lowEnergy, output, bitDepth);
        }
    }

    /**
     * @brief Calculate optimal fusion weights based on image statistics
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param optimalHighWeight Output: calculated optimal high-energy weight
     * @param optimalLowWeight Output: calculated optimal low-energy weight
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int calculateOptimalWeights(const unsigned short* highEnergy,
                               const unsigned short* lowEnergy,
                               float* optimalHighWeight,
                               float* optimalLowWeight) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr ||
            optimalHighWeight == nullptr || optimalLowWeight == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        try {
            // Calculate SNR (Signal-to-Noise Ratio) for both images
            double meanHigh = 0.0, meanLow = 0.0;
            double varHigh = 0.0, varLow = 0.0;

            // Calculate means
            for (int i = 0; i < m_pixelCount; ++i) {
                meanHigh += highEnergy[i];
                meanLow += lowEnergy[i];
            }
            meanHigh /= m_pixelCount;
            meanLow /= m_pixelCount;

            // Calculate variances
            for (int i = 0; i < m_pixelCount; ++i) {
                double diffHigh = highEnergy[i] - meanHigh;
                double diffLow = lowEnergy[i] - meanLow;
                varHigh += diffHigh * diffHigh;
                varLow += diffLow * diffLow;
            }
            varHigh /= m_pixelCount;
            varLow /= m_pixelCount;

            // SNR approximation: mean^2 / variance
            double snrHigh = (varHigh > 0) ? (meanHigh * meanHigh / varHigh) : 1.0;
            double snrLow = (varLow > 0) ? (meanLow * meanLow / varLow) : 1.0;

            // Weight by SNR
            double totalSNR = snrHigh + snrLow;
            *optimalHighWeight = static_cast<float>(snrHigh / totalSNR);
            *optimalLowWeight = static_cast<float>(snrLow / totalSNR);

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Generate material-specific images (decomposition)
     * @param highEnergy High-energy image data
     * @param lowEnergy Low-energy image data
     * @param organicOutput Output image emphasizing organic materials
     * @param inorganicOutput Output image emphasizing inorganic materials
     * @param bitDepth Bit depth of data
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int decomposeMaterials(const unsigned short* highEnergy,
                          const unsigned short* lowEnergy,
                          unsigned short* organicOutput,
                          unsigned short* inorganicOutput,
                          int bitDepth) {
        if (!m_initialized) {
            return HUBX_ERROR_INVALID_PARAM;
        }

        if (highEnergy == nullptr || lowEnergy == nullptr ||
            organicOutput == nullptr || inorganicOutput == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        const int maxValue = (1 << bitDepth) - 1;

        try {
            for (int i = 0; i < m_pixelCount; ++i) {
                float high = static_cast<float>(highEnergy[i]);
                float low = static_cast<float>(lowEnergy[i]);
                
                // Organic materials: more absorption at low energy
                // Emphasis on low energy difference
                float organic = low - 0.5f * high;
                organic = std::max(0.0f, std::min(static_cast<float>(maxValue), organic));
                organicOutput[i] = static_cast<unsigned short>(organic + 0.5f);
                
                // Inorganic materials: relatively uniform absorption
                // Emphasis on high energy
                float inorganic = high - 0.3f * (high - low);
                inorganic = std::max(0.0f, std::min(static_cast<float>(maxValue), inorganic));
                inorganicOutput[i] = static_cast<unsigned short>(inorganic + 0.5f);
            }

            return HUBX_SUCCESS;
        }
        catch (const std::exception&) {
            return HUBX_ERROR_CALCULATION;
        }
    }

    /**
     * @brief Get current fusion weights
     * @param highWeight Output: current high-energy weight
     * @param lowWeight Output: current low-energy weight
     * @return HUBX_SUCCESS on success, error code otherwise
     */
    int getFusionWeights(float* highWeight, float* lowWeight) const {
        if (highWeight == nullptr || lowWeight == nullptr) {
            return HUBX_ERROR_NULL_POINTER;
        }

        *highWeight = m_highEnergyWeight;
        *lowWeight = m_lowEnergyWeight;
        return HUBX_SUCCESS;
    }

    /**
     * @brief Release resources
     */
    void release() {
        m_tempBuffer.clear();
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
    float m_highEnergyWeight;
    float m_lowEnergyWeight;
    FusionMode m_fusionMode;
    std::vector<float> m_tempBuffer;
};

// Global instance for C-style API
static DualEnergyFusion g_dualEnergyFusion;

} // namespace Correction
} // namespace HubxSDK

// C-style API for compatibility
extern "C" {

/**
 * @brief Initialize dual-energy fusion module
 */
int hubx_dualenergy_init(int width, int height) {
    return HubxSDK::Correction::g_dualEnergyFusion.initialize(width, height);
}

/**
 * @brief Set fusion weights
 */
int hubx_dualenergy_set_weights(float highWeight, float lowWeight) {
    return HubxSDK::Correction::g_dualEnergyFusion.setFusionWeights(highWeight, lowWeight);
}

/**
 * @brief Set fusion mode
 */
int hubx_dualenergy_set_mode(int mode) {
    return HubxSDK::Correction::g_dualEnergyFusion.setFusionMode(
        static_cast<HubxSDK::Correction::FusionMode>(mode));
}

/**
 * @brief Perform fusion with current settings
 */
int hubx_dualenergy_fuse(const unsigned short* highEnergy,
                         const unsigned short* lowEnergy,
                         unsigned short* output,
                         int bitDepth) {
    return HubxSDK::Correction::g_dualEnergyFusion.fuse(highEnergy, lowEnergy, output, bitDepth);
}

/**
 * @brief Perform weighted average fusion
 */
int hubx_dualenergy_fuse_weighted(const unsigned short* highEnergy,
                                  const unsigned short* lowEnergy,
                                  unsigned short* output,
                                  int bitDepth) {
    return HubxSDK::Correction::g_dualEnergyFusion.fuseWeightedAverage(
        highEnergy, lowEnergy, output, bitDepth);
}

/**
 * @brief Perform material decomposition fusion
 */
int hubx_dualenergy_fuse_material(const unsigned short* highEnergy,
                                  const unsigned short* lowEnergy,
                                  unsigned short* output,
                                  int bitDepth,
                                  float materialCoeff) {
    return HubxSDK::Correction::g_dualEnergyFusion.fuseMaterialDecomposition(
        highEnergy, lowEnergy, output, bitDepth, materialCoeff);
}

/**
 * @brief Perform adaptive fusion
 */
int hubx_dualenergy_fuse_adaptive(const unsigned short* highEnergy,
                                  const unsigned short* lowEnergy,
                                  unsigned short* output,
                                  int bitDepth,
                                  int windowSize) {
    return HubxSDK::Correction::g_dualEnergyFusion.fuseAdaptive(
        highEnergy, lowEnergy, output, bitDepth, windowSize);
}

/**
 * @brief Calculate optimal fusion weights
 */
int hubx_dualenergy_calc_weights(const unsigned short* highEnergy,
                                 const unsigned short* lowEnergy,
                                 float* optimalHighWeight,
                                 float* optimalLowWeight) {
    return HubxSDK::Correction::g_dualEnergyFusion.calculateOptimalWeights(
        highEnergy, lowEnergy, optimalHighWeight, optimalLowWeight);
}

/**
 * @brief Decompose materials
 */
int hubx_dualenergy_decompose(const unsigned short* highEnergy,
                              const unsigned short* lowEnergy,
                              unsigned short* organicOutput,
                              unsigned short* inorganicOutput,
                              int bitDepth) {
    return HubxSDK::Correction::g_dualEnergyFusion.decomposeMaterials(
        highEnergy, lowEnergy, organicOutput, inorganicOutput, bitDepth);
}

/**
 * @brief Get current fusion weights
 */
int hubx_dualenergy_get_weights(float* highWeight, float* lowWeight) {
    return HubxSDK::Correction::g_dualEnergyFusion.getFusionWeights(highWeight, lowWeight);
}

/**
 * @brief Release dual-energy fusion resources
 */
void hubx_dualenergy_release() {
    HubxSDK::Correction::g_dualEnergyFusion.release();
}

} // extern "C"