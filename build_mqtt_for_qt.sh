# pas de QT versie aan naar uw QT versie
# ik ga er van uit dat QT sdk op de "/home/user/" dir staat zie de "~/"

export QT_VERSION="6.8.1"

export QT_INSTALL_DIR="${HOME}/Qt"
export CMAKE_BIN_DIR="${QT_INSTALL_DIR}/Tools/CMake/bin"
export QMAKE_BIN_DIR="${QT_INSTALL_DIR}/${QT_VERSION}/gcc_64/bin"
export CMAKE_PREFIX_PATH="${QT_INSTALL_DIR}/${QT_VERSION}/gcc_64/"
export NINJA_DIR="${QT_INSTALL_DIR}/Tools/Ninja"
export PATH="${PATH}:${CMAKE_BIN_DIR}:${QMAKE_BIN_DIR}:${NINJA_DIR}"

# Create a work directory
mkdir ~/temporal && cd ~/temporal

# Clone the repository
git clone https://github.com/qt/qtmqtt.git

# Switch to the repository
cd qtmqtt

# Checkout the branch corresponding to the target kit
git checkout ${QT_VERSION}

# Create a directory to build the module cleanly
mkdir build && cd build

# Use the qt-configure-module tool
~/Qt/${QT_VERSION}/gcc_64/bin/qt-configure-module ..

# Build it here
~/Qt/Tools/CMake/bin/cmake --build .

# Install the module in the correct location
~/Qt/Tools/CMake/bin/cmake --install . --verbose
