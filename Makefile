# Cross-platform Makefile for Enhanced Steep Parallax Demo
# Supports both Windows (MinGW) and Linux builds

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -mtune=native
DEBUG_FLAGS = -g -O0 -DDEBUG

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM = linux
    LIBS = -lGL -lGLU -lGLEW -lglut -lm -lpthread
    INCLUDES = 
endif
ifeq ($(UNAME_S),Darwin)
    PLATFORM = macos
    LIBS = -framework OpenGL -lGLEW -lglut -lm -lpthread
    INCLUDES = 
endif
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    LIBS = -lglew32 -lfreeglut -lopengl32 -lgdi32 -lm
    INCLUDES = -I./GL
    CXXFLAGS += -DWIN32
endif

# Source files
SRCDIR = src
SOURCES = main_enhanced.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = SteepParallaxDemo

# Shader files
SHADERS = $(wildcard shaders/*.glsl) $(wildcard shaders/*.vert) $(wildcard shaders/*.frag)

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET) for $(PLATFORM)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "Build complete!"

# Compile source files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe
	@echo "Clean complete!"

# Install dependencies (Linux only)
install-deps:
ifeq ($(PLATFORM),linux)
	sudo apt-get update
	sudo apt-get install -y libglew-dev freeglut3-dev libglfw3-dev pkg-config build-essential
else
	@echo "Dependency installation only supported on Linux"
endif

# Run the demo
run: $(TARGET)
	./$(TARGET)

# Print build info
info:
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Sources: $(SOURCES)"

# Help
help:
	@echo "Enhanced Steep Parallax Demo - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the demo (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  clean        - Remove build artifacts"
	@echo "  install-deps - Install dependencies (Linux only)"
	@echo "  run          - Build and run the demo"
	@echo "  info         - Show build configuration"
	@echo "  help         - Show this help message"

.PHONY: all debug clean install-deps run info help