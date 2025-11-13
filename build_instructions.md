# Building the INDI Test Client with StellarSolver Integration

The INDI Test Client now integrates the StellarSolver library for plate solving functionality. Here are the updated build instructions.

## Prerequisites

### Core Dependencies
- Qt 6 (Core, Widgets)
- INDI Library (libindi)
- CFITSIO
- StellarSolver library

### Installation on macOS with Homebrew

```bash
# Install Qt6, INDI, and CFITSIO
brew install qt6 indi libindi cfitsio

# Install StellarSolver dependencies
brew install gsl wcslib pkgconfig

# Install Astrometry.net index files
brew install astrometry-net
```

### Building StellarSolver (if not available via package manager)

StellarSolver needs to be built and installed separately as follows:

```bash
# Clone the repository
git clone https://github.com/rlancaste/stellarsolver.git
cd stellarsolver

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..

# Build and install
make -j4
sudo make install
```

## Building the INDI Test Client

1. Clone the project repository:
   ```bash
   git clone <repository_url>
   cd indi-test-client
   ```

2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```

3. Configure with CMake:
   ```bash
   cmake ..
   ```

4. Build the project:
   ```bash
   make -j4
   ```

5. Run the application:
   ```bash
   ./indi_test_client
   ```

## Customizing the Build

If you need to specify custom paths for any dependencies, you can modify the CMakeLists.txt file or pass parameters to CMake:

```bash
cmake -DCMAKE_PREFIX_PATH=/path/to/qt6 \
      -DINDI_INCLUDE_DIR=/path/to/indi/include \
      -DINDI_LIBRARIES=/path/to/indi/lib \
      -DSTELLARSOLVER_INCLUDE_DIR=/path/to/stellarsolver/include \
      -DSTELLARSOLVER_LIBRARIES=/path/to/stellarsolver/lib \
      ..
```

## Astrometry Index Files

StellarSolver requires astrometry.net index files to function properly. The program will automatically search for these files in common locations:

- `/usr/local/astrometry/data`
- `/opt/homebrew/share/astrometry`
- `/usr/local/share/astrometry`
- `/usr/share/astrometry`

If your index files are in a different location, you can specify it in the Plate Solver panel's user interface.

## Troubleshooting

- **Missing StellarSolver headers**: Make sure StellarSolver is installed and the headers are in your include path
- **Missing astrometry index files**: Install astrometry.net index files and make sure they're in one of the searched paths
- **Cannot connect to INDI server**: Make sure an INDI server is running (e.g., `indiserver -v indi_simulator_telescope indi_simulator_ccd`)
- **Plate solving failure**: Check that appropriate index files are available for your field of view range