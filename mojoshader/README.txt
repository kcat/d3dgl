This version of Mojoshader is heavily modified. A lot of unneeded functionality
is removed, including all non-GLSL profiles, and it generates extra code needed
to interface with the main library (for implementing or fixing up differences
between D3D9 and OpenGL, including pixel center differences, constants register
storage, etc).

The generated shaders require GLSL 3.30 and GL_ARB_separate_shader_objects.
