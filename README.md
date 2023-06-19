# rtac_optix

This is a C++ wrapper around the NVIDIA OptiX library. It needs the rtac_base
package installed, a working installation of CUDA 10.2 or later and the OptiX
SDK 7.0 or later.

## Dependencies

### CUDA

If you haven't installed CUDA yet, follow instructions from
[here](https://github.com/ENSTABretagneRobotics/rtac_base).

### OptiX SDK

Download the OptiX SDK from NVIDIA developers
[website](https://developer.nvidia.com/designworks/optix/download). (You will
need a NVIDIA developer account). Find a version that matches your NVIDIA driver
version and CUDA version (see rtac_base installation).

Keep in mind that the RTAC framework was solely tested on the optix-7.3.0 SDK.
While there should not be further compatibility issue with newer OptiX version,
fall back to 7.3.0 if there are some.

Launch the downloaded script. It should generate a directory named
NVIDIA-OptiX-SDK-7.3.0-linux64-x86_64 with 3 subdirectories 'doc', 'include',
and 'SDK'.

Add a persistent environment variable 'OPTIX_PATH' pointing to the
NVIDIA-OptiX-SDK-7.3.0-linux64-x86_64 directory (for example with the following
commands)

```
echo "export OPTIX_PATH=<path_to_optix>/NVIDIA-OptiX-SDK-7.3.0-linux64-x86_64" >> $HOME/.bashrc
source $HOME/.bashrc
```

The OPTIX_PATH variable is needed for CMake to find proper header files.

### rtac_base

Follow installation instructions [here](https://github.com/ENSTABretagneRobotics/rtac_base).

You also might want to install the display utilities from
[here](https://github.com/ENSTABretagneRobotics/rtac_display)



