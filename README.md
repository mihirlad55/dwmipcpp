# dwmipcpp
![CI](https://github.com/mihirlad55/dwmipcpp/workflows/CI/badge.svg)

dwmipcpp is a C++ client library for communicating with an IPC-patched dwm. This
library requires that dwm have the IPC patch which can be found
[here](https://github.com/mihirlad55/dwm-ipc). This library contains a very
simple interface for sending all the message types implemented in the IPC patch.
It also supports subscribing to and listening for events.


## Requirements
To build dwmipcpp, you will need:
- `cmake (>= 3.0)` to build the library
- `jsoncpp (>= 1.7.2)` for generating and parsing IPC messages
- C+11 compiler to compile the library


## Building the Library
To use the library, add the following to your project `CMakeLists.txt`:
```cmake
add_subdirectory(dwmipcpp)
include_directories(${DWMIPCPP_INCLUDE_DIRS})
include_directories(${DWMIPCPP_LIBRARY_DIRS})

target_link_libraries(<your project> ${DWMIPCPP_LIBRARIES})
```


## Documentation
The library is thoroughly documented using Doxygen. The documentation can be
found [here](https://mihirlad55.github.io/dwmipcpp). To use the library, you
only need to worry about the `Connection`
[class](https://mihirlad55.github.io/dwmipcpp/classdwmipc_1_1Connection.html).


## Examples
Some example code for interacting with the library can be found in
[examples/](https://github.com/mihirlad55/dwmipcpp/tree/master/examples).

To run the examples, you can execute the `build.sh` script in the project
directory. The example executables will be located in `build/examples/`.


## Related Projects
See the [dwm IPC patch](https://github.com/mihirlad55/dwm-ipc)

See the [dwm polybar module \[WIP\]](https://github.com/mihirlad55/polybar)


## Credits
Special thanks to the authors of [i3ipcpp](https://github.com/drmgc/i3ipcpp)
whose code structure was adapted for this library.
