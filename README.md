# Crux - A C version of Krux

Crux is an experimental project that explores the capabilities of the ESP32-P4 while gradually porting the structure and features of [Krux](https://github.com/selfcustody/krux) into C

Along the way, we also experiment with alternative dependencies such as [libwally](https://github.com/ElementsProject/libwally-core), aiming to expand the project’s possibilities while maintaining full compatibility and interoperability with its Python predecessor.

## Build

### Cloning the Repository

This project uses git submodules. You have two options:

#### Option 1: Clone with submodules (Recommended)

When cloning the project for the first time, make sure to clone it recursively to include all submodules:

```bash
git clone --recursive https://github.com/yourusername/yourrepository.git
```

#### Option 2: Initialize submodules after cloning

If you've already cloned the repository without the `--recursive` flag, you can initialize and update the submodules with:

```bash
git submodule update --init --recursive
```

### Building the Project

Follow these instructions to install esp-idf https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32p4/get-started/index.html

After that you can build the project from the root directory with:

```bash
idf.py build
```

or flash the project to the device with:

```bash
idf.py flash
```

and if you are debuggning things you may want to run monitor too:

```bash
idf.py monitor
```

### Build Options

#### Enable/disable Auto-focus

To enable camera auto-focus, set on sdkconfig.defaults:

```
CONFIG_CAM_MOTOR_DW9714=y
CONFIG_CAMERA_OV5647_ENABLE_MOTOR_BY_GPIO0=y
```
