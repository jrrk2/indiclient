# INDI Test Client Project Structure

## Overview

The INDI Test Client is organized as follows:

```
INDITestClient/
├── CMakeLists.txt                  # Main build system file
├── README.md                       # Project documentation
│
├── src/                            # Source directory
│   ├── INDITestClient.h            # Main header file with class declarations
│   ├── INDIClient.cpp              # INDI client implementation
│   ├── MainWindow.cpp              # Main application window implementation
│   ├── CameraPanel.cpp             # Camera control panel implementation
│   ├── MountPanel.cpp              # Mount control panel implementation
│   ├── PlateSolverPanel.cpp        # Plate solver panel implementation 
│   ├── MountModelPanel.cpp         # Mount modeling panel implementation
│   ├── ImagePanel.cpp              # Image display panel implementation
│   └── main.cpp                    # Application entry point
│
└── resources/                      # Application resources
    └── icons/                      # UI icons
```

## Key Classes and Their Responsibilities

### INDIClient
- Manages the connection to the INDI server
- Handles device discovery and property updates
- Provides methods for controlling INDI devices

### MainWindow
- Creates and manages the main application UI
- Handles server connection and disconnection
- Organizes the panels into tabs

### CameraPanel
- Provides UI for camera device selection and connection
- Controls camera settings (exposure, gain, binning)
- Manages image acquisition

### MountPanel
- Provides UI for mount device selection and connection
- Controls mount movements and coordinates
- Displays current mount position

### PlateSolverPanel
- Provides UI for plate solving configuration
- Manages the plate solving process
- Displays solution results

### MountModelPanel
- Manages the creation of pointing models
- Collects and organizes model points
- Displays model statistics

### ImagePanel
- Displays and manipulates captured images
- Provides zoom and save functionality

## Building the Project

The project uses CMake as its build system. The CMakeLists.txt file handles:
- Finding required dependencies (Qt, INDI, CFITSIO)
- Setting up include directories
- Defining source files and executable targets
- Configuring installation rules
- Optional package creation

## External Dependencies

1. **Qt5**: For the GUI
2. **INDI Library**: For communicating with astronomy devices
3. **libsolver**: For plate solving functionality
4. **Ekos align**: For mount modeling algorithms
5. **CFITSIO**: For handling FITS image files

## Notes for Contributors

- All UI classes follow a similar structure with setupUI() methods
- Signal/slot connections are typically set up in the setupUI() methods
- The INDI client implementation follows INDI's base client architecture
- Mock implementations are used for libsolver and Ekos align in this example