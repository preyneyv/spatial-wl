#ifndef _CTWL_SHADERS_H
#define _CTWL_SHADERS_H

#include <GLES2/gl2.h>
#include <stdbool.h>

struct ctwl_gl {
    GLuint hello_world;
};

bool ctwl_gl_init(struct ctwl_gl *gl);
void ctwl_gl_finish(struct ctwl_gl *gl);

#endif