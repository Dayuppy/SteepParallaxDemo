# CRITICAL FIX FOR NVIDIA CRASH (nvoglv32.dll)

## The Problem
The existing `SteepParallaxGLSL.exe` file is from an older version and will crash after ~10 seconds with:
```
Unhandled exception at 0x53C88E89 (nvoglv32.dll) in SteepParallaxGLSL.exe: Fatal program exit requested.
```

## The Solution
The crash has been fixed in the enhanced version, but you need to rebuild the executable.

### For Windows Users:

1. **DELETE the old executable:**
   ```
   del SteepParallaxGLSL.exe
   ```

2. **Open the project in Visual Studio:**
   - Open `SteepParallaxGLSL.sln` in Visual Studio
   - Make sure the project is set to compile `main_enhanced.cpp` (it should be by default)

3. **Rebuild the project:**
   - Build > Rebuild Solution
   - This will create a new, stable executable

4. **Run the new version:**
   - The new executable will be crash-free and include 30+ advanced features

### For Linux Users:

1. **Install dependencies:**
   ```bash
   make install-deps
   ```

2. **Build the demo:**
   ```bash
   make clean
   make
   ```

3. **Run:**
   ```bash
   ./SteepParallaxDemo
   ```

## What Was Fixed

The crash was caused by several precision and driver stability issues:

1. **Performance counter overflow** - Frame counts now reset every 1000 frames instead of 10000
2. **OpenGL error accumulation** - Added comprehensive error checking and recovery
3. **GPU synchronization issues** - Added periodic `glFinish()` calls to prevent driver queue overflow
4. **Uniform precision problems** - Frame count uniform is now modulated to prevent large values
5. **State management** - Enhanced error recovery resets OpenGL state when errors occur

## Controls (Enhanced Version)

- **H** - Show help
- **Q** - Quit  
- **J** - Basic parallax shader
- **I** - Steep parallax shader
- **E** - Enhanced multi-feature shader
- **U** - Physically-based rendering shader
- **1-9,0** - Toggle various post-processing effects
- **Arrow keys** - Rotate camera

The enhanced version is stable and will not crash like the old version.