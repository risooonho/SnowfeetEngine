#include <core/util/Log.hpp>
#include "gl_check.hpp"
#include <GL/glew.h>
#include <string>

namespace sn {
namespace render {

void glCheckError(const char * file, unsigned int line)
{
	// Get last OpenGL error
	GLenum code = glGetError();

	if (code == GL_NO_ERROR)
		return;

	std::string error;

	switch (code)
	{
	case GL_INVALID_ENUM:
		error = "GL_INVALID_ENUM";
		break;

	case GL_INVALID_OPERATION:
		error = "GL_INVALID_OPERATION";
		break;

	case GL_STACK_OVERFLOW:
		error = "GL_STACK_OVERFLOW";
		break;

	case GL_STACK_UNDERFLOW:
		error = "GL_STACK_UNDERFLOW";
		break;

	case GL_OUT_OF_MEMORY:
		error = "GL_OUT_OF_MEMORY";
		break;

	default:
		error = "Unknown error";
		break;
	}

	SN_ERROR("OpenGL error in " << file << ": " << line << ": " << error);
}

} // namespace render
} // namespace sn
