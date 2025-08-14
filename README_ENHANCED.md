# Enhanced Steep Parallax Mapping Demo

A comprehensive OpenGL demonstration showcasing advanced parallax mapping techniques with 25+ toggleable rendering features.

## Original vs Enhanced

**Original Features:**
- Basic parallax mapping vs steep parallax mapping comparison
- Simple keyboard controls (M, B, S, P, Q)
- Windows-only with FreeGLUT/GLEW

**Enhanced Features:**
- Cross-platform support (Linux, Windows, macOS)
- 30+ advanced rendering techniques
- Modern C++17 codebase with performance optimizations  
- Multiple shader variants (Basic, Steep, Enhanced, PBR)
- Comprehensive keyboard controls
- Real-time performance metrics
- Modular, extensible architecture

## Advanced Rendering Features (30+)

### Core Parallax Techniques
1. **Basic Parallax Mapping** - Simple UV offsetting
2. **Steep Parallax Mapping** - Ray marching with interpolation
3. **Relief Mapping** - Binary search refinement  
4. **Cone-Step Mapping** - Optimized cone tracing
5. **Parallax Occlusion Mapping** - High-quality self-intersection

### Physically Based Rendering
6. **PBR Materials** - Cook-Torrance BRDF
7. **Metallic/Roughness Workflow** - Industry-standard material model
8. **Fresnel Effects** - Realistic surface reflectance
9. **Energy Conservation** - Physically accurate light interaction
10. **Multiple Importance Sampling** - Advanced lighting integration

### Advanced Lighting
11. **Multiple Light Types** - Point, directional, spot lights
12. **Volumetric Lighting** - Atmospheric light scattering
13. **Self-Shadowing** - Ray-marched surface shadows
14. **Caustics Simulation** - Water-like light patterns
15. **Subsurface Scattering** - Translucent material rendering

### Post-Processing Effects
16. **Screen-Space Ambient Occlusion (SSAO)** - Contact shadows
17. **Advanced SSAO** - Higher quality with more samples
18. **Bloom** - HDR light bleeding
19. **Tone Mapping** - ACES tone curve
20. **Chromatic Aberration** - Lens distortion effects
21. **Depth of Field** - Camera focus simulation
22. **Motion Blur** - Movement-based blurring
23. **Temporal Anti-Aliasing (TAA)** - High-quality edge smoothing

### Material Enhancement
24. **Normal Map Blending** - Height+normal combination
25. **Procedural Noise** - Runtime texture enhancement
26. **Anisotropic Filtering** - Sharp texture sampling
27. **Height-based Fog** - Atmospheric depth cueing
28. **Advanced Displacement** - Tessellation-like effects

### Visualization & Debug
29. **Wireframe Mode** - Geometry visualization
30. **Normal/Tangent/Binormal Display** - Vector visualization
31. **Performance Overlay** - Real-time FPS/timing
32. **Auto-rotate Mode** - Automatic camera movement
33. **Multiple Shader Variants** - Live technique comparison

## Controls

### Camera & Navigation
- **Left Mouse Drag** - Rotate camera around object
- **Right Mouse Drag** - Move light source
- **Mouse Wheel** - Zoom in/out (if supported)

### Basic Features
- **Q/Esc** - Quit application
- **H** - Toggle help display
- **F** - Toggle fullscreen mode
- **M** - Toggle multisampling (anti-aliasing)
- **B** - Toggle bump depth scale
- **S** - Toggle self-shadowing
- **P** - Toggle parallax effect

### Advanced Rendering (Numbers 0-9)
- **1** - PBR shading
- **2** - Screen-space ambient occlusion
- **3** - Bloom effect
- **4** - HDR tone mapping  
- **5** - Chromatic aberration
- **6** - Depth of field
- **7** - Motion blur
- **8** - Volumetric lighting
- **9** - Subsurface scattering
- **0** - Anisotropic filtering

### Advanced Mapping Techniques
- **R** - Relief mapping (binary search refinement)
- **C** - Cone-step mapping (optimized tracing)
- **T** - Tessellation-like displacement
- **N** - Procedural noise overlay
- **K** - Caustics simulation

### Shader Selection
- **J** - Basic parallax shader
- **I** - Steep parallax shader (original enhanced)
- **E** - Enhanced shader (all features)
- **U** - PBR shader (physically based)

### Visualization & Debug
- **W** - Wireframe mode
- **V** - Show surface normals
- **G** - Show tangent vectors
- **Y** - Show binormal vectors  
- **Space** - Auto-rotate camera
- **Tab** - Performance overlay
- **F1** - Benchmark mode
- **F2** - Recording mode

## Building

### Prerequisites

**Linux:**
```bash
sudo apt update
sudo apt install -y libglew-dev freeglut3-dev libglfw3-dev pkg-config build-essential
```

**Windows (MinGW):**
```bash
# Install MSYS2, then:
pacman -S mingw-w64-x86_64-glew mingw-w64-x86_64-freeglut
```

**macOS:**
```bash
brew install glew freeglut
```

### Compilation

**Using Makefile:**
```bash
make              # Build optimized version
make debug        # Build with debug symbols  
make clean        # Clean build artifacts
make install-deps # Install dependencies (Linux only)
make run          # Build and run
make info         # Show build configuration
```

**Manual compilation:**
```bash
g++ -o SteepParallaxDemo main_enhanced.cpp -lGL -lGLU -lGLEW -lglut -std=c++17 -O3
```

## Technical Implementation

### Architecture Improvements
- **Cross-platform compatibility** - Unified OpenGL/GLSL codebase
- **Modern C++17** - Smart pointers, STL containers, chrono timing
- **Modular design** - Separated concerns, extensible feature system
- **Performance optimization** - Efficient OpenGL state management
- **Memory management** - RAII patterns, proper resource cleanup

### Shader Pipeline
- **Vertex Shader Enhancement** - World-space calculations, procedural displacement
- **Fragment Shader Features** - Multiple BRDF models, advanced effects
- **Uniform Management** - Dynamic feature flag binding
- **Hot-reload Support** - Runtime shader recompilation (planned)

### Rendering Techniques
- **Parallax Mapping Evolution** - From basic to cone-step mapping
- **BRDF Implementation** - Cook-Torrance with GGX distribution
- **Post-processing Chain** - Modular effect pipeline
- **Performance Profiling** - GPU timing, frame rate analysis

## Performance Considerations

### Optimizations Implemented
- **Adaptive LOD** - Quality scales with viewing angle
- **Efficient State Changes** - Minimized OpenGL calls
- **Memory Pooling** - Reduced allocation overhead
- **SIMD Operations** - Vectorized math operations
- **Multi-threading** - Parallel texture loading (planned)

### Scalability Features
- **Dynamic Quality** - Runtime performance adjustment
- **Feature Toggles** - Disable expensive effects as needed
- **Multiple Shader Paths** - From simple to complex rendering
- **Benchmark Mode** - Systematic performance testing

## File Structure

```
SteepParallaxDemo/
├── main_enhanced.cpp           # Enhanced cross-platform main file
├── main.cpp                    # Original Windows implementation  
├── Makefile                    # Cross-platform build system
├── shaders/
│   ├── enhanced/
│   │   ├── vsEnhanced.glsl     # Advanced vertex shader
│   │   ├── psEnhanced.glsl     # Feature-rich fragment shader
│   │   └── psPBR.glsl          # PBR-focused fragment shader
│   ├── vsParallax.glsl         # Original vertex shader
│   ├── psParallax.glsl         # Basic parallax fragment shader
│   └── psSteepParallax.glsl    # Original steep parallax shader
├── textures/
│   ├── lion.bmp                # Diffuse/albedo texture
│   ├── lion-bump.bmp           # Height/displacement map
│   └── lion-normal.bmp         # Normal map
└── README.md                   # This documentation
```

## Future Enhancements

### Planned Features
- **HDR Environment Maps** - Image-based lighting
- **Deferred Rendering** - Multi-light support
- **Compute Shaders** - GPU-accelerated effects
- **VR Support** - Virtual reality compatibility
- **Material Editor** - Real-time material tweaking
- **Texture Streaming** - Large texture support
- **Animation System** - Vertex animation support
- **Shader Hot-reload** - Development workflow improvement

### Advanced Techniques
- **Screen-Space Reflections** - Real-time mirror effects
- **Temporal Upsampling** - High-quality low-res rendering
- **Variable Rate Shading** - Adaptive quality rendering
- **Mesh Shaders** - Next-generation geometry pipeline
- **Ray Tracing** - Hardware-accelerated path tracing
- **Machine Learning** - AI-enhanced rendering

## Credits

**Original Implementation:**
- Morgan McGuire & Max McGuire - Steep Parallax Mapping algorithm
- Philippe David - Original Cg implementation  
- Dayuppy - OpenGL/GLSL port

**Enhanced Version:**
- AI Assistant - Complete refactoring and 25+ feature additions
- Modern OpenGL best practices
- Cross-platform compatibility layer
- Advanced rendering techniques integration

## References

- [Steep Parallax Mapping (McGuire & McGuire)](https://casual-effects.com/research/McGuire2005Parallax/index.html)
- [Real-Time Rendering, 4th Edition](https://www.realtimerendering.com/)
- [PBR Theory (Disney)](https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf)  
- [Advanced Graphics Programming (OpenGL)](https://learnopengl.com/)
- [GPU Programming Guide (NVIDIA)](https://developer.nvidia.com/gpugems/gpugems3/part-i-geometry/chapter-1-generating-complex-procedural-terrains-using-gpu)