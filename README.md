# reedkiln
The `reedkiln` library aims to provide unit testing with minimal macros.
Output follows the [Test Anything Protocol](https://testanything.org/)
for easier integration with other existing tools.

As a library that avoids macros, it therefore lacks any test creation
domain-specific language (DSL). Depending on the use case, this may
be a welcome feature or an undesirable gap.

## Goals
- Minimal C macro usage
- Test Anything Protocol support
- Optional logging support
- Cross-platform
- Small code footprint

## Build

This project uses CMake for building. Developers can obtain CMake from
the following URL:
[https://cmake.org/download/](https://cmake.org/download/)

To use CMake with this project, first create a directory to hold the build
results. Then run CMake in the directory with a path to the source code.
On UNIX, the commands would look like the following:
```
mkdir build
cd build
cmake ../reedkiln
```

Running CMake should create a build project, which then can be processed
using other tools. Usually, the tool would be Makefile or a IDE project.
For Makefiles, use the following command to build the project:
```
make
```
For IDE projects, the IDE must be installed and ready to use. Open the
project within the IDE.

Since this project's source only holds two required files (`reedkiln.h`
and `reedkiln.c`) and one optional header file (`log.h`), developers
could also use these files independently from CMake.

## License
This project uses the Unlicense, which makes the source effectively
public domain. Go to [http://unlicense.org/](http://unlicense.org/)
to learn more about the Unlicense.

Contributions to this project should likewise be provided under a
public domain dedication.
