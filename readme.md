# INDI Test Client

An interactive C++ Qt GUI application for testing INDI astronomy drivers, with support for mount modeling and plate solving.

## Overview

This application provides a comprehensive testing platform for INDI-compatible astronomical equipment, particularly focused on testing mount and camera drivers. It integrates with libsolver for plate solving and the Ekos align library for mount modeling. However not all functions are implemented yet. Tested and partially working under MacOS, should be portable to other platforms for those skilled in the art.

## Features

- **INDI Client Interface**: Connect to an INDI server and interact with any INDI devices.
- **Camera Control Panel**: Manage camera settings, control exposure times, and capture images.
- **Mount Control Panel**: Move your telescope mount, sync coordinates, and perform parking/homing operations.
- **Plate Solving Panel**: Analyze captured images using libsolver to determine exact sky coordinates.
- **Mount Modeling Panel**: Create and manage pointing models to improve mount accuracy using Ekos align library.
- **Image Display Panel**: View and analyze captured images.

## Requirements

- Qt 5.12 or later
- INDI Library 1.9.0 or later
- libsolver
- Ekos align library
- CFitsIO

## Building

### Prerequisites

First, install the required dependencies:

```bash
# On Ubuntu/Debian
sudo apt-get install build-essential cmake qt5-default
sudo apt-get install libindi-dev libcfitsio-dev
sudo apt-get install indi-full # Optional: To get the INDI server and drivers
```

For macOS with Homebrew:

```bash
brew install cmake qt5 
brew install libindi cfitsio
```

### Compilation

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

1. Start your INDI server:
   ```bash
   indiserver -v indi_simulator_telescope indi_simulator_ccd
   ```

2. Launch the INDI Test Client application:
   ```bash
   ./indi_test_client
   ```

3. Connect to the INDI server (default: localhost:7624)

4. Once connected, you'll see your devices listed in the respective tabs

5. Connect to your camera and mount devices to begin testing

## Testing Workflow

A typical testing workflow might include:

1. Connect to your mount and camera
2. Take an exposure with the camera
3. Plate solve the resulting image
4. Sync the mount to the solved coordinates
5. Add the point to your mount model
6. Repeat steps 2-5 at different parts of the sky to build a comprehensive model

## License

This software is released under the GNU General Public License v3.0.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
