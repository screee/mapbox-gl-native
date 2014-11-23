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

    GLuint vertShader;
    if (!compileShader(&vertShader, GL_VERTEX_SHADER, vertSource)) {
        Log::Error(Event::Shader, "Vertex shader failed to compile: %s", vertSource);
        return;
    }

    GLuint fragShader;
    if (!compileShader(&fragShader, GL_FRAGMENT_SHADER, fragSource)) {
        Log::Error(Event::Shader, "Fragment shader failed to compile: %s", fragSource);
        return;
    }

    program = glCreateProgram();

    // Attach shaders
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);


    {
        if (gl::GetProgramBinary != nullptr) {
            gl::ProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
        }
    
        // Link program
        GLint status;
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == 0) {
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

    {
        // Validate program
        GLint status;
        glValidateProgram(program);

        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        if (status == 0) {
            GLsizei logLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            if (logLength > 0) {
                GLchar *log = (GLchar *)malloc(logLength);
                glGetProgramInfoLog(program, logLength, &logLength, log);
                Log::Error(Event::Shader, "Program failed to validate: %s", log);
                free(log);
                log = nullptr;
            }

            glDeleteShader(vertShader);
            vertShader = 0;
            glDeleteShader(fragShader);
            fragShader = 0;
            glDeleteProgram(program);
            program = 0;
        }

    }

    if (gl::GetProgramBinary != nullptr) {
        // Retrieve the program binary
        GLsizei binaryLength;
        glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binaryLength);
        if (binaryLength > 0) {
            GLenum binaryFormat;
            void *binary = (void *)malloc(binaryLength);
            gl::GetProgramBinary(program, binaryLength, NULL, &binaryFormat, binary);

            // Write the binary to a file            
            std::string binaryFileName = mbgl::platform::defaultShaderCache() + name + ".bin";
            FILE *binaryFile = fopen(binaryFileName.c_str(), "wb");
            assert(binaryFile != nullptr);
            fwrite(&binaryLength, sizeof(binaryLength), 1, binaryFile);
            fwrite(&binaryFormat, sizeof(binaryFormat), 1, binaryFile);
            fwrite(binary, binaryLength, 1, binaryFile);
            fclose(binaryFile);
            free(binary);
            binaryFile = nullptr;
            binary = nullptr;
        }
    }

    // Remove the compiled shaders; they are now part of the program.
    glDetachShader(program, vertShader);
    glDeleteShader(vertShader);
    glDetachShader(program, fragShader);
    glDeleteShader(fragShader);

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
    if (status == 0) {
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
