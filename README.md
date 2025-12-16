# KERN

Kern is an experimental project that explores the capabilities of the ESP32-P4 as a platform to perform air-gapped Bitcoin signatures and cryptography. 

DIY Hardware & Programming Notes: For information about suitable DIY hardware, suggested configuration profiles, and notes on secure boot, see the DIY Guide.

## Prerequisites (Linux)

### Build dependencies

CMake and Ninja are required to build the firmware.

#### Debian / Ubuntu / Mint / Kubuntu

```bash
sudo apt update
sudo apt install -y cmake ninja-build git python3 python3-venv
```

### Python requirement

A recent Python version is required (recommended: Python >= 3.11). Older system versions may cause dependency installation failures.

Check your version:

```bash
python3 --version
```

## ESP32-P4 — ESP-IDF 5.5 Setup (Linux)

Crux requires the ESP-IDF SDK v5.5, which provides official support for ESP32-P4. For detailed SDK documentation, see the Espressif official guide: https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32p4/

### Install ESP-IDF v5.5 (isolated setup)

To avoid conflicts with other ESP-IDF versions (e.g. v5.4), the tools and Python environment must be isolated.

#### Get the ESP-IDF SDK

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git esp-idf-5.5
```

#### Isolate ESP-IDF tools (IMPORTANT)

Before installing, define a dedicated tools directory for ESP-IDF 5.5:

```bash
export IDF_TOOLS_PATH="$HOME/.espressif-idf55"
```

This prevents reuse of Python environments created for other ESP-IDF versions.

#### Install required tools for ESP32-P4

```bash
cd ~/esp/esp-idf-5.5
./install.sh esp32p4
```

This step installs:

- ESP32-P4 toolchain
- Python virtual environment
- ESP-IDF v5.5 dependency constraints

#### Activate the ESP-IDF environment

```bash
. ~/esp/esp-idf-5.5/export.sh
```

Verify installation:

```bash
idf.py --version
```

## Repository setup

This project uses git submodules. The example below uses the `~/esp` workspace created during the ESP-IDF setup.

### Clone with submodules (recommended)

```bash
cd ~/esp
git clone --recursive https://github.com/odudex/Crux.git
cd Crux
```

### Initialize submodules after cloning

If you've already cloned the repository without the `--recursive` flag, initialize and update the submodules with:

```bash
git submodule update --init --recursive
```

## Build, flash, monitor

With the ESP-IDF 5.5 environment activated and from the project root:

```bash
idf.py build
idf.py flash
idf.py monitor
```

Exit monitor with `Ctrl + ]`.

## Build options

### Enable/disable Auto-focus

To enable camera auto-focus, enable camera focus motor on menuconfig:

```
CONFIG_CAM_MOTOR_DW9714=y
CONFIG_CAMERA_OV5647_ENABLE_MOTOR_BY_GPIO0=y
```
