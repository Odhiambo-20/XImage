/**
 * @file xlibdll_interface.h
 * @brief Internal interface for xlibdll.dll proxy
 * @warning This is for INTERNAL USE ONLY - not part of public API
 */

#ifndef XLIBDLL_INTERFACE_H
#define XLIBDLL_INTERFACE_H

namespace HX {

/**
 * @brief Initialize the xlibdll proxy layer
 * @param dllPath Optional path to xlibdll.dll (nullptr for default)
 * @return true on success
 */
bool XLibProxy_Initialize(const char* dllPath = nullptr);

/**
 * @brief Shutdown and unload xlibdll
 */
void XLibProxy_Shutdown();

/**
 * @brief Connect to device via xlibdll
 * @param config Configuration string
 * @return Status code (0 = success, <0 = error)
 */
int XLibProxy_Connect(const char* config);

/**
 * @brief Send command via xlibdll
 * @param command Command string
 * @param response Response buffer
 * @param responseSize Response buffer size
 * @return Status code
 */
int XLibProxy_SendCommand(const char* command, char* response, int responseSize);

/**
 * @brief Receive data via xlibdll
 * @param buffer Data buffer
 * @param bufferSize Buffer size
 * @return Number of bytes received or error code
 */
int XLibProxy_ReceiveData(unsigned char* buffer, int bufferSize);

/**
 * @brief Check if xlibdll is loaded
 * @return true if loaded
 */
bool XLibProxy_IsLoaded();

} // namespace HX

#endif // XLIBDLL_INTERFACE_H
