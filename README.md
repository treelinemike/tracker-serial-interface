# tracker-serial-interface
## Requirements
1. C++ serial library. Formerly in [wjwwood/serial](https://github.com/wjwwood/serial), now use [ami-iit/serial_cpp](https://github.com/ami-iit/serial_cpp)
   - In github directory: `git clone git@github.com:ami-lit/serial_cpp`
   - `cd serial_cpp`
   - `cmake -DCMAKE_BUILD_TYPE=Release -Bbuild -S. -DCMAKE_INSTALL_PREFIX=/usr/local/`
   - `cmake --build build`
   - `sudo cmake --install build`
   - Library (header and staticly linkable binary) will be automatically installed 
2. [jarro2783/cxxopts](https://github.com/jarro2783/cxxopts) (header-only library)
   - In github directory: `git clone git@github.com:jarro2783/cxxopts`
   - Create `/usr/local/include/cxxopts`
   - Copy `cxxopts.hpp` from `cxxopts/include` to `/usr/local/include/cxxopts`
3. NDI Common API (conveniently available in private repo [treelinemike/ndicapi](https://github.com/treelinemike/ndicapi)
   - In github directory: `git clone git@github.com:treelinemike/ndicapi`
   - Install [gstreamer](https://gstreamer.freedesktop.org/)
     - In Ubuntu 24.04 ([ref](https://idroot.us/install-gstreamer-ubuntu-24-04/)): `sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`
   - Starting in `github/ndicapi` directory:
     - `make`
     - `cd library/include`
     - `sudo mkdir /usr/local/include/ndicapi`
     - `sudo cp *.h /usr/local/include/ndicapi`
     - `cd ~/github/ndicapi/build/linux/obj/library/src/`
     - `ar rc libndicapi.a *.o` to create archive
     - `ranlib libndicapi.a` to index archive
     - `sudo cp -p libndicapi.a /usr/local/lib/`
     - `sudo ldconfig -v` to update linker
## Building
   - `make clean`
   - `make`
