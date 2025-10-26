/**
 * @file xlibdll_proxy_usage_example.cpp
 * @brief Example of how xlibdll proxy is used internally by hubx.dll classes
 * 
 * This demonstrates how XControl, XGrabber, and other classes use the proxy
 * to access xlibdll.dll functionality without exposing it externally.
 * 
 * @copyright Copyright (c) 2024 FXImage Development Team
 * @version 2.1.0
 */

#include "xlibdll_interface.h"
#include <iostream>
#include <vector>
#include <cstring>

using namespace HX::Internal;

/**
 * @brief Example: Initialize xlibdll proxy on application startup
 * 
 * This would typically be called in XFactory or during library initialization
 */
bool InitializeHubxLibrary() {
    std::cout << "Initializing HubxSDK..." << std::endl;
    
    // Initialize the xlibdll proxy
    if (!XLibProxy_Initialize()) {
        std::cerr << "ERROR: Failed to initialize xlibdll proxy" << std::endl;
        std::cerr << "Make sure xlibdll.dll is in the correct location" << std::endl;
        return false;
    }
    
    std::cout << "xlibdll proxy initialized successfully" << std::endl;
    std::cout << "xlibdll is now hidden and can only be accessed through hubx.dll" << std::endl;
    
    return true;
}

/**
 * @brief Example: How XAdaptor uses the proxy to discover detectors
 * 
 * This shows how XAdaptor::Connect() would internally use the proxy
 */
int DiscoverDetectorsExample(const char* localIP) {
    std::cout << "\n=== Discovering Detectors ===" << std::endl;
    
    // Check if proxy is initialized
    if (!XLibProxy_IsLoaded()) {
        std::cerr << "ERROR: xlibdll proxy not initialized" << std::endl;
        return -1;
    }
    
    // Discover devices using proxy
    int32_t deviceCount = XLibProxy_DiscoverDevices(localIP);
    
    if (deviceCount < 0) {
        std::cerr << "ERROR: Failed to discover devices: " 
                  << XLibProxy_GetErrorMessage(deviceCount) << std::endl;
        return deviceCount;
    }
    
    std::cout << "Found " << deviceCount << " detector(s)" << std::endl;
    
    // Get information about each discovered device
    for (int32_t i = 0; i < deviceCount; ++i) {
        XLibDeviceInfo info;
        
        if (XLibProxy_GetDeviceInfo(i, &info) == 0) {
            char macStr[18];
            XLib_MACToString(info.mac, macStr);
            
            std::cout << "\nDetector " << (i + 1) << ":" << std::endl;
            std::cout << "  MAC: " << macStr << std::endl;
            std::cout << "  IP: " << info.ip << std::endl;
            std::cout << "  Command Port: " << info.cmdPort << std::endl;
            std::cout << "  Image Port: " << info.imgPort << std::endl;
            std::cout << "  Serial: " << info.serialNumber << std::endl;
            std::cout << "  Pixels: " << info.pixelCount << std::endl;
            std::cout << "  Modules: " << static_cast<int>(info.moduleCount) << std::endl;
        }
    }
    
    return deviceCount;
}

/**
 * @brief Example: How XControl uses the proxy to send commands
 * 
 * This shows how XControl::Write() would internally send commands
 */
int SendCommandExample(const char* detectorIP, uint16_t cmdPort) {
    std::cout << "\n=== Sending Command to Detector ===" << std::endl;
    
    // Initialize network if not already done
    int32_t result = XLibProxy_InitNetwork(detectorIP, cmdPort);
    if (result < 0) {
        std::cerr << "ERROR: Failed to initialize network: "
                  << XLibProxy_GetErrorMessage(result) << std::endl;
        return result;
    }
    
    // Build a command packet (example: read integration time)
    // Command format from TDI04_8S_Command_List: 0x20 0x02 0x00 0x00
    uint8_t command[4] = {0x20, 0x02, 0x00, 0x00}; // [ST,R,0]
    
    uint8_t response[256];
    uint32_t responseLen = sizeof(response);
    
    // Send command through proxy
    result = XLibProxy_SendCommand(
        command, sizeof(command),
        response, &responseLen,
        XLIB_DEFAULT_CMD_TIMEOUT
    );
    
    if (result < 0) {
        std::cerr << "ERROR: Failed to send command: "
                  << XLibProxy_GetErrorMessage(result) << std::endl;
        return result;
    }
    
    std::cout << "Command sent successfully" << std::endl;
    std::cout << "Response received: " << result << " bytes" << std::endl;
    
    // Parse response
    if (responseLen >= 4) {
        uint32_t integrationTime = (response[0] << 24) | (response[1] << 16) |
                                   (response[2] << 8) | response[3];
        std::cout << "Integration time: " << integrationTime << " μs" << std::endl;
    }
    
    return 0;
}

/**
 * @brief Example: How XGrabber uses the proxy to receive image data
 * 
 * This shows how XGrabber::Grab() would internally receive image packets
 */
int ReceiveImageDataExample() {
    std::cout << "\n=== Receiving Image Data ===" << std::endl;
    
    const uint32_t BUFFER_SIZE = XLIB_MAX_IMAGE_PACKET_SIZE;
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    
    // Receive image data through proxy
    int32_t bytesReceived = XLibProxy_ReceiveImageData(
        buffer.data(),
        BUFFER_SIZE,
        XLIB_DEFAULT_IMG_TIMEOUT
    );
    
    if (bytesReceived < 0) {
        if (bytesReceived == XLIB_ERROR_TIMEOUT) {
            std::cout << "No image data (timeout)" << std::endl;
        } else {
            std::cerr << "ERROR: Failed to receive image data: "
                      << XLibProxy_GetErrorMessage(bytesReceived) << std::endl;
        }
        return bytesReceived;
    }
    
    std::cout << "Received " << bytesReceived << " bytes of image data" << std::endl;
    
    // Extract packet header
    XLibPacketHeader header;
    if (XLibProxy_ExtractPacketHeader(buffer.data(), &header) == 0) {
        std::cout << "Packet Header:" << std::endl;
        std::cout << "  Packet ID: " << header.packetId << std::endl;
        std::cout << "  Line ID: " << header.lineId << std::endl;
        std::cout << "  Timestamp: " << header.timestamp << " μs" << std::endl;
        std::cout << "  Energy Flag: " << static_cast<int>(header.energyFlag) << std::endl;
        std::cout << "  Module ID: " << static_cast<int>(header.moduleId) << std::endl;
        std::cout << "  Data Length: " << header.dataLength << " bytes" << std::endl;
    }
    
    // Parse image data
    std::vector<uint8_t> imageData(BUFFER_SIZE);
    uint32_t imageLen = imageData.size();
    
    if (XLibProxy_ParseImagePacket(buffer.data(), bytesReceived,
                                   imageData.data(), &imageLen) == 0) {
        std::cout << "Successfully parsed " << imageLen << " bytes of image data" << std::endl;
        
        // Now this data would be passed to XFrame for line assembly
        // and eventually to correction algorithms
    }
    
    return 0;
}

/**
 * @brief Example: How to configure a detector through the proxy
 * 
 * This shows how XAdaptor::ConfigDetector() would work internally
 */
int ConfigureDetectorExample(const uint8_t* mac, const char* newIP,
                             uint16_t cmdPort, uint16_t imgPort) {
    std::cout << "\n=== Configuring Detector ===" << std::endl;
    
    char macStr[18];
    XLib_MACToString(mac, macStr);
    
    std::cout << "Configuring detector:" << std::endl;
    std::cout << "  MAC: " << macStr << std::endl;
    std::cout << "  New IP: " << newIP << std::endl;
    std::cout << "  Command Port: " << cmdPort << std::endl;
    std::cout << "  Image Port: " << imgPort << std::endl;
    
    // Validate IP address
    if (!XLib_ValidateIP(newIP)) {
        std::cerr << "ERROR: Invalid IP address format" << std::endl;
        return XLIB_ERROR_INVALID_PARAM;
    }
    
    // Configure through proxy
    int32_t result = XLibProxy_ConfigureDevice(mac, newIP, cmdPort, imgPort);
    
    if (result < 0) {
        std::cerr << "ERROR: Failed to configure device: "
                  << XLibProxy_GetErrorMessage(result) << std::endl;
        return result;
    }
    
    std::cout << "Device configured successfully" << std::endl;
    std::cout << "Please wait for device to reboot..." << std::endl;
    
    return 0;
}

/**
 * @brief Example: Complete workflow using the proxy
 * 
 * This demonstrates a typical detection, configuration, and acquisition workflow
 */
int CompleteWorkflowExample() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Complete Workflow Example" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Step 1: Initialize the library (done once at startup)
    if (!InitializeHubxLibrary()) {
        return -1;
    }
    
    // Step 2: Discover detectors on the network
    const char* localIP = "192.168.1.100";
    int deviceCount = DiscoverDetectorsExample(localIP);
    
    if (deviceCount <= 0) {
        std::cerr << "No detectors found" << std::endl;
        return -1;
    }
    
    // Step 3: Get information about first detector
    XLibDeviceInfo detectorInfo;
    if (XLibProxy_GetDeviceInfo(0, &detectorInfo) != 0) {
        std::cerr << "Failed to get detector info" << std::endl;
        return -1;
    }
    
    // Step 4: Send commands to configure detector
    SendCommandExample(detectorInfo.ip, detectorInfo.cmdPort);
    
    // Step 5: Start image acquisition
    std::cout << "\nStarting image acquisition..." << std::endl;
    std::cout << "Press Ctrl+C to stop (in real app)" << std::endl;
    
    // In a real application, this would be in a loop
    for (int i = 0; i < 5; ++i) {
        std::cout << "\nReceiving frame " << (i + 1) << "..." << std::endl;
        ReceiveImageDataExample();
    }
    
    // Step 6: Cleanup
    std::cout << "\nCleaning up..." << std::endl;
    XLibProxy_CloseNetwork();
    XLibProxy_Cleanup();
    
    std::cout << "\nWorkflow complete!" << std::endl;
    std::cout << "xlibdll.dll was hidden throughout - only accessed via proxy" << std::endl;
    
    return 0;
}

/**
 * @brief Example: Error handling with the proxy
 */
void ErrorHandlingExample() {
    std::cout << "\n=== Error Handling Example ===" << std::endl;
    
    // Attempt operation that might fail
    int32_t result = XLibProxy_DiscoverDevices("invalid.ip.address");
    
    if (result < 0) {
        // Get error details
        int32_t errorCode = XLibProxy_GetLastError();
        const char* errorMsg = XLibProxy_GetErrorMessage(errorCode);
        
        std::cerr << "Operation failed!" << std::endl;
        std::cerr << "Error Code: " << errorCode << std::endl;
        std::cerr << "Error Message: " << errorMsg << std::endl;
        
        // Handle specific errors
        switch (errorCode) {
            case XLIB_ERROR_NETWORK:
                std::cerr << "Network error - check IP address and connections" << std::endl;
                break;
            case XLIB_ERROR_TIMEOUT:
                std::cerr << "Operation timed out - detector may be offline" << std::endl;
                break;
            case XLIB_ERROR_INVALID_PARAM:
                std::cerr << "Invalid parameter - check input values" << std::endl;
                break;
            default:
                std::cerr << "Unexpected error occurred" << std::endl;
        }
    }
}

/**
 * @brief Main function - demonstrates all examples
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "xlibdll Proxy Usage Examples" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThese examples show how hubx.dll internally uses" << std::endl;
    std::cout << "the xlibdll proxy to hide xlibdll.dll from external access" << std::endl;
    
    // Run complete workflow
    CompleteWorkflowExample();
    
    // Show error handling
    ErrorHandlingExample();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Key Points:" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "1. xlibdll.dll is loaded dynamically by the proxy" << std::endl;
    std::cout << "2. All xlibdll functions are accessed through XLibProxy_* functions" << std::endl;
    std::cout << "3. These proxy functions are NOT exported from hubx.dll" << std::endl;
    std::cout << "4. External applications can only use the public XControl, XGrabber, etc. API" << std::endl;
    std::cout << "5. xlibdll.dll remains completely hidden from end users" << std::endl;
    
    return 0;
}