#ifndef OPENGL_FRAMEWORK_SHADER_ATTRIB_HPP
#define OPENGL_FRAMEWORK_SHADER_ATTRIB_HPP

#include <GLFW/glfw3.h>

struct ShaderAttrib {
  GLuint index;
  GLint size;
  unsigned int stride;
  unsigned int offset;
};

#endif //OPENGL_FRAMEWORK_SHADER_ATTRIB_HPP
