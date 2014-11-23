#include <mbgl/shader/shader.hpp>
#include <mbgl/platform/gl.hpp>
#include <mbgl/util/stopwatch.hpp>
#include <mbgl/platform/log.hpp>
#include <mbgl/platform/platform.hpp>

#include <cstring>
#include <cstdlib>
#include <cassert>

using namespace mbgl;

Shader::Shader(const char *name_, const GLchar *vertSource, const GLchar *fragSource)
    : name(name_),
      valid(false),
      program(0) {
    util::stopwatch stopwatch("shader compilation", Event::Shader);

    program = glCreateProgram();

    // Load binary shader if it exists
    bool skip_compile = false;
    std::string binaryFileName = mbgl::platform::defaultShaderCache() + name + ".bin";
    if (!binaryFileName.empty() && (gl::ProgramBinary != nullptr)) {
        FILE *binaryFile = fopen(binaryFileName.c_str(), "rb");
        if (binaryFile != nullptr) {
            GLsizei binaryLength;
            GLenum binaryFormat;
            fread(&binaryLength, sizeof(binaryLength), 1, binaryFile);
            fread(&binaryFormat, sizeof(binaryFormat), 1, binaryFile);
 
            if (binaryLength > 0) {
                void *binary = (void *)malloc(binaryLength);
                fread(binary, binaryLength, 1, binaryFile);

                gl::ProgramBinary(program, binaryFormat, binary, binaryLength);

                free(binary);
                binary = nullptr;

                // Check if the binary was valid
                GLint status;
                glGetProgramiv(program, GL_LINK_STATUS, &status);
                if (status == GL_TRUE) {
                    skip_compile = true;
                }
            }

            fclose(binaryFile);
            binaryFile = nullptr;
        }
    }

    GLuint vertShader;
    GLuint fragShader;
    if (!skip_compile) {
        if (!compileShader(&vertShader, GL_VERTEX_SHADER, vertSource)) {
            Log::Error(Event::Shader, "Vertex shader failed to compile: %s", vertSource);
            glDeleteProgram(program);
            program = 0;
            return;
        }

        if (!compileShader(&fragShader, GL_FRAGMENT_SHADER, fragSource)) {
            Log::Error(Event::Shader, "Fragment shader failed to compile: %s", fragSource);
            glDeleteShader(vertShader);
            vertShader = 0;
            glDeleteProgram(program);
            program = 0;
            return;
        }

        // Attach shaders
        glAttachShader(program, vertShader);
        glAttachShader(program, fragShader);

        {
            if (!binaryFileName.empty() && (gl::ProgramParameteri != nullptr)) {
                gl::ProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
            }

            // Link program
            GLint status;
            glLinkProgram(program);

            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                GLsizei logLength;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
                if (logLength > 0) {
                    GLchar *log = (GLchar *)malloc(logLength);
                    glGetProgramInfoLog(program, logLength, &logLength, log);
                    Log::Error(Event::Shader, "Program failed to link: %s", log);
                    free(log);
                    log = nullptr;
                }

                glDeleteShader(vertShader);
                vertShader = 0;
                glDeleteShader(fragShader);
                fragShader = 0;
                glDeleteProgram(program);
                program = 0;
                return;
            }
        }
    }

    {
        // Validate program
        GLint status;
        glValidateProgram(program);

        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        if (status == GL_FALSE) {
            GLsizei logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 0) {
                GLchar *log = (GLchar *)malloc(logLength);
                glGetProgramInfoLog(program, logLength, &logLength, log);
                Log::Error(Event::Shader, "Program failed to validate: %s", log);
                free(log);
                log = nullptr;
            }

            if (!skip_compile) {
                glDeleteShader(vertShader);
                vertShader = 0;
                glDeleteShader(fragShader);
                fragShader = 0;
            }
            glDeleteProgram(program);
            program = 0;
        }

    }

    if (!binaryFileName.empty() && !skip_compile && (gl::GetProgramBinary != nullptr)) {
        // Retrieve the program binary
        GLsizei binaryLength;
        GLenum binaryFormat;
        glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        if (binaryLength > 0) {
            void *binary = (void *)malloc(binaryLength);
            gl::GetProgramBinary(program, binaryLength, NULL, &binaryFormat, binary);

            // Write the binary to a file
            FILE *binaryFile = fopen(binaryFileName.c_str(), "wb");
            if (binaryFile != nullptr) {
                fwrite(&binaryLength, sizeof(binaryLength), 1, binaryFile);
                fwrite(&binaryFormat, sizeof(binaryFormat), 1, binaryFile);
                fwrite(binary, binaryLength, 1, binaryFile);
                fclose(binaryFile);
                binaryFile = nullptr;
            }

            free(binary);
            binary = nullptr;
        }
    }

    if (!skip_compile) {
        // Remove the compiled shaders; they are now part of the program.
        glDetachShader(program, vertShader);
        glDeleteShader(vertShader);
        glDetachShader(program, fragShader);
        glDeleteShader(fragShader);
    }

    valid = true;
}


bool Shader::compileShader(GLuint *shader, GLenum type, const GLchar *source) {
    GLint status;

    *shader = glCreateShader(type);
    const GLchar *strings[] = { source };
    const GLsizei lengths[] = { (GLsizei)strlen(source) };
    glShaderSource(*shader, 1, strings, lengths);

    glCompileShader(*shader);

    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLsizei logLength;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            GLchar *log = (GLchar *)malloc(logLength);
            glGetShaderInfoLog(*shader, logLength, &logLength, log);
            Log::Error(Event::Shader, "Shader failed to compile: %s", log);
            free(log);
            log = nullptr;
        }

        glDeleteShader(*shader);
        *shader = 0;
        return false;
    }

    return true;
}

Shader::~Shader() {
    if (program) {
        glDeleteProgram(program);
        program = 0;
        valid = false;
    }
}
