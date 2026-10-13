#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "../Component.h"
#include "../../core/statics/PublicDomain.h"

class TextRenderer : public Component
{
public:
    TextRenderer(
        std::string text,
        std::string fontPath,
        unsigned int pixelSize = 48,
        glm::vec3 color = glm::vec3(1.0f)
    )
        : text(std::move(text)),
          fontPath(std::move(fontPath)),
          pixelSize(pixelSize),
          color(color)
    {
        Initialize();
    }

    ~TextRenderer()
    {
        for (const auto& [character, glyph] : glyphs)
        {
            glDeleteTextures(1, &glyph.textureID);
        }

        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
        if (VBO != 0)
            glDeleteBuffers(1, &VBO);
        if (shader != 0)
            glDeleteProgram(shader);
    }

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    void SetText(const std::string& value) { text = value; }
    const std::string& GetText() const { return text; }

    // Transform::position is the text baseline in framebuffer pixels, measured
    // from the top-left corner. Transform::scale changes the glyph size.
    void Render() const
    {
        if (shader == 0 || VAO == 0 || !transform || text.empty())
            return;

        int width = 0;
        int height = 0;
        if (!PublicDomain::CurrentWindow)
            return;
        glfwGetFramebufferSize(PublicDomain::CurrentWindow->GetGLFWWindow(), &width, &height);
        if (width <= 0 || height <= 0)
            return;

        const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(shader);
        const glm::mat4 projection = glm::ortho(
            0.0f,
            static_cast<float>(width),
            static_cast<float>(height),
            0.0f
        );
        glUniformMatrix4fv(
            glGetUniformLocation(shader, "projection"),
            1,
            GL_FALSE,
            &projection[0][0]
        );
        glUniform3fv(glGetUniformLocation(shader, "textColor"), 1, &color[0]);
        glUniform1i(glGetUniformLocation(shader, "textTexture"), 0);

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(VAO);

        float x = transform->position.x;
        const float y = transform->position.y;
        const float scaleX = transform->scale.x;
        const float scaleY = transform->scale.y;

        for (const char character : text)
        {
            const auto glyph = glyphs.find(character);
            if (glyph == glyphs.end())
                continue;

            const Character& renderedGlyph = glyph->second;
            const float xpos = x + renderedGlyph.bearing.x * scaleX;
            const float ypos = y - renderedGlyph.bearing.y * scaleY;
            const float w = renderedGlyph.size.x * scaleX;
            const float h = renderedGlyph.size.y * scaleY;

            const float vertices[6][4] = {
                { xpos,     ypos,     0.0f, 0.0f },  // top-left
                { xpos,     ypos + h, 0.0f, 1.0f },  // bottom-left
                { xpos + w, ypos + h, 1.0f, 1.0f },  // bottom-right

                { xpos,     ypos,     0.0f, 0.0f },  // top-left
                { xpos + w, ypos + h, 1.0f, 1.0f },  // bottom-right
                { xpos + w, ypos,     1.0f, 0.0f }   // top-right
            };

            glBindTexture(GL_TEXTURE_2D, renderedGlyph.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += static_cast<float>(renderedGlyph.advance >> 6) * scaleX;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (depthEnabled)
            glEnable(GL_DEPTH_TEST);
        if (!blendEnabled)
            glDisable(GL_BLEND);
    }

private:
    struct Character
    {
        GLuint textureID = 0;
        glm::ivec2 size = glm::ivec2(0);
        glm::ivec2 bearing = glm::ivec2(0);
        long advance = 0;
    };

    std::string text;
    std::string fontPath;
    unsigned int pixelSize;
    glm::vec3 color;
    std::map<char, Character> glyphs;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shader = 0;

    static GLuint CompileShader(const char* source, GLenum type)
    {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[1024] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "ERROR::TEXT_RENDERER::SHADER_COMPILATION_FAILED: " << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void Initialize()
    {
        FT_Library library = nullptr;
        FT_Face face = nullptr;
        if (FT_Init_FreeType(&library) != 0 ||
            FT_New_Face(library, fontPath.c_str(), 0, &face) != 0)
        {
            std::cerr << "ERROR::TEXT_RENDERER::FONT_LOAD_FAILED: " << fontPath << std::endl;
            if (library)
                FT_Done_FreeType(library);
            return;
        }

        FT_Set_Pixel_Sizes(face, 0, pixelSize);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char character = 32; character < 128; ++character)
        {
            if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0)
                continue;

            GLuint texture = 0;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glyphs.emplace(
                static_cast<char>(character),
                Character{
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    face->glyph->advance.x
                }
            );
        }

        FT_Done_Face(face);
        FT_Done_FreeType(library);
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        const char* vertexSource = R"(
            #version 330 core
            layout (location = 0) in vec4 vertex;
            uniform mat4 projection;
            out vec2 textureCoordinate;
            void main()
            {
                gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
                textureCoordinate = vertex.zw;
            }
        )";
        const char* fragmentSource = R"(
            #version 330 core
            in vec2 textureCoordinate;
            uniform sampler2D textTexture;
            uniform vec3 textColor;
            out vec4 FragColor;
            void main()
            {
                FragColor = vec4(textColor, texture(textTexture, textureCoordinate).r);
            }
        )";

        const GLuint vertexShader = CompileShader(vertexSource, GL_VERTEX_SHADER);
        const GLuint fragmentShader = CompileShader(fragmentSource, GL_FRAGMENT_SHADER);
        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0)
                glDeleteShader(vertexShader);
            if (fragmentShader != 0)
                glDeleteShader(fragmentShader);
            return;
        }

        shader = glCreateProgram();
        glAttachShader(shader, vertexShader);
        glAttachShader(shader, fragmentShader);
        glLinkProgram(shader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint linked = GL_FALSE;
        glGetProgramiv(shader, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[1024] = {};
            glGetProgramInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "ERROR::TEXT_RENDERER::PROGRAM_LINK_FAILED: " << log << std::endl;
            glDeleteProgram(shader);
            shader = 0;
            return;
        }

        const float vertices[6][4] = {};
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};
