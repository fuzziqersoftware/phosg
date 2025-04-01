# phosg

phosg is a basic C++ wrapper library around some common C routines.

A short summary of its contents:
* Byteswapping and encoding functions (base64, rot13)
* Integer types with explicit endianness and transparent byteswapping
* Directory listing, smart-pointer fopen and stat, file and path manipulation
* Hash functions (fnv1a64, fnv1a32, sha1, sha256)
* Basic image manipulation/drawing
* JSON (de)serialization
* Network helpers (IP address parsing/formatting, socket listen and connect functions)
* Functions for getting random data from the OS
* Process utilities (list processes, name <> PID mapping, subprocess execution)
* Time conversions
* std::string helpers like printf, split/join, time and size formatting, etc.
* 2D, 3D, and 4D vectors and basic vector math
* KD-tree and LRU set data structures

This project also includes a few simple executables:
* **jsonformat**: Parses the input JSON and either minimizes it (with --compress) or reformats it for human readability (with --format).
* **bindiff**: Shows the differing bytes between two binary files in a colored hex/ASCII view. This just does a direct comparison of the two files byte for byte; it doesn't run any e.g. edit-distance algorithm (yet).
* **parse-data**: Parses the data format used by `phosg::parse_data_string` and outputs the result.
* **phosg-png-conv**: Converts the input image (in any format that `phosg::Image` can load) to a PNG image.

## Building

Building & installing on macOS, Linux, or Windows (in a Cygwin bash shell):
* Install CMake and zlib (zlib-devel on Cygwin).
* `cmake .`
* `make`
* `make test`
* `sudo make install`

Some of the tests exercise rarely-used and platform-specific parts of the library, and may fail in less-common environments. If you encounter issues, try building with `cmake . -DPHOSG_SKIP_PROCESS_TEST=1`.

The Windows build does not have continuous integration, so I may accidentally break it and not know for a while. Please file a GitHub issue if it doesn't work.
