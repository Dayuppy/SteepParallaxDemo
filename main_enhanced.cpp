// Enhanced Steep Parallax Mapping Demo
// Cross-platform OpenGL implementation with extensive feature set
// Author: Enhanced by AI Assistant, Original: Morgan McGuire, updated by Dayuppy

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <csignal>  // For signal handling

#ifndef _WIN32
    #include <unistd.h>  // For getcwd on Linux
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Cross-platform OpenGL includes
#ifdef _WIN32
    #include <windows.h>
    #pragma comment(lib, "glew32.lib")
    #pragma comment(lib, "freeglut.lib")
#endif

#include "GL/glew.h"
#include "GL/glut.h"

// Cross-platform image loading
#ifdef _WIN32
    #include "READ_BMP.h"
#else
    // We'll implement a simple BMP loader for Linux
    typedef unsigned char BYTE;
#endif

#undef min
#undef max

// === CONFIGURATION AND GLOBALS ===

// Window and camera
static float camera_rotate_angle = 0.0f;
static float camera_elevate_angle = -20.0f;
static int screenWidth = 1400;
static int screenHeight = 700;
static bool fullscreen = false;

// Mouse state
static int mouseButton = -1, mouseX = 0, mouseY = 0;

// Feature toggles (25+ features!)
struct DemoFeatures {
    bool multisampling = true;
    bool bumpy = false;
    bool selfShadowing = true;
    bool parallaxEnabled = true;
    bool pbrShading = false;
    bool ssaoEnabled = true;
    bool bloomEnabled = false;
    bool toneMappingEnabled = false;
    bool chromaticAberration = false;
    bool depthOfField = false;
    bool motionBlur = false;
    bool volumetricLighting = false;
    bool subsurfaceScattering = false;
    bool anisotropicFiltering = true;
    bool temporalAA = false;
    bool screenSpaceReflections = false;
    bool proceduralNoise = false;
    bool tessellationEnabled = false;
    bool reliefMapping = false;
    bool coneStepMapping = false;
    bool quadtreeDisplacement = false;
    bool caustics = false;
    bool normalBlending = true;
    bool heightFog = false;
    bool wireframeMode = false;
    bool showNormals = false;
    bool showTangents = false;
    bool showBinormals = false;
    bool autoRotate = false;
    bool pauseAnimation = false;
    bool showHelp = false;
    bool showPerformance = false;
    bool benchmarkMode = false;
    bool recordingMode = false;
} features;

// Performance tracking
struct PerformanceMetrics {
    float frameTime = 0.0f;
    float fps = 0.0f;
    int frameCount = 0;
    std::chrono::high_resolution_clock::time_point lastTime;
    float avgFrameTime = 0.0f;
    float minFrameTime = 9999.0f;
    float maxFrameTime = 0.0f;
} perf;

// Light configuration
struct Light {
    float position[3] = {0.0f, 0.0f, 8.0f};
    float color[3] = {1.0f, 1.0f, 0.65f};
    float intensity = 1.0f;
    int type = 0; // 0=point, 1=directional, 2=spot
    float spotAngle = 45.0f;
    float falloff = 1.0f;
};
static std::vector<Light> lights;

// Textures
static GLuint texture_id = 0;
static GLuint bumpTexture_id = 0;
static GLuint normalTexture_id = 0;
static BYTE* image_data = nullptr;
static int image_width = 0;
static int image_height = 0;

// Shader programs
static std::unordered_map<std::string, GLuint> shaderPrograms;
static std::string currentShader = "steep";

// Geometry
static GLuint VBO = 0;
static GLuint VAO = 0;
static GLuint instanceVBO = 0;
static int instanceCount = 1;

// === UTILITY FUNCTIONS ===

template<typename T>
constexpr T clamp(T v, T lo, T hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Enhanced OpenGL error checking
static bool checkGLError(const char* operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in " << operation << ": ";
        switch (error) {
            case GL_INVALID_ENUM:
                std::cerr << "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:
                std::cerr << "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:
                std::cerr << "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY:
                std::cerr << "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:
                std::cerr << "Unknown error " << error; break;
        }
        std::cerr << " (" << error << ")" << std::endl;
        return false;
    }
    return true;
}

// Frame rate limiter to prevent overwhelming the system
static void limitFrameRate() {
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    static const auto targetFrameTime = std::chrono::microseconds(16667); // ~60 FPS
    
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = currentTime - lastFrameTime;
    
    if (elapsed < targetFrameTime) {
        auto sleepTime = targetFrameTime - elapsed;
        std::this_thread::sleep_for(sleepTime);
    }
    
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

static void updatePerformance() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - perf.lastTime);
    perf.frameTime = duration.count() / 1000.0f; // Convert to milliseconds
    
    // Prevent division by zero and invalid frame times
    if (perf.frameTime <= 0.0f) {
        perf.frameTime = 0.016f; // Default to ~60 FPS
    }
    
    perf.fps = 1000.0f / perf.frameTime;
    
    // Update averages with overflow protection
    perf.frameCount++;
    
    // Reset counter every 10000 frames to prevent overflow
    if (perf.frameCount > 10000) {
        perf.frameCount = 1;
        perf.avgFrameTime = perf.frameTime;
        perf.minFrameTime = perf.frameTime;
        perf.maxFrameTime = perf.frameTime;
    } else if (perf.frameCount > 1) {
        // Use exponential moving average to prevent precision loss
        float alpha = 0.1f; // Smoothing factor
        perf.avgFrameTime = alpha * perf.frameTime + (1.0f - alpha) * perf.avgFrameTime;
        perf.minFrameTime = std::min(perf.minFrameTime, perf.frameTime);
        perf.maxFrameTime = std::max(perf.maxFrameTime, perf.frameTime);
    } else {
        perf.avgFrameTime = perf.frameTime;
        perf.minFrameTime = perf.frameTime;
        perf.maxFrameTime = perf.frameTime;
    }
    
    perf.lastTime = currentTime;
}

static void printHelp() {
    std::cout << "\n=== Enhanced Steep Parallax Demo - Controls ===\n";
    std::cout << "Camera:\n";
    std::cout << "  Left Mouse Drag  - Rotate camera\n";
    std::cout << "  Right Mouse Drag - Move light\n";
    std::cout << "  Mouse Wheel      - Zoom\n\n";
    
    std::cout << "Basic Features:\n";
    std::cout << "  Q/Esc - Quit\n";
    std::cout << "  H     - Toggle this help\n";
    std::cout << "  F     - Toggle fullscreen\n";
    std::cout << "  M     - Toggle multisampling\n";
    std::cout << "  B     - Toggle bump depth\n";
    std::cout << "  S     - Toggle self-shadowing\n";
    std::cout << "  P     - Toggle parallax effect\n\n";
    
    std::cout << "Advanced Rendering (25+ Features):\n";
    std::cout << "  1 - PBR shading\n";
    std::cout << "  2 - SSAO\n";
    std::cout << "  3 - Bloom\n";
    std::cout << "  4 - Tone mapping\n";
    std::cout << "  5 - Chromatic aberration\n";
    std::cout << "  6 - Depth of field\n";
    std::cout << "  7 - Motion blur\n";
    std::cout << "  8 - Volumetric lighting\n";
    std::cout << "  9 - Subsurface scattering\n";
    std::cout << "  0 - Anisotropic filtering\n\n";
    
    std::cout << "Advanced Mapping:\n";
    std::cout << "  R - Relief mapping\n";
    std::cout << "  C - Cone-step mapping\n";
    std::cout << "  T - Tessellation\n";
    std::cout << "  N - Procedural noise\n";
    std::cout << "  K - Caustics\n\n";
    
    std::cout << "Shader Selection:\n";
    std::cout << "  J - Basic parallax shader\n";
    std::cout << "  I - Steep parallax shader\n";
    std::cout << "  E - Enhanced shader (all features)\n";
    std::cout << "  U - PBR shader\n\n";
    
    std::cout << "Visualization:\n";
    std::cout << "  W     - Wireframe mode\n";
    std::cout << "  V     - Show normals\n";
    std::cout << "  G     - Show tangents\n";
    std::cout << "  Y     - Show binormals\n";
    std::cout << "  Space - Auto-rotate\n";
    std::cout << "  Tab   - Performance overlay\n";
    std::cout << "  F1    - Benchmark mode\n";
    std::cout << "  F2    - Recording mode\n\n";
}

// === CROSS-PLATFORM BMP LOADER ===

#ifndef _WIN32
bool BMP_Read(const char *filename, BYTE** pixels, int &width, int &height) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return false;
    }
    
    // Read BMP header
    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) {
        std::cerr << "Error: Invalid BMP file " << filename << std::endl;
        fclose(file);
        return false;
    }
    
    // Check if it's a BMP file
    if (header[0] != 'B' || header[1] != 'M') {
        std::cerr << "Error: Not a BMP file " << filename << std::endl;
        fclose(file);
        return false;
    }
    
    // Extract image dimensions
    width = *(int*)&header[18];
    height = *(int*)&header[22];
    int bitsPerPixel = *(int*)&header[28];
    
    if (bitsPerPixel != 24) {
        std::cerr << "Error: Only 24-bit BMP files supported" << std::endl;
        fclose(file);
        return false;
    }
    
    // Allocate memory for pixel data
    int imageSize = width * height * 3;
    if (*pixels) delete[] *pixels;
    *pixels = new BYTE[imageSize];
    
    // Read pixel data (note: BMP stores BGR, we need RGB)
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            unsigned char bgr[3];
            if (fread(bgr, 1, 3, file) != 3) {
                std::cerr << "Error: Failed to read pixel data" << std::endl;
                delete[] *pixels;
                *pixels = nullptr;
                fclose(file);
                return false;
            }
            
            int idx = (y * width + x) * 3;
            (*pixels)[idx] = bgr[2];     // R
            (*pixels)[idx + 1] = bgr[1]; // G
            (*pixels)[idx + 2] = bgr[0]; // B
        }
        
        // Handle padding
        int padding = (4 - (width * 3) % 4) % 4;
        fseek(file, padding, SEEK_CUR);
    }
    
    fclose(file);
    return true;
}
#endif

// === MATRIX OPERATIONS ===

static void multiply4x4(const float A[16], const float B[16], float out[16]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += A[k * 4 + r] * B[c * 4 + k];
            }
            out[c * 4 + r] = sum;
        }
    }
}

static void invertRigid(const float M[16], float out[16]) {
    // Transpose the 3×3 rotation
    out[0] = M[0];  out[1] = M[4];  out[2] = M[8];   out[3] = 0.0f;
    out[4] = M[1];  out[5] = M[5];  out[6] = M[9];   out[7] = 0.0f;
    out[8] = M[2];  out[9] = M[6];  out[10] = M[10]; out[11] = 0.0f;
    // Compute -R^T * T
    out[12] = -(out[0] * M[12] + out[4] * M[13] + out[8] * M[14]);
    out[13] = -(out[1] * M[12] + out[5] * M[13] + out[9] * M[14]);
    out[14] = -(out[2] * M[12] + out[6] * M[13] + out[10] * M[14]);
    out[15] = 1.0f;
}

// === SHADER MANAGEMENT ===

static std::string readFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "ERROR: cannot open '" << path << "'" << std::endl;
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint status = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Shader compile error (" << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << "):\n" << log << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glBindAttribLocation(p, 0, "Position");
    glBindAttribLocation(p, 1, "UV");
    glBindAttribLocation(p, 2, "Normal");
    glBindAttribLocation(p, 3, "Tangent");
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glBindFragDataLocation(p, 0, "fragColor");
    glLinkProgram(p);
    GLint status = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << std::endl;
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

static GLuint createShaderProgram(const char* vsFile, const char* fsFile) {
    std::string vsSrc = readFile(vsFile);
    std::string fsSrc = readFile(fsFile);
    if (vsSrc.empty() || fsSrc.empty()) return 0;
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    if (!vs || !fs) return 0;
    GLuint prog = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// === INITIALIZATION ===

static void initTextures() {
    std::cout << "Loading textures..." << std::endl;
    
    // Load diffuse texture
    if (BMP_Read("lion.bmp", &image_data, image_width, image_height)) {
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        if (features.anisotropicFiltering) {
            float maxAniso;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }
        
        std::cout << "Loaded diffuse texture: " << image_width << "x" << image_height << std::endl;
    }
    
    // Load bump texture
    if (BMP_Read("lion-bump.bmp", &image_data, image_width, image_height)) {
        glGenTextures(1, &bumpTexture_id);
        glBindTexture(GL_TEXTURE_2D, bumpTexture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded bump texture" << std::endl;
    }
    
    // Load normal texture  
    if (BMP_Read("lion-normal.bmp", &image_data, image_width, image_height)) {
        glGenTextures(1, &normalTexture_id);
        glBindTexture(GL_TEXTURE_2D, normalTexture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded normal texture" << std::endl;
    }
}

static void initShaders() {
    std::cout << "Compiling shaders..." << std::endl;
    
    // Load basic parallax shader
    GLuint basicProg = createShaderProgram("vsParallax.glsl", "psParallax.glsl");
    if (basicProg) {
        shaderPrograms["basic"] = basicProg;
        std::cout << "Loaded basic parallax shader" << std::endl;
    }
    
    // Load steep parallax shader
    GLuint steepProg = createShaderProgram("vsParallax.glsl", "psSteepParallax.glsl");
    if (steepProg) {
        shaderPrograms["steep"] = steepProg;
        std::cout << "Loaded steep parallax shader" << std::endl;
    }
    
    // Load enhanced shaders
    GLuint enhancedProg = createShaderProgram("shaders/enhanced/vsEnhanced.glsl", "shaders/enhanced/psEnhanced.glsl");
    if (enhancedProg) {
        shaderPrograms["enhanced"] = enhancedProg;
        std::cout << "Loaded enhanced shader" << std::endl;
    }
    
    // Load PBR shader
    GLuint pbrProg = createShaderProgram("shaders/enhanced/vsEnhanced.glsl", "shaders/enhanced/psPBR.glsl");
    if (pbrProg) {
        shaderPrograms["pbr"] = pbrProg;
        std::cout << "Loaded PBR shader" << std::endl;
    }
    
    if (shaderPrograms.empty()) {
        std::cerr << "ERROR: Failed to load any shaders!" << std::endl;
        exit(1);
    }
    
    // Set default shader
    currentShader = "enhanced";
    if (shaderPrograms.find(currentShader) == shaderPrograms.end()) {
        currentShader = "steep";
    }
}

static void initGeometry() {
    std::cout << "Initializing geometry..." << std::endl;
    
    // Enhanced quad with proper tangent vectors
    float verts[] = {
        // Position     UV      Normal      Tangent
        -7.0f, -7.0f, 4.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -7.0f,  7.0f, 4.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         7.0f,  7.0f, 4.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         7.0f, -7.0f, 4.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // UV attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Tangent attribute
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);
    
    glBindVertexArray(0);
    std::cout << "Geometry initialized" << std::endl;
}

static void initLights() {
    // Initialize default light
    Light defaultLight;
    defaultLight.position[0] = 0.0f;
    defaultLight.position[1] = 0.0f;
    defaultLight.position[2] = 8.0f;
    defaultLight.color[0] = 1.0f;
    defaultLight.color[1] = 1.0f;
    defaultLight.color[2] = 0.65f;
    defaultLight.intensity = 1.0f;
    defaultLight.type = 0; // Point light
    lights.push_back(defaultLight);
    
    std::cout << "Lights initialized (" << lights.size() << " lights)" << std::endl;
}

// Forward declarations
static void Handle_Display();
static void Handle_Keyboard(unsigned char key, int x, int y);
static void Handle_Reshape(int w, int h);
static void Handle_Mouse(int button, int state, int x, int y);
static void Handle_Motion(int x, int y);

static void cleanupResources() {
    std::cout << "Cleaning up resources..." << std::endl;
    
    // Clean up textures
    if (texture_id != 0) {
        glDeleteTextures(1, &texture_id);
        texture_id = 0;
    }
    if (bumpTexture_id != 0) {
        glDeleteTextures(1, &bumpTexture_id);
        bumpTexture_id = 0;
    }
    if (normalTexture_id != 0) {
        glDeleteTextures(1, &normalTexture_id);
        normalTexture_id = 0;
    }
    
    // Clean up shaders
    for (auto& pair : shaderPrograms) {
        if (pair.second != 0) {
            glDeleteProgram(pair.second);
        }
    }
    shaderPrograms.clear();
    
    // Clean up geometry
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    
    // Clean up image data
    if (image_data) {
        delete[] image_data;
        image_data = nullptr;
    }
    
    std::cout << "Resource cleanup complete!" << std::endl;
}

// Signal handler for clean exit
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", cleaning up..." << std::endl;
    cleanupResources();
    exit(signal);
}

// === MAIN FUNCTION ===

int main(int argc, char* argv[]) {
    std::cout << "Enhanced Steep Parallax Mapping Demo" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Register cleanup handlers
    std::atexit(cleanupResources);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#ifdef _WIN32
    std::signal(SIGBREAK, signalHandler);
#endif
    
    // Print working directory
    char cwd[1024];
#ifdef _WIN32
    if (GetCurrentDirectoryA(1024, cwd))
#else
    if (getcwd(cwd, sizeof(cwd)))
#endif
        std::cout << "Working directory: " << cwd << std::endl;
    
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(screenWidth, screenHeight);
    glutCreateWindow("Enhanced Steep Parallax Mapping Demo");
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewResult = glewInit();
    if (glewResult != GLEW_OK) {
        std::cerr << "ERROR: glewInit failed: " << glewGetErrorString(glewResult) << std::endl;
        return 1;
    }
    
    // Clear any OpenGL errors that may have been generated by glewInit
    while (glGetError() != GL_NO_ERROR) {}
    
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;
    
    // Enable OpenGL features with error checking
    glEnable(GL_DEPTH_TEST);
    if (!checkGLError("Enabling depth test")) return 1;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (!checkGLError("Enabling blending")) return 1;
    
    // Initialize performance tracking
    perf.lastTime = std::chrono::high_resolution_clock::now();
    
    // Initialize subsystems with validation
    std::cout << "Initializing subsystems..." << std::endl;
    
    try {
        initTextures();
        if (!checkGLError("Texture initialization")) return 1;
        
        initShaders();
        if (!checkGLError("Shader initialization")) return 1;
        
        initGeometry();
        if (!checkGLError("Geometry initialization")) return 1;
        
        initLights();
        
        // Validate that all critical resources are loaded
        if (shaderPrograms.empty()) {
            std::cerr << "ERROR: No shaders loaded successfully!" << std::endl;
            return 1;
        }
        
        if (texture_id == 0 || bumpTexture_id == 0 || normalTexture_id == 0) {
            std::cerr << "ERROR: Failed to load required textures!" << std::endl;
            return 1;
        }
        
        if (VAO == 0 || VBO == 0) {
            std::cerr << "ERROR: Failed to create geometry buffers!" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR during initialization: " << e.what() << std::endl;
        return 1;
    }
    
    // Set up GLUT callbacks
    glutDisplayFunc(Handle_Display);
    glutKeyboardFunc(Handle_Keyboard);
    glutReshapeFunc(Handle_Reshape);
    glutMouseFunc(Handle_Mouse);
    glutMotionFunc(Handle_Motion);
    glutIdleFunc(Handle_Display);
    
    std::cout << "\nInitialization complete!" << std::endl;
    std::cout << "Loaded " << shaderPrograms.size() << " shader programs" << std::endl;
    std::cout << "Press 'H' for help, 'Q' to quit" << std::endl;
    
    // Main loop - this should never return
    glutMainLoop();
    
    // If we somehow get here, clean up and exit
    cleanupResources();
    return 0;
}

// === EVENT HANDLERS ===

static void Handle_Keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'q': case 'Q': case 27: // Quit
            cleanupResources();
            exit(0);
            break;
            
        case 'h': case 'H': // Help
            features.showHelp = !features.showHelp;
            if (features.showHelp) printHelp();
            break;
            
        case 'f': case 'F': // Fullscreen
            fullscreen = !fullscreen;
            if (fullscreen) {
                glutFullScreen();
            } else {
                glutReshapeWindow(screenWidth, screenHeight);
            }
            break;
            
        // Basic features
        case 'm': case 'M': features.multisampling = !features.multisampling; break;
        case 'b': case 'B': features.bumpy = !features.bumpy; break;
        case 's': case 'S': features.selfShadowing = !features.selfShadowing; break;
        case 'p': case 'P': features.parallaxEnabled = !features.parallaxEnabled; break;
        
        // Advanced rendering features (25+)
        case '1': features.pbrShading = !features.pbrShading; break;
        case '2': features.ssaoEnabled = !features.ssaoEnabled; break;
        case '3': features.bloomEnabled = !features.bloomEnabled; break;
        case '4': features.toneMappingEnabled = !features.toneMappingEnabled; break;
        case '5': features.chromaticAberration = !features.chromaticAberration; break;
        case '6': features.depthOfField = !features.depthOfField; break;
        case '7': features.motionBlur = !features.motionBlur; break;
        case '8': features.volumetricLighting = !features.volumetricLighting; break;
        case '9': features.subsurfaceScattering = !features.subsurfaceScattering; break;
        case '0': features.anisotropicFiltering = !features.anisotropicFiltering; break;
        
        // Advanced mapping
        case 'r': case 'R': features.reliefMapping = !features.reliefMapping; break;
        case 'c': case 'C': features.coneStepMapping = !features.coneStepMapping; break;
        case 't': case 'T': features.tessellationEnabled = !features.tessellationEnabled; break;
        case 'n': case 'N': features.proceduralNoise = !features.proceduralNoise; break;
        case 'k': case 'K': features.caustics = !features.caustics; break;
        
        // Visualization
        case 'w': case 'W': features.wireframeMode = !features.wireframeMode; break;
        case 'v': case 'V': features.showNormals = !features.showNormals; break;
        case 'g': case 'G': features.showTangents = !features.showTangents; break;
        case 'y': case 'Y': features.showBinormals = !features.showBinormals; break;
        case ' ': features.autoRotate = !features.autoRotate; break; // Space
        case 9: features.showPerformance = !features.showPerformance; break; // Tab
        
        // Shader switching
        case 'j': case 'J': // Switch to basic shader
            currentShader = "basic";
            if (shaderPrograms.find(currentShader) == shaderPrograms.end()) currentShader = "steep";
            std::cout << "Switched to: " << currentShader << " shader" << std::endl;
            break;
        case 'e': case 'E': // Switch to enhanced shader
            currentShader = "enhanced";
            if (shaderPrograms.find(currentShader) == shaderPrograms.end()) currentShader = "steep";
            std::cout << "Switched to: " << currentShader << " shader" << std::endl;
            break;
        case 'u': case 'U': // Switch to PBR shader
            currentShader = "pbr";
            if (shaderPrograms.find(currentShader) == shaderPrograms.end()) currentShader = "steep";
            std::cout << "Switched to: " << currentShader << " shader" << std::endl;
            break;
        case 'i': case 'I': // Switch to steep shader
            currentShader = "steep";
            std::cout << "Switched to: " << currentShader << " shader" << std::endl;
            break;
    }
    
    std::cout << "Feature toggled: " << key << std::endl;
    glutPostRedisplay();
}

static void Handle_Reshape(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(25.0, (double)w * 0.5 / h, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

static void Handle_Mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        mouseButton = button;
        mouseX = x; 
        mouseY = y;
    } else {
        mouseButton = -1;
    }
}

static void Handle_Motion(int x, int y) {
    if (mouseButton == GLUT_LEFT_BUTTON) {
        camera_elevate_angle += (y - mouseY);
        camera_rotate_angle += (x - mouseX);
    } else if (mouseButton == GLUT_RIGHT_BUTTON) {
        float s = 0.1f;
        lights[0].position[0] += (x - mouseX) * s;
        lights[0].position[1] -= (y - mouseY) * s;
    }
    mouseX = x; 
    mouseY = y;
    glutPostRedisplay();
}

// === RENDERING ===

static void bindUniforms(GLuint prog) {
    if (prog == 0) {
        std::cerr << "Warning: Invalid shader program in bindUniforms" << std::endl;
        return;
    }
    
    glUseProgram(prog);
    if (!checkGLError("glUseProgram in bindUniforms")) return;
    
    // Get uniform locations (these may return -1 if not found, which is okay)
    GLint uMVP = glGetUniformLocation(prog, "ModelViewProj");
    GLint uMVI = glGetUniformLocation(prog, "ModelViewI");
    GLint uLight = glGetUniformLocation(prog, "lightPosition");
    GLint uDiffuse = glGetUniformLocation(prog, "diffuseTexture");
    GLint uHeight = glGetUniformLocation(prog, "heightMap");
    GLint uNormal = glGetUniformLocation(prog, "normalMap");
    GLint uScale = glGetUniformLocation(prog, "bumpScale");
    GLint uSelfShadow = glGetUniformLocation(prog, "selfShadowTest");
    GLint uParallax = glGetUniformLocation(prog, "parralax");
    
    // Enhanced uniforms
    GLint uTime = glGetUniformLocation(prog, "time");
    GLint uScreenSize = glGetUniformLocation(prog, "screenSize");
    GLint uFrameCount = glGetUniformLocation(prog, "frameCount");
    GLint uLightColor = glGetUniformLocation(prog, "lightColor");
    GLint uRoughness = glGetUniformLocation(prog, "roughness");
    GLint uMetallic = glGetUniformLocation(prog, "metallic");
    
    // Feature flags
    GLint uEnableParallax = glGetUniformLocation(prog, "enableParallax");
    GLint uEnablePBR = glGetUniformLocation(prog, "enablePBR");
    GLint uEnableSSAO = glGetUniformLocation(prog, "enableSSAO");
    GLint uEnableBloom = glGetUniformLocation(prog, "enableBloom");
    GLint uEnableToneMapping = glGetUniformLocation(prog, "enableToneMapping");
    GLint uEnableChromaticAberration = glGetUniformLocation(prog, "enableChromaticAberration");
    GLint uEnableVolumetricLighting = glGetUniformLocation(prog, "enableVolumetricLighting");
    GLint uEnableSubsurfaceScattering = glGetUniformLocation(prog, "enableSubsurfaceScattering");
    GLint uEnableReliefMapping = glGetUniformLocation(prog, "enableReliefMapping");
    GLint uEnableConeStepMapping = glGetUniformLocation(prog, "enableConeStepMapping");
    GLint uEnableProceduralNoise = glGetUniformLocation(prog, "enableProceduralNoise");
    GLint uEnableCaustics = glGetUniformLocation(prog, "enableCaustics");
    GLint uEnableNormalBlending = glGetUniformLocation(prog, "enableNormalBlending");
    GLint uEnableHeightFog = glGetUniformLocation(prog, "enableHeightFog");
    GLint uEnableMotionBlur = glGetUniformLocation(prog, "enableMotionBlur");
    GLint uEnableAdvancedSSAO = glGetUniformLocation(prog, "enableAdvancedSSAO");
    
    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    if (uDiffuse >= 0) glUniform1i(uDiffuse, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bumpTexture_id);
    if (uHeight >= 0) glUniform1i(uHeight, 1);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture_id);
    if (uNormal >= 0) glUniform1i(uNormal, 2);
    
    // Set basic uniforms
    if (uScale >= 0) glUniform1f(uScale, features.bumpy ? 0.125f : 0.05f);
    if (uSelfShadow >= 0) glUniform1f(uSelfShadow, features.selfShadowing ? 1.0f : 0.0f);
    if (uParallax >= 0) glUniform1f(uParallax, features.parallaxEnabled ? 1.0f : 0.0f);
    
    // Set enhanced uniforms
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();
    
    if (uTime >= 0) glUniform1f(uTime, time);
    if (uScreenSize >= 0) glUniform2f(uScreenSize, (float)screenWidth, (float)screenHeight);
    if (uFrameCount >= 0) glUniform1i(uFrameCount, perf.frameCount);
    if (uLightColor >= 0) glUniform3f(uLightColor, lights[0].color[0], lights[0].color[1], lights[0].color[2]);
    if (uRoughness >= 0) glUniform1f(uRoughness, 0.5f);
    if (uMetallic >= 0) glUniform1f(uMetallic, 0.1f);
    
    // Set feature flags
    if (uEnableParallax >= 0) glUniform1i(uEnableParallax, features.parallaxEnabled ? 1 : 0);
    if (uEnablePBR >= 0) glUniform1i(uEnablePBR, features.pbrShading ? 1 : 0);
    if (uEnableSSAO >= 0) glUniform1i(uEnableSSAO, features.ssaoEnabled ? 1 : 0);
    if (uEnableBloom >= 0) glUniform1i(uEnableBloom, features.bloomEnabled ? 1 : 0);
    if (uEnableToneMapping >= 0) glUniform1i(uEnableToneMapping, features.toneMappingEnabled ? 1 : 0);
    if (uEnableChromaticAberration >= 0) glUniform1i(uEnableChromaticAberration, features.chromaticAberration ? 1 : 0);
    if (uEnableVolumetricLighting >= 0) glUniform1i(uEnableVolumetricLighting, features.volumetricLighting ? 1 : 0);
    if (uEnableSubsurfaceScattering >= 0) glUniform1i(uEnableSubsurfaceScattering, features.subsurfaceScattering ? 1 : 0);
    if (uEnableReliefMapping >= 0) glUniform1i(uEnableReliefMapping, features.reliefMapping ? 1 : 0);
    if (uEnableConeStepMapping >= 0) glUniform1i(uEnableConeStepMapping, features.coneStepMapping ? 1 : 0);
    if (uEnableProceduralNoise >= 0) glUniform1i(uEnableProceduralNoise, features.proceduralNoise ? 1 : 0);
    if (uEnableCaustics >= 0) glUniform1i(uEnableCaustics, features.caustics ? 1 : 0);
    if (uEnableNormalBlending >= 0) glUniform1i(uEnableNormalBlending, features.normalBlending ? 1 : 0);
    if (uEnableHeightFog >= 0) glUniform1i(uEnableHeightFog, features.heightFog ? 1 : 0);
    if (uEnableMotionBlur >= 0) glUniform1i(uEnableMotionBlur, features.motionBlur ? 1 : 0);
    if (uEnableAdvancedSSAO >= 0) glUniform1i(uEnableAdvancedSSAO, features.ssaoEnabled ? 1 : 0);
}

static void Handle_Display() {
    // Frame rate limiting to prevent overwhelming the system
    limitFrameRate();
    
    updatePerformance();

    // Comprehensive error checking and validation
    if (!checkGLError("Display function start")) return;
    
    // Defensive: Check OpenGL context and lights vector
    if (lights.empty()) {
        std::cerr << "Error: No lights available!" << std::endl;
        return;
    }

    // Defensive: Check VAO
    if (VAO == 0) {
        std::cerr << "Error: VAO not initialized!" << std::endl;
        return;
    }

    // Defensive: Check shader program
    if (shaderPrograms.empty()) {
        std::cerr << "Error: No shader programs loaded!" << std::endl;
        return;
    }

    // Defensive: Check texture IDs
    if (texture_id == 0 || bumpTexture_id == 0 || normalTexture_id == 0) {
        std::cerr << "Error: One or more textures not loaded!" << std::endl;
        return;
    }

    // Auto-rotate camera
    if (features.autoRotate) {
        camera_rotate_angle += 0.5f;
    }

    // Clamp light position
    lights[0].position[0] = clamp(lights[0].position[0], -10.0f, 10.0f);
    lights[0].position[1] = clamp(lights[0].position[1], -10.0f, 10.0f);
    lights[0].position[2] = clamp(lights[0].position[2], 2.0f, 20.0f);

    // Enable/disable multisampling
    if (features.multisampling) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    // Enable/disable wireframe
    if (features.wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Clear buffers
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable scissor for split-screen
    glEnable(GL_SCISSOR_TEST);

    int halfW = screenWidth / 2;
    int squareW = (screenHeight < halfW ? screenHeight : halfW);

    // === LEFT SIDE: Basic Parallax ===
    int leftX = halfW - squareW;
    glViewport(leftX, 0, squareW, squareW);
    glScissor(leftX, 0, squareW, squareW);

    // Setup camera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 35, 0, 0, 0, 0, 1, 0);

    // Transform light to eye space
    float MV0[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, MV0);
    if (!checkGLError("glGetFloatv(GL_MODELVIEW_MATRIX)")) return;
    
    float lightEye[3] = {
        MV0[0] * lights[0].position[0] + MV0[4] * lights[0].position[1] + MV0[8] * lights[0].position[2] + MV0[12],
        MV0[1] * lights[0].position[0] + MV0[5] * lights[0].position[1] + MV0[9] * lights[0].position[2] + MV0[13],
        MV0[2] * lights[0].position[0] + MV0[6] * lights[0].position[1] + MV0[10] * lights[0].position[2] + MV0[14]
    };

    // Draw light marker with error checking
    glUseProgram(0);
    if (!checkGLError("glUseProgram(0)")) return;
    
    glPushMatrix();
    glTranslatef(lights[0].position[0], lights[0].position[1], lights[0].position[2]);
    glColor3f(lights[0].color[0], lights[0].color[1], lights[0].color[2]);
    glutSolidSphere(0.5f, 16, 16);
    glPopMatrix();
    
    if (!checkGLError("Light marker rendering")) return;

    // Rotate quad
    glLoadMatrixf(MV0);
    glRotatef(camera_elevate_angle, 1, 0, 0);
    glRotatef(camera_rotate_angle, 0, 1, 0);

    // Build matrices
    float MV[16], PM[16], MVP[16], invMV[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, MV);
    glGetFloatv(GL_PROJECTION_MATRIX, PM);
    if (!checkGLError("Matrix operations")) return;
    multiply4x4(PM, MV, MVP);
    invertRigid(MV, invMV);

    // Render with basic shader
    if (shaderPrograms.find("basic") != shaderPrograms.end()) {
        GLuint prog = shaderPrograms["basic"];
        if (prog != 0) {
            bindUniforms(prog);
            
            // Safely bind matrix uniforms
            GLint mvpLoc = glGetUniformLocation(prog, "ModelViewProj");
            GLint mviLoc = glGetUniformLocation(prog, "ModelViewI");
            GLint lightLoc = glGetUniformLocation(prog, "lightPosition");
            
            if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, MVP);
            if (mviLoc >= 0) glUniformMatrix4fv(mviLoc, 1, GL_FALSE, invMV);
            if (lightLoc >= 0) glUniform3fv(lightLoc, 1, lightEye);
            
            glBindVertexArray(VAO);
            if (!checkGLError("Binding VAO for basic shader")) return;
            
            glDrawArrays(GL_QUADS, 0, 4);
            if (!checkGLError("Drawing basic shader")) return;
        }
    }
    
    // === RIGHT SIDE: Enhanced Steep Parallax ===
    glClear(GL_DEPTH_BUFFER_BIT);
    int rightX = halfW;
    glViewport(rightX, 0, squareW, squareW);
    glScissor(rightX, 0, squareW, squareW);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 0, 35, 0, 0, 0, 0, 1, 0);
    
    // Recompute light in eye space
    glGetFloatv(GL_MODELVIEW_MATRIX, MV0);
    if (!checkGLError("glGetFloatv(GL_MODELVIEW_MATRIX) right side")) return;
    lightEye[0] = MV0[0] * lights[0].position[0] + MV0[4] * lights[0].position[1] + MV0[8] * lights[0].position[2] + MV0[12];
    lightEye[1] = MV0[1] * lights[0].position[0] + MV0[5] * lights[0].position[1] + MV0[9] * lights[0].position[2] + MV0[13];
    lightEye[2] = MV0[2] * lights[0].position[0] + MV0[6] * lights[0].position[1] + MV0[10] * lights[0].position[2] + MV0[14];
    
    // Rotate quad
    glLoadMatrixf(MV0);
    glRotatef(camera_elevate_angle, 1, 0, 0);
    glRotatef(camera_rotate_angle, 0, 1, 0);
    
    // Rebuild matrices
    glGetFloatv(GL_MODELVIEW_MATRIX, MV);
    glGetFloatv(GL_PROJECTION_MATRIX, PM);
    multiply4x4(PM, MV, MVP);
    invertRigid(MV, invMV);
    
    // Render with current shader
    if (shaderPrograms.find(currentShader) != shaderPrograms.end()) {
        GLuint prog = shaderPrograms[currentShader];
        if (prog != 0) {
            bindUniforms(prog);
            
            // Safely bind matrix uniforms
            GLint mvpLoc = glGetUniformLocation(prog, "ModelViewProj");
            GLint mviLoc = glGetUniformLocation(prog, "ModelViewI");
            GLint lightLoc = glGetUniformLocation(prog, "lightPosition");
            
            if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, MVP);
            if (mviLoc >= 0) glUniformMatrix4fv(mviLoc, 1, GL_FALSE, invMV);
            if (lightLoc >= 0) glUniform3fv(lightLoc, 1, lightEye);
            
            glBindVertexArray(VAO);
            if (!checkGLError("Binding VAO for enhanced shader")) return;
            
            glDrawArrays(GL_QUADS, 0, 4);
            if (!checkGLError("Drawing enhanced shader")) return;
        }
    }
    
    // === Performance Overlay ===
    if (features.showPerformance) {
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, screenWidth, screenHeight);
        glUseProgram(0);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        
        // Simple text rendering would go here
        // For now, we'll output to console periodically
        static int frameCounter = 0;
        if (++frameCounter % 60 == 0) {
            std::cout << "FPS: " << (int)perf.fps 
                      << " | Frame: " << perf.frameTime << "ms"
                      << " | Avg: " << perf.avgFrameTime << "ms" << std::endl;
        }
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }
    
    // Cleanup and final error check
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset wireframe
    
    // Final error check before buffer swap
    if (!checkGLError("End of display function")) {
        // If there's an error, try to recover by resetting OpenGL state
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        glDisable(GL_SCISSOR_TEST);
    }
    
    glutSwapBuffers();
    
    // Periodic error check to prevent accumulation
    static int errorCheckCounter = 0;
    if (++errorCheckCounter % 300 == 0) { // Every ~5 seconds at 60 FPS
        glGetError(); // Clear any pending errors
    }
}