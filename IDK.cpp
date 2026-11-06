// I called this IDK.cpp because I GENUINELY don't know anything about what it does
// All I know is that these specific set of lines are required for most of the external
// libraries to work, like why bro what did I do

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define NOMINMAX

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define PL_MPEG_IMPLEMENTATION
#include <pl_mpeg.h>

#include <ft2build.h>
#include FT_FREETYPE_H 