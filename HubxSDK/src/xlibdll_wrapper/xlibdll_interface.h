/**
 * @file xlibdll_interface.h
 * @brief Internal interface definitions for xlibdll proxy
 * 
 * This header is INTERNAL to hubx.dll and should NOT be exposed externally.
 * It defines structures and functions for communicating with xlibdll.dll
 * 
 * @copyright Copyright (c) 2024 FXImage Development Team
 * @version 2.1.0
 */

#ifndef XLIBDLL_INTERFACE_H
#define XLIBDLL_INTERFACE_H

#include <cstdint>
#include <cstdio>

namespace HX {
namespace Internal {

// ============================================================================
// Forward Declarations
// ============================================================================

struct XLibPacketHeader;

// ============================================================================
// Error Codes
// ============================================================================

/**
 * @enum XLibErrorCode
 * @brief Error codes from xlibdll
 */
enum XLibErrorCode {
    XLIB_SUCCESS = 0,                  ///< Success
    XLIB_ERROR_GENERAL = -1,           ///< General error
    XLIB_ERROR_NETWORK = -2,           ///< Network error
    XLIB_ERROR_TIMEOUT = -3,           ///< Operation timeout
    XLIB_ERROR_INVALID_PARAM = -4,     ///< Invalid parameter
    XLIB_ERROR_DEVICE_NOT_FOUND = -5,  ///< Device not found
    XLIB_ERROR_CONNECTION = -6,        ///< Connection error
    XLIB_ERROR_SEND_FAILED = -7,       ///< Send failed
    XLIB_ERROR_RECEIVE_FAILED = -8,    ///< Receive failed
    XLIB_ERROR_PARSE_FAILED = -9,      ///< Parse error
    XLIB_ERROR_CHECKSUM = -10,         ///< Checksum error
    XLIB_ERROR_BUFFER_OVERFLOW = -11,  ///< Buffer overflow
    XLIB_ERROR_NOT_INITIALIZED = -12,  ///< Not initialized
    XLIB_ERROR_ALREADY_OPEN = -13,     ///< Already opened
    XLIB_ERROR_NOT_OPEN = -14,         ///< Not opened
    XLIB_ERROR_NO_DEVICE = -15         ///< No device available
};

/**
 * @enum XLibOperationMode
 * @brief Operation modes supported by xlibdll
 */
enum XLibOperationMode {
    XLIB_MODE_CONTINUOUS = 0,          ///< Continuous mode
    XLIB_MODE_NON_CONTINUOUS = 1,      ///< Non-continuous mode
    XLIB_MODE_FIXED_INTEGRATION = 2,   ///< Fixed integration time mode
    XLIB_MODE_DUAL_ENERGY = 3          ///< Dual energy mode
};

/**
 * @enum XLibTriggerMode
 * @brief Trigger modes
 */
enum XLibTriggerMode {
    XLIB_TRIGGER_RISING_EDGE = 0,      ///< Rising edge trigger
    XLIB_TRIGGER_FALLING_EDGE = 1,     ///< Falling edge trigger
    XLIB_TRIGGER_SYNC_FLAG = 2,        ///< Synchronous flag
    XLIB_TRIGGER_ASYNC_FLAG = 3        ///< Asynchronous flag
};

// ============================================================================
// Data Structures for xlibdll communication
// ============================================================================

/**
 * @struct XLibDeviceInfo
 * @brief Device information structure from xlibdll
 */
struct XLibDeviceInfo {
    uint8_t mac[6];           ///< MAC address
    char ip[32];              ///< IP address string
    uint16_t cmdPort;         ///< Command port
    uint16_t imgPort;         ///< Image data port
    char serialNumber[32];    ///< Device serial number
    uint32_t pixelCount;      ///< Total pixel count
    uint8_t moduleCount;      ///< Number of detector modules
    uint8_t cardType;         ///< Card type identifier
    uint16_t firmwareVersion; ///< Firmware version
    uint16_t checksum;        ///< CRC16 checksum
    uint8_t reserved[62];     ///< Reserved for future use
};

/**
 * @struct XLibPacketHeader
 * @brief Image packet header structure
 */
struct XLibPacketHeader {
    uint32_t packetId;        ///< Packet sequence number
    uint16_t lineId;          ///< Line number
    uint32_t timestamp;       ///< Timestamp in microseconds
    uint8_t energyFlag;       ///< Energy mode flag (0=low, 1=high)
    uint8_t moduleId;         ///< Module identifier
    uint16_t dataLength;      ///< Payload data length
    uint16_t checksum;        ///< CRC checksum
    uint8_t reserved[8];      ///< Reserved
};

/**
 * @struct XLibNetworkConfig
 * @brief Network configuration structure
 */
struct XLibNetworkConfig {
    char localIP[32];         ///< Local adapter IP
    uint16_t localPort;       ///< Local port for binding
    char remoteIP[32];        ///< Remote device IP
    uint16_t cmdPort;         ///< Command channel port
    uint16_t imgPort;         ///< Image data channel port
    uint32_t timeout;         ///< Default timeout (ms)
    uint32_t bufferSize;      ///< Socket buffer size
    uint8_t reserved[32];     ///< Reserved
};

/**
 * @struct XLibCommandPacket
 * @brief Command packet structure for xlibdll
 */
struct XLibCommandPacket {
    uint16_t header;          ///< Packet header (0xAA55)
    uint8_t command;          ///< Command code
    uint8_t operation;        ///< Operation type (read/write/execute)
    uint8_t dmId;             ///< DM module ID (0xFF for all)
    uint8_t dataLength;       ///< Payload length
    uint8_t data[256];        ///< Command data
    uint16_t checksum;        ///< CRC16 checksum
};

/**
 * @struct XLibResponsePacket
 * @brief Response packet structure from xlibdll
 */
struct XLibResponsePacket {
    uint16_t header;          ///< Packet header (0xAA55)
    uint8_t command;          ///< Echo of command
    uint8_t operation;        ///< Echo of operation
    uint8_t errorCode;        ///< Error code (0=success)
    uint8_t dataLength;       ///< Response data length
    uint8_t data[256];        ///< Response data
    uint16_t checksum;        ///< CRC16 checksum
};

/**
 * @struct XLibImageBuffer
 * @brief Image buffer structure for data transfer
 */
struct XLibImageBuffer {
    uint8_t* data;            ///< Pointer to image data
    uint32_t size;            ///< Buffer size in bytes
    uint32_t width;           ///< Image width (pixels)
    uint32_t height;          ///< Image height (lines)
    uint8_t pixelDepth;       ///< Bits per pixel (16/18/20/24)
    uint32_t lineId;          ///< Current line ID
    uint32_t timestamp;       ///< Acquisition timestamp
    uint8_t reserved[16];     ///< Reserved
};

/**
 * @struct XLibDetectorConfig
 * @brief Detector configuration from xlibdll
 */
struct XLibDetectorConfig {
    uint32_t integrationTime;    ///< Integration time (microseconds)
    uint32_t nonIntegrationTime; ///< Non-continuous integration time
    uint8_t operationMode;       ///< Operation mode
    uint8_t dmCount;             ///< Number of DM modules
    uint16_t pixelsPerDM;        ///< Pixels per DM module
    uint8_t pixelDepth;          ///< Bits per pixel
    uint8_t cardType;            ///< Card type
    uint16_t lineRate;           ///< Line rate (Hz)
    uint8_t triggerMode;         ///< Trigger mode
    uint8_t triggerEnabled;      ///< Trigger enable flag
    uint8_t reserved[32];        ///< Reserved
};

/**
 * @struct XLibCalibrationData
 * @brief Calibration data structure
 */
struct XLibCalibrationData {
    float* gainCoefficients;     ///< Gain correction coefficients
    float* offsetCoefficients;   ///< Offset correction coefficients
    uint32_t pixelCount;         ///< Number of pixels
    uint32_t targetValue;        ///< Target calibration value
    uint8_t energyMode;          ///< Energy mode (0=single, 1=dual)
    uint8_t reserved[32];        ///< Reserved
};

// ============================================================================
// Constants
// ============================================================================

/// Maximum number of detectors that can be discovered
const uint32_t XLIB_MAX_DEVICES = 16;

/// Maximum command packet size
const uint32_t XLIB_MAX_COMMAND_SIZE = 512;

/// Maximum response packet size
const uint32_t XLIB_MAX_RESPONSE_SIZE = 512;

/// Maximum image packet size
const uint32_t XLIB_MAX_IMAGE_PACKET_SIZE = 65536;

/// Default command timeout (milliseconds)
const uint32_t XLIB_DEFAULT_CMD_TIMEOUT = 5000;

/// Default image receive timeout (milliseconds)
const uint32_t XLIB_DEFAULT_IMG_TIMEOUT = 1000;

/// Default network buffer size
const uint32_t XLIB_DEFAULT_BUFFER_SIZE = 131072; // 128KB

/// UDP packet header size
const uint32_t XLIB_UDP_HEADER_SIZE = 28;

/// Command packet header signature
const uint16_t XLIB_PACKET_HEADER = 0xAA55;

/// Maximum serial number length
const uint32_t XLIB_MAX_SERIAL_LENGTH = 32;

/// Maximum IP address length
const uint32_t XLIB_MAX_IP_LENGTH = 32;

// ============================================================================
// Internal C API for xlibdll proxy
// These functions are used internally by hubx.dll classes
// They are NOT exported from hubx.dll
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Initialization Functions
// ============================================================================

/**
 * @brief Initialize xlibdll proxy
 * @return true on success, false on failure
 * @internal This function is for internal use only
 */
bool XLibProxy_Initialize();

/**
 * @brief Cleanup xlibdll proxy
 * @internal This function is for internal use only
 */
void XLibProxy_Cleanup();

/**
 * @brief Check if xlibdll is loaded
 * @return true if loaded, false otherwise
 * @internal This function is for internal use only
 */
bool XLibProxy_IsLoaded();

// ============================================================================
// Network Functions
// ============================================================================

/**
 * @brief Initialize network interface
 * @param localIP Local IP address string
 * @param port Port number
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_InitNetwork(const char* localIP, uint16_t port);

/**
 * @brief Close network interface
 * @internal This function is for internal use only
 */
void XLibProxy_CloseNetwork();

/**
 * @brief Send command to detector
 * @param cmd Command buffer
 * @param cmdLen Command length
 * @param response Response buffer
 * @param responseLen Pointer to response length
 * @param timeout Timeout in milliseconds
 * @return Bytes received on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_SendCommand(const uint8_t* cmd, uint32_t cmdLen,
                               uint8_t* response, uint32_t* responseLen,
                               uint32_t timeout);

/**
 * @brief Receive image data packet
 * @param buffer Data buffer
 * @param bufferSize Buffer size
 * @param timeout Timeout in milliseconds
 * @return Bytes received on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_ReceiveImageData(uint8_t* buffer, uint32_t bufferSize,
                                   uint32_t timeout);

// ============================================================================
// Device Discovery Functions
// ============================================================================

/**
 * @brief Discover devices on network
 * @param localIP Local IP address for broadcasting
 * @return Number of devices found, or negative error code
 * @internal This function is for internal use only
 */
int32_t XLibProxy_DiscoverDevices(const char* localIP);

/**
 * @brief Get information about discovered device
 * @param index Device index (0-based)
 * @param info Pointer to device info structure
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_GetDeviceInfo(uint32_t index, XLibDeviceInfo* info);

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * @brief Configure device network settings
 * @param mac MAC address (6 bytes)
 * @param ip New IP address string
 * @param cmdPort Command port
 * @param imgPort Image port
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_ConfigureDevice(const uint8_t* mac, const char* ip,
                                  uint16_t cmdPort, uint16_t imgPort);

/**
 * @brief Reset device to default settings
 * @param mac MAC address (6 bytes)
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_ResetDevice(const uint8_t* mac);

// ============================================================================
// Data Processing Functions
// ============================================================================

/**
 * @brief Parse raw image packet
 * @param rawData Raw packet data
 * @param rawLen Raw packet length
 * @param imageData Parsed image data output
 * @param imageLen Pointer to image data length
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_ParseImagePacket(const uint8_t* rawData, uint32_t rawLen,
                                   uint8_t* imageData, uint32_t* imageLen);

/**
 * @brief Extract packet header information
 * @param rawData Raw packet data
 * @param header Pointer to header structure
 * @return 0 on success, negative error code on failure
 * @internal This function is for internal use only
 */
int32_t XLibProxy_ExtractPacketHeader(const uint8_t* rawData,
                                      XLibPacketHeader* header);

// ============================================================================
// Error Handling Functions
// ============================================================================

/**
 * @brief Get last error code from xlibdll
 * @return Error code
 * @internal This function is for internal use only
 */
int32_t XLibProxy_GetLastError();

/**
 * @brief Get error message for error code
 * @param errorCode Error code
 * @return Error message string
 * @internal This function is for internal use only
 */
const char* XLibProxy_GetErrorMessage(int32_t errorCode);

#ifdef __cplusplus
}
#endif

// ============================================================================
// Helper Macros
// ============================================================================

/**
 * @def XLIB_CHECK_INITIALIZED
 * @brief Check if xlibdll proxy is initialized
 */
#define XLIB_CHECK_INITIALIZED() \
    do { \
        if (!XLibProxy_IsLoaded()) { \
            return XLIB_ERROR_NOT_INITIALIZED; \
        } \
    } while(0)

/**
 * @def XLIB_CHECK_POINTER
 * @brief Check if pointer is valid
 */
#define XLIB_CHECK_POINTER(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            return XLIB_ERROR_INVALID_PARAM; \
        } \
    } while(0)

/**
 * @def XLIB_SAFE_CALL
 * @brief Safely call xlibdll function with error checking
 */
#define XLIB_SAFE_CALL(func) \
    do { \
        int32_t result = (func); \
        if (result < 0) { \
            return result; \
        } \
    } while(0)

// ============================================================================
// Utility Functions (inline)
// ============================================================================

/**
 * @brief Calculate CRC16 checksum
 * @param data Data buffer
 * @param length Data length
 * @return CRC16 checksum
 */
inline uint16_t XLib_CalculateCRC16(const uint8_t* data, uint32_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint32_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief Verify CRC16 checksum
 * @param data Data buffer (including checksum at end)
 * @param length Total length including checksum
 * @return true if checksum is valid
 */
inline bool XLib_VerifyCRC16(const uint8_t* data, uint32_t length) {
    if (length < 2) return false;
    
    uint16_t calculated = XLib_CalculateCRC16(data, length - 2);
    uint16_t received = (static_cast<uint16_t>(data[length - 1]) << 8) | 
                        static_cast<uint16_t>(data[length - 2]);
    
    return calculated == received;
}

/**
 * @brief Convert MAC address to string
 * @param mac MAC address (6 bytes)
 * @param str Output string buffer (min 18 bytes)
 */
inline void XLib_MACToString(const uint8_t* mac, char* str) {
    snprintf(str, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/**
 * @brief Convert string to MAC address
 * @param str MAC address string (format: XX:XX:XX:XX:XX:XX)
 * @param mac Output MAC address (6 bytes)
 * @return true on success
 */
inline bool XLib_StringToMAC(const char* str, uint8_t* mac) {
    int values[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return false;
    }
    
    for (int i = 0; i < 6; ++i) {
        mac[i] = static_cast<uint8_t>(values[i]);
    }
    
    return true;
}

/**
 * @brief Validate IP address string
 * @param ip IP address string
 * @return true if valid
 */
inline bool XLib_ValidateIP(const char* ip) {
    if (!ip) return false;
    
    int a, b, c, d;
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        return false;
    }
    
    return (a >= 0 && a <= 255) && (b >= 0 && b <= 255) &&
           (c >= 0 && c <= 255) && (d >= 0 && d <= 255);
}

} // namespace Internal
} // namespace HX

#endif // XLIBDLL_INTERFACE_H