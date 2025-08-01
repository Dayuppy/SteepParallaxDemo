Parallax vs Steep Parallax Mapping Demo
---------------------------------------

This project is an OpenGL demonstration of standard Parallax Occlusion Mapping (POM) compared to Steep Parallax Mapping (SPM) using custom GLSL shaders.

Left half of the window: Standard parallax occlusion mapping

Right half of the window: Steep parallax occlusion mapping with optional self-shadowing

It visualizes the visual and performance differences between the two techniques using a textured quad and height/bump maps.

Background
---------------------------------------
Parallax Mapping is a real-time rendering technique that simulates depth on flat surfaces using height maps and UV offsetting, without increasing actual geometry.
Steep Parallax Mapping, as developed by Morgan McGuire and Max McGuire and later updated by Natalya Tatarchuk, improves realism with ray stepping to handle extreme viewing angles and optional self-shadowing.

Original implementation: Philippe David, ported and modernized to GLSL.

Features
---------------------------------------
Renders a quad with:

Color texture (lion.bmp)

Height/bump map (lion-bump.bmp)

Normal map (lion-normal.bmp)

Side-by-side comparison of basic parallax vs steep parallax mapping.

Interactive camera and movable point light.

Shader toggles:
---------------------------------------
M: Toggle multisampling
B: Toggle bump depth (scale)
S: Toggle self-shadowing (Steep Parallax only)
P: Enable/disable parallax effect
Q / Esc: Quit

Controls
---------------------------------------
Left mouse drag: Rotate the camera

Right mouse drag: Move the point light

Keyboard shortcuts: See the feature list above

Requirements
---------------------------------------
Windows + OpenGL 3.3+

GLEW and FreeGLUT (linked as external libraries)

Example linker settings (Visual Studio / MinGW):

How It Works
---------------------------------------
Initializes OpenGL context with FreeGLUT and GLEW.

Loads textures (diffuse, bump, and normal) from BMP files.

Compiles GLSL shaders for both parallax and steep parallax techniques.

Creates a textured quad (VBO + VAO) and renders it twice:

Left viewport: Standard parallax mapping

Right viewport: Steep parallax mapping with optional self-shadowing

Updates in real time based on camera rotation and light movement.

References:
---------------------------------------
Philippe David: Original implementation

[Morgan McGuire & Max McGuire: Steep Parallax Mapping] - (https://casual-effects.com/research/McGuire2005Parallax/index.html)

[Natalya Tatarchuk: Advanced Real-Time Rendering in 3D Graphics and Games] - (https://advances.realtimerendering.com/s2006/Chapter5-Parallax_Occlusion_Mapping_for_detailed_surface_rendering.pdf)

