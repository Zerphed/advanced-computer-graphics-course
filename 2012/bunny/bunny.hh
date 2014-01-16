#ifndef BUNNY_HH
#define BUNNY_HH

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/GL.h>

// In case the direct state access functions are not defined
// the functions that are used in the program are declared here
#if GL_EXT_direct_state_access == 0

void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    GLint old;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old);
    glUseProgram(program);
    glUniform1f(location, v0);
    glUseProgram(old);
}


void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    GLint old;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old);
    glUseProgram(program);
    glUniform1i(location, v0);
    glUseProgram(old);
}

void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLint old;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old);
    glUseProgram(program);
    glUniform4fv(location, count, value);
    glUseProgram(old);
}

void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat* value) {
    GLint old;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old);
    glUseProgram(program);
    glUniform3fv(location, count, value);
    glUseProgram(old);
}


void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    GLint old;
    glGetIntegerv(GL_CURRENT_PROGRAM, &old);
    glUseProgram(program);
    glUniformMatrix4fv(location, count, transpose, value);
    glUseProgram(old);
}
#endif

#endif