# sunar

A simplest unarchiver with encoding settings supported.

## Build

### Install Boost for MSVC

```bash
C:
cd \
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

vcpkg integrate install

vcpkg install boost-program-options[core]:x64-windows-static
vcpkg install boost-filesystem[core]:x64-windows-static
vcpkg install boost-process[core]:x64-windows-static
```

### Build with CMake

```bash
cmake.exe -G Ninja -DCMAKE_BUILD_TYPE=RelWithDeb -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -S . -B build
cmake.exe --build build
```

## Usage

```
sunar.exe -e GB2312 -o C:\path\to\output\directory example.zip
```