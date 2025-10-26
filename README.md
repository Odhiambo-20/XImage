# FXImage 2.1.0

## Overview
FXImage 2.1.0 is an X-ray detector control and image acquisition software system.

## Project Structure
- **HubxSDK/** - Core SDK library (hubx.dll/libhubx.so)
- **FXImage/** - Qt-based GUI application
- **tests/** - Unit and integration tests
- **docs/** - Documentation
- **scripts/** - Build and deployment scripts

## Building

### Linux
```bash
./scripts/build_all.sh
```

### Windows
Use Visual Studio or Qt Creator to build the projects.

## Requirements
- Qt 5.12 or higher
- CMake 3.10 or higher
- C++11 compatible compiler
- xlibdll.dll (provided)

## Development Tasks

### Priority 1: Core Development
1. **dll hidden** - Implement xlibdll.dll wrapper/proxy
2. **SDK packaging** - Create hubx.dll with all required APIs
3. **Several algorithms** - Implement correction algorithms
4. **Several commands for communication with lower machine** - Implement command interface

## License
[To be specified]

## Contact
[To be specified]
