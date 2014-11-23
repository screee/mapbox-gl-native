#ifndef MBGL_RENDERER_GL
#define MBGL_RENDERER_GL

#include <string>

#if __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_IPHONE
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>
    #elif TARGET_IPHONE_SIMULATOR
        #include <OpenGLES/ES2/gl.h>
        #include <OpenGLES/ES2/glext.h>
    #elif TARGET_OS_MAC
        #include <OpenGL/OpenGL.h>
        #include <OpenGL/gl.h>
    #else
        #error Unsupported Apple platform
    #endif
#elif MBGL_USE_GLES2
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
#else
    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>
#endif

namespace mbgl {
namespace gl {

// GL_ARB_vertex_array_object / GL_OES_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING 0x85B5
typedef void (* PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (* PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint* arrays);
typedef void (* PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint* arrays);
typedef GLboolean (* PFNGLISVERTEXARRAYPROC) (GLuint array);
extern PFNGLBINDVERTEXARRAYPROC BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC DeleteVertexArrays;
extern PFNGLGENVERTEXARRAYSPROC GenVertexArrays;
extern PFNGLISVERTEXARRAYPROC IsVertexArray;

// GL_ARB_get_program_binary / GL_OES_get_program_binary
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#define GL_PROGRAM_BINARY_LENGTH           0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS      0x87FE
#define GL_PROGRAM_BINARY_FORMATS          0x87FF
typedef void (* PFNGLGETPROGRAMBINARYPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
typedef void (* PFNGLPROGRAMBINARYPROC) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
typedef void (* PFNGLPROGRAMPARAMETERIPROC) (GLuint program, GLenum pname, GLint value);
extern PFNGLGETPROGRAMBINARYPROC GetProgramBinary;
extern PFNGLPROGRAMBINARYPROC ProgramBinary;
extern PFNGLPROGRAMPARAMETERIPROC ProgramParameteri;

// Debug group markers, useful for debugging on iOS
#if __APPLE__ && defined(DEBUG) && defined(GL_EXT_debug_marker)
// static int indent = 0;
inline void start_group(const std::string &str) {
    glPushGroupMarkerEXT(0, str.c_str());
    // fprintf(stderr, "%s%s\n", std::string(indent * 4, ' ').c_str(), str.c_str());
    // indent++;
}

inline void end_group() {
    glPopGroupMarkerEXT();
    // indent--;
}
#else
inline void start_group(const std::string &) {}
inline void end_group() {}
#endif


struct group {
    inline group(const std::string &str) { start_group(str); }
    ~group() { end_group(); };
};
}
}

#ifdef GL_ES_VERSION_2_0
    #define glClearDepth glClearDepthf
    #define glDepthRange glDepthRangef
#endif

void _CHECK_GL_ERROR(const char *cmd, const char *file, int line);

#define _CHECK_ERROR(cmd, file, line) \
    cmd; \
    do { _CHECK_GL_ERROR(#cmd, file, line); } while (false);

#define CHECK_ERROR(cmd) _CHECK_ERROR(cmd, __FILE__, __LINE__)

#endif
