#ifndef _DEF_OPTIX_HELPES_DISPLAY_UTILS_H_
#define _DEF_OPTIX_HELPES_DISPLAY_UTILS_H_

#include <iostream>

#include <GL/glew.h>
//#define GL3_PROTOTYPES 1
#include <GL/gl.h>

#include <optix_helpers/Source.h>

namespace optix_helpers { namespace display {

bool checkGLError(const std::string& location = "");

GLuint compile_shader(GLenum shaderType, const Source& source);

GLuint create_render_program(const Source& vertexShaderSource,
                             const Source& fragmentShaderSource);

}; //namespace display
}; //namespace optix_helpers


#endif //_DEF_OPTIX_HELPES_DISPLAY_UTILS_H_
