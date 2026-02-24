# Building for Windows

## Option A — Compile directly on Windows (recommended, easiest)

### Requirements
- [Qt6](https://www.qt.io/download-qt-installer) — install the **MSVC 2022 64-bit** or **MinGW 64-bit** component
- [CMake ≥ 3.16](https://cmake.org/download/)
- Visual Studio 2022 (for MSVC) **or** MinGW-w64 bundled with Qt

### Steps (MSVC)
```bat
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2022_64"
cmake --build build --config Release
```

### Steps (MinGW bundled with Qt)
```bat
cmake -B build -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\mingw_64" ^
      -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

After building, `windeployqt` runs automatically (via CMake post-build step) and
copies all required Qt DLLs next to the `.exe`.

---

## Option B — Cross-compile from Linux with MinGW-w64

### 1. Install MinGW-w64
```bash
sudo apt install g++-mingw-w64-x86-64-posix
```

### 2. Get a Windows build of Qt6
The easiest way is via [aqtinstall](https://github.com/miurahr/aqtinstall):
```bash
pip install aqtinstall
aqt install-qt linux desktop 6.7.0 win64_mingw \
    --outputdir ~/Qt
# Qt will be at: ~/Qt/6.7.0/mingw_64/
```

### 3. Configure and build
```bash
cmake -B build-win \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake \
      -DCMAKE_PREFIX_PATH="$HOME/Qt/6.7.0/mingw_64" \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build-win -j$(nproc)
```

### 4. Collect the .exe + Qt DLLs
Copy `build-win/HitzGurutzatuak.exe` to a Windows machine, or run
`windeployqt` from a Windows Qt installation to bundle the DLLs.

---

## Assets
The `assets/` folder must be placed next to the `.exe` at runtime:
```
HitzGurutzatuak.exe
assets/
  grids/
  words/
```
