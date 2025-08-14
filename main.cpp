//// Port of Cg prototype to GLSL with debug logging.
//// Author: Morgan McGuire, updated by Dayuppy
//
//#include <windows.h>
//
//#ifndef M_PI
//#define M_PI 3.14159265358979323846
//#endif
//
//#pragma comment(lib, "glew32.lib")
//#pragma comment(lib, "freeglut.lib")
//#include "GL/glew.h"
//#include "GL/glut.h"
//#include "READ_BMP.h"
//#include <fstream>
//#include <sstream>
//
//// Camera and window
//static float camera_rotate_angle = 0.0f;
//static float camera_elevate_angle = -20.0f;
//static int   screenWidth = 1400;
//static int   screenHeight = 700;
//
//// Mouse state
//static int mouseButton = -1, mouseX = 0, mouseY = 0;
//
//// Quality toggles
//static bool multisampling = true;
//static bool bumpy = false;
//static bool selfShadowing = true;
//static bool parallaxEnabled = true;
//
//// Light
//static float lightPosition[3] = { 0.0f, 0.0f, 8.0f };
//static float lightModelViewMat[16];
//
//// Textures
//static GLuint  texture_id;
//static GLuint  bumpTexture_id;
//static GLuint  normalTexture_id;
//static BYTE* image_data = nullptr;
//static int     image_width = 0;
//static int     image_height = 0;
//
//// Shader programs
//static GLuint vsProg = 0;
//static GLuint psProg = 0;
//static GLuint psSteepProg = 0;
//
//// Geometry
//static GLuint VBO = 0;
//static GLuint VAO = 0;
//
//// Forward declarations
//static void initPrograms();
//static void initGeometry();
//static void Handle_Display();
//static void Handle_Keyboard(unsigned char key, int x, int y);
//static void Handle_Reshape(int w, int h);
//static void Handle_Mouse(int button, int state, int x, int y);
//static void Handle_Motion(int x, int y);
//
//template<typename T>
//constexpr T clamp(T v, T lo, T hi) {
//    return (v < lo) ? lo : (v > hi) ? hi : v;
//}
//
//// Helper: multiply two 4×4 matrices (column-major) ⇒ out = A * B
//static void multiply4x4(const float A[16], const float B[16], float out[16]) {
//    for (int r = 0; r < 4; ++r) {
//        for (int c = 0; c < 4; ++c) {
//            float sum = 0.0f;
//            for (int k = 0; k < 4; ++k) {
//                sum += A[k * 4 + r] * B[c * 4 + k];
//            }
//            out[c * 4 + r] = sum;
//        }
//    }
//}
//
//// Helper: invert an affine rigid‐body transform M (rotation + translation only)
//static void invertRigid(const float M[16], float out[16]) {
//    // Transpose the 3×3 rotation
//    out[0] = M[0];  out[1] = M[4];  out[2] = M[8];   out[3] = 0.0f;
//    out[4] = M[1];  out[5] = M[5];  out[6] = M[9];   out[7] = 0.0f;
//    out[8] = M[2];  out[9] = M[6];  out[10] = M[10];  out[11] = 0.0f;
//    // Compute -Rᵀ * T
//    out[12] = -(out[0] * M[12] + out[4] * M[13] + out[8] * M[14]);
//    out[13] = -(out[1] * M[12] + out[5] * M[13] + out[9] * M[14]);
//    out[14] = -(out[2] * M[12] + out[6] * M[13] + out[10] * M[14]);
//    out[15] = 1.0f;
//}
//
//// Utility: read entire file into string
//static std::string readFile(const char* path) {
//    std::ifstream f(path, std::ios::binary);
//    if (!f.is_open()) {
//        fprintf(stderr, "ERROR: cannot open '%s'\n", path);
//        return "";
//    }
//    std::ostringstream ss;
//    ss << f.rdbuf();
//    return ss.str();
//}
//
//// Compile a shader and print any errors
//static GLuint compileShader(GLenum type, const std::string& src) {
//    GLuint s = glCreateShader(type);
//    const char* c = src.c_str();
//    glShaderSource(s, 1, &c, nullptr);
//    glCompileShader(s);
//    GLint status = GL_FALSE;
//    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
//    if (status != GL_TRUE) {
//        char log[512];
//        glGetShaderInfoLog(s, 512, nullptr, log);
//        fprintf(stderr, "Shader compile error (%s):\n%s\n",
//            type == GL_VERTEX_SHADER ? "VERT" : "FRAG", log);
//        glDeleteShader(s);
//        return 0;
//    }
//    return s;
//}
//
//// Link program, bind attributes and fragment output
//static GLuint linkProgram(GLuint vs, GLuint fs) {
//    GLuint p = glCreateProgram();
//    // Attribute locations must match VAO layout:
//    glBindAttribLocation(p, 0, "Position");
//    glBindAttribLocation(p, 1, "UV");
//    glBindAttribLocation(p, 2, "Normal");
//    glBindAttribLocation(p, 3, "Tangent");
//    glAttachShader(p, vs);
//    glAttachShader(p, fs);
//    glBindFragDataLocation(p, 0, "fragColor");
//    glLinkProgram(p);
//    GLint status = GL_FALSE;
//    glGetProgramiv(p, GL_LINK_STATUS, &status);
//    if (status != GL_TRUE) {
//        char log[512];
//        glGetProgramInfoLog(p, 512, nullptr, log);
//        fprintf(stderr, "Program link error:\n%s\n", log);
//        glDeleteProgram(p);
//        return 0;
//    }
//    return p;
//}
//
//// Build a complete shader program from two GLSL files
//static GLuint createShaderProgram(const char* vsFile, const char* fsFile) {
//    std::string vsSrc = readFile(vsFile);
//    std::string fsSrc = readFile(fsFile);
//    if (vsSrc.empty() || fsSrc.empty()) return 0;
//    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
//    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
//    if (!vs || !fs) return 0;
//    GLuint prog = linkProgram(vs, fs);
//    glDeleteShader(vs);
//    glDeleteShader(fs);
//    return prog;
//}
//
//// Debug helper for uniform locations
//static void debugUniform(GLuint prog, const char* name, GLint loc) {
//    if (loc < 0) {
//        fprintf(stderr, "DEBUG: Uniform '%s' not found in program %u\n", name, prog);
//    }
//    else {
//        fprintf(stdout, "DEBUG: Uniform '%s' loc=%d in program %u\n", name, loc, prog);
//    }
//}
//
//// Bind and set up uniforms & textures for basic parallax
//static void bindParallax(GLuint prog) {
//    glUseProgram(prog);
//    GLint uMVP = glGetUniformLocation(prog, "ModelViewProj");
//    GLint uMVI = glGetUniformLocation(prog, "ModelViewI");
//    GLint uLight = glGetUniformLocation(prog, "lightPosition");
//    GLint uDiffuse = glGetUniformLocation(prog, "diffuseTexture");
//    GLint uHeight = glGetUniformLocation(prog, "heightMap");
//    GLint uNormal = glGetUniformLocation(prog, "normalMap");
//    GLint uScale = glGetUniformLocation(prog, "bumpScale");
//    GLint uParallax = glGetUniformLocation(prog, "parralax");
//
//    debugUniform(prog, "ModelViewProj", uMVP);
//    debugUniform(prog, "ModelViewI", uMVI);
//    debugUniform(prog, "lightPosition", uLight);
//    debugUniform(prog, "diffuseTexture", uDiffuse);
//    debugUniform(prog, "heightMap", uHeight);
//    debugUniform(prog, "normalMap", uNormal);
//    debugUniform(prog, "bumpScale", uScale);
//    debugUniform(prog, "parralax", uParallax);
//
//    // Sampler bindings
//    glUniform1i(uDiffuse, 0);
//    glUniform1i(uHeight, 1);
//    glUniform1i(uNormal, 2);
//
//    // Texture units
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, texture_id);
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, bumpTexture_id);
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, normalTexture_id);
//
//    // Options
//    glUniform1f(uScale, bumpy ? 0.125f : 0.05f);
//    glUniform1f(uParallax, parallaxEnabled ? 1.0f : 0.0f);
//}
//
//// Bind and set up uniforms & textures for steep‐parallax
//static void bindSteep(GLuint prog) {
//    glUseProgram(prog);
//    GLint uMVP = glGetUniformLocation(prog, "ModelViewProj");
//    GLint uMVI = glGetUniformLocation(prog, "ModelViewI");
//    GLint uLight = glGetUniformLocation(prog, "lightPosition");
//    GLint uDiffuse = glGetUniformLocation(prog, "diffuseTexture");
//    GLint uHeight = glGetUniformLocation(prog, "heightMap");
//    GLint uNormal = glGetUniformLocation(prog, "normalMap");
//    GLint uScale = glGetUniformLocation(prog, "bumpScale");
//    GLint uSelfShadow = glGetUniformLocation(prog, "selfShadowTest");
//
//    debugUniform(prog, "ModelViewProj", uMVP);
//    debugUniform(prog, "ModelViewI", uMVI);
//    debugUniform(prog, "lightPosition", uLight);
//    debugUniform(prog, "diffuseTexture", uDiffuse);
//    debugUniform(prog, "heightMap", uHeight);
//    debugUniform(prog, "normalMap", uNormal);
//    debugUniform(prog, "bumpScale", uScale);
//    debugUniform(prog, "selfShadowTest", uSelfShadow);
//
//    // Sampler bindings
//    glUniform1i(uDiffuse, 0);
//    glUniform1i(uHeight, 1);
//    glUniform1i(uNormal, 2);
//
//    // Texture units
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, texture_id);
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, bumpTexture_id);
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, normalTexture_id);
//
//    glUniform1f(uScale, bumpy ? 0.125f : 0.05f);
//    glUniform1f(uSelfShadow, selfShadowing ? 1.0f : 0.0f);
//}
//
//// Display callback
//static void Handle_Display() {
//    // Clamp light so it can't wander off
//    lightPosition[0] = clamp(lightPosition[0], -10.0f, 10.0f);
//    lightPosition[1] = clamp(lightPosition[1], -10.0f, 10.0f);
//    lightPosition[2] = clamp(lightPosition[2], 2.0f, 20.0f);
//
//    // Multisample
//    if (multisampling) glEnable(GLUT_MULTISAMPLE);
//    else               glDisable(GLUT_MULTISAMPLE);
//
//    // Clear
//    glClearColor(0, 0, 0, 1);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//    // Enable scissor to clip at center line
//    glEnable(GL_SCISSOR_TEST);
//
//    // Compute half‐width and square size
//    int halfW = screenWidth / 2;
//    int squareW = (screenHeight < halfW ? screenHeight : halfW);
//
//    // ---- LEFT SQUARE: basic parallax ----
//    int leftX = halfW - squareW;
//    glViewport(leftX, 0, squareW, squareW);
//    glScissor(leftX, 0, squareW, squareW);
//
//    // Setup camera
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
//    gluLookAt(0, 0, 35, 0, 0, 0, 0, 1, 0);
//
//    // Light in eye space
//    float MV0[16];
//    glGetFloatv(GL_MODELVIEW_MATRIX, MV0);
//    float lightEye[3] = {
//        MV0[0] * lightPosition[0] + MV0[4] * lightPosition[1] + MV0[8] * lightPosition[2] + MV0[12],
//        MV0[1] * lightPosition[0] + MV0[5] * lightPosition[1] + MV0[9] * lightPosition[2] + MV0[13],
//        MV0[2] * lightPosition[0] + MV0[6] * lightPosition[1] + MV0[10] * lightPosition[2] + MV0[14]
//    };
//
//    // Draw light‐marker on left side only
//    glUseProgram(0);
//    glPushMatrix();
//    glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
//    glColor3f(1, 1, 0);
//    glutSolidSphere(0.5f, 16, 16);
//    glPopMatrix();
//
//    // Rotate quad
//    glLoadMatrixf(MV0);
//    glRotatef(camera_elevate_angle, 1, 0, 0);
//    glRotatef(camera_rotate_angle, 0, 1, 0);
//
//    // Build MVP + inverse MV
//    float MV[16], PM[16], MVP[16], invMV[16];
//    glGetFloatv(GL_MODELVIEW_MATRIX, MV);
//    glGetFloatv(GL_PROJECTION_MATRIX, PM);
//    multiply4x4(PM, MV, MVP);
//    invertRigid(MV, invMV);
//
//    // Render left quad
//    glUseProgram(psProg);
//    glUniformMatrix4fv(glGetUniformLocation(psProg, "ModelViewProj"), 1, GL_FALSE, MVP);
//    glUniformMatrix4fv(glGetUniformLocation(psProg, "ModelViewI"), 1, GL_FALSE, invMV);
//    glUniform3fv(glGetUniformLocation(psProg, "lightPosition"), 1, lightEye);
//    bindParallax(psProg);
//    glBindVertexArray(VAO);
//    glDrawArrays(GL_QUADS, 0, 4);
//
//    // ---- RIGHT SQUARE: steep parallax ----
//    glClear(GL_DEPTH_BUFFER_BIT);
//    int rightX = halfW;
//    glViewport(rightX, 0, squareW, squareW);
//    glScissor(rightX, 0, squareW, squareW);
//
//    glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
//    gluLookAt(0, 0, 35, 0, 0, 0, 0, 1, 0);
//
//    // Recompute lightEye for steep side
//    glGetFloatv(GL_MODELVIEW_MATRIX, MV0);
//    lightEye[0] = MV0[0] * lightPosition[0] + MV0[4] * lightPosition[1] + MV0[8] * lightPosition[2] + MV0[12];
//    lightEye[1] = MV0[1] * lightPosition[0] + MV0[5] * lightPosition[1] + MV0[9] * lightPosition[2] + MV0[13];
//    lightEye[2] = MV0[2] * lightPosition[0] + MV0[6] * lightPosition[1] + MV0[10] * lightPosition[2] + MV0[14];
//
//    // **Removed** the fixed‐pipeline light marker on steep side
//
//    // Rotate quad
//    glLoadMatrixf(MV0);
//    glRotatef(camera_elevate_angle, 1, 0, 0);
//    glRotatef(camera_rotate_angle, 0, 1, 0);
//
//    // Rebuild MVP + inverse MV
//    glGetFloatv(GL_MODELVIEW_MATRIX, MV);
//    glGetFloatv(GL_PROJECTION_MATRIX, PM);
//    multiply4x4(PM, MV, MVP);
//    invertRigid(MV, invMV);
//
//    // Render right quad
//    glUseProgram(psSteepProg);
//    glUniformMatrix4fv(glGetUniformLocation(psSteepProg, "ModelViewProj"), 1, GL_FALSE, MVP);
//    glUniformMatrix4fv(glGetUniformLocation(psSteepProg, "ModelViewI"), 1, GL_FALSE, invMV);
//    glUniform3fv(glGetUniformLocation(psSteepProg, "lightPosition"), 1, lightEye);
//    bindSteep(psSteepProg);
//    glDrawArrays(GL_QUADS, 0, 4);
//
//    // Cleanup
//    glDisable(GL_SCISSOR_TEST);
//    glBindVertexArray(0);
//    glutSwapBuffers();
//}
//
//// Keyboard handler
//static void Handle_Keyboard(unsigned char key, int, int) {
//    if (key == 'q' || key == 'Q' || key == 27) exit(0);
//    if (key == 'm' || key == 'M') multisampling = !multisampling;
//    if (key == 'b' || key == 'B') bumpy = !bumpy;
//    if (key == 's' || key == 'S') selfShadowing = !selfShadowing;
//    if (key == 'p' || key == 'P') parallaxEnabled = !parallaxEnabled;
//}
//
//// Reshape handler
//static void Handle_Reshape(int w, int h) {
//    screenWidth = w;
//    screenHeight = h;
//    glViewport(0, 0, w, h);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//    gluPerspective(25.0, (double)w * 0.5 / h, 0.1, 1000.0);
//    glMatrixMode(GL_MODELVIEW);
//}
//
//// Mouse down/up
//static void Handle_Mouse(int button, int state, int x, int y) {
//    if (state == GLUT_DOWN) {
//        mouseButton = button;
//        mouseX = x; mouseY = y;
//    }
//    else {
//        mouseButton = -1;
//    }
//}
//
//// Mouse drag
//static void Handle_Motion(int x, int y) {
//    if (mouseButton == GLUT_LEFT_BUTTON) {
//        camera_elevate_angle += (y - mouseY);
//        camera_rotate_angle += (x - mouseX);
//    }
//    else if (mouseButton == GLUT_RIGHT_BUTTON) {
//        float s = 0.1f;
//        lightPosition[0] += (x - mouseX) * s;
//        lightPosition[1] -= (y - mouseY) * s;
//    }
//    mouseX = x; mouseY = y;
//    glutPostRedisplay();
//}
//
//// Compile/link shaders
//static void initPrograms() {
//    fprintf(stdout, "DEBUG: Compiling/linking GLSL shaders...\n");
//    vsProg = createShaderProgram("vsParallax.glsl", "psParallax.glsl");
//    psProg = createShaderProgram("vsParallax.glsl", "psParallax.glsl");
//    psSteepProg = createShaderProgram("vsParallax.glsl", "psSteepParallax.glsl");
//    if (!vsProg || !psProg || !psSteepProg) {
//        fprintf(stderr, "ERROR: Shader setup failed\n");
//        exit(1);
//    }
//}
//
//// Prepare VBO & VAO for a single quad
//static void initGeometry() {
//    float verts[] = {
//        -7,-7,4,  0,0,  0,0,1,  1,0,0,1,
//        -7, 7,4,  1,0,  0,0,1,  1,0,0,1,
//         7, 7,4,  1,1,  0,0,1,  1,0,0,1,
//         7,-7,4,  0,1,  0,0,1,  1,0,0,1
//    };
//    glGenVertexArrays(1, &VAO);
//    glGenBuffers(1, &VBO);
//    glBindVertexArray(VAO);
//    glBindBuffer(GL_ARRAY_BUFFER, VBO);
//    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
//    // layout(location = 0) Position
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)0);
//    glEnableVertexAttribArray(0);
//    // layout(location = 1) UV
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(3 * sizeof(float)));
//    glEnableVertexAttribArray(1);
//    // layout(location = 2) Normal
//    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(5 * sizeof(float)));
//    glEnableVertexAttribArray(2);
//    // layout(location = 3) Tangent
//    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), (void*)(8 * sizeof(float)));
//    glEnableVertexAttribArray(3);
//    glBindVertexArray(0);
//}
//
//// Entry point
//int main(int argc, char* argv[]) {
//    // Print working directory
//    char cwd[MAX_PATH];
//    if (GetCurrentDirectoryA(MAX_PATH, cwd))
//        fprintf(stdout, "Working directory: %s\n", cwd);
//
//    // Init GLUT + window
//    glutInit(&argc, argv);
//    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
//    glutInitWindowSize(screenWidth, screenHeight);
//    glutCreateWindow("Parallax Mapping GLSL");
//
//    // Init GLEW
//    glewExperimental = GL_TRUE;
//    if (glewInit() != GLEW_OK) {
//        fprintf(stderr, "ERROR: glewInit failed\n");
//        return 1;
//    }
//
//    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    // Load and upload lion.bmp
//    BMP_Read("lion.bmp", &image_data, image_width, image_height);
//    glGenTextures(1, &texture_id);
//    glBindTexture(GL_TEXTURE_2D, texture_id);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
//
//    // Load and upload lion-bump.bmp
//    BMP_Read("lion-bump.bmp", &image_data, image_width, image_height);
//    glGenTextures(1, &bumpTexture_id);
//    glBindTexture(GL_TEXTURE_2D, bumpTexture_id);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
//
//    // Load and upload lion-normal.bmp
//    BMP_Read("lion-normal.bmp", &image_data, image_width, image_height);
//    glGenTextures(1, &normalTexture_id);
//    glBindTexture(GL_TEXTURE_2D, normalTexture_id);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
//
//    initPrograms();
//    initGeometry();
//
//    glutMouseFunc(Handle_Mouse);
//    glutMotionFunc(Handle_Motion);
//    glutKeyboardFunc(Handle_Keyboard);
//    glutReshapeFunc(Handle_Reshape);
//    glutDisplayFunc(Handle_Display);
//    glutIdleFunc(Handle_Display);
//
//    glutMainLoop();
//    return 0;
//}