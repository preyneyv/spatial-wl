#include "shaders.h"

#include <GLES2/gl2.h>
#include <wlr/util/log.h>

#include "shaders/hello_world_fragment.h"
#include "shaders/hello_world_vertex.h"

static GLuint ctwl_gl_compile_shader(GLenum type, const GLchar *src,
                                     const GLint length) {
    GLuint shader = glCreateShader(type);
    wlr_log(WLR_ERROR, "SHADER: %d", shader);
    glShaderSource(shader, 1, &src, &length);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        wlr_log(WLR_ERROR, "Failed to compile shader: %s", log);
        return 0;
    }

    return shader;
}

struct ctwl_shader_build_job {
    const char *name;
    const GLchar *vertex_src;
    const GLuint vertex_len;
    const GLchar *fragment_src;
    const GLuint fragment_len;
    GLuint *program_ptr;
};

bool ctwl_gl_init(struct ctwl_gl *gl) {
    struct ctwl_shader_build_job jobs[] = {
        {.name = "hello_world",
         .vertex_src = shader_hello_world_vertex,
         .vertex_len = shader_hello_world_vertex_len,
         .fragment_src = shader_hello_world_fragment,
         .fragment_len = shader_hello_world_fragment_len,
         .program_ptr = &gl->hello_world}};

    for (size_t i = 0; i < sizeof(jobs) / sizeof(jobs[0]); i++) {
        struct ctwl_shader_build_job *job = &jobs[i];
        GLuint vertex_shader = ctwl_gl_compile_shader(
            GL_VERTEX_SHADER, job->vertex_src, job->vertex_len);
        if (vertex_shader == 0) {
            wlr_log(WLR_ERROR, "Failed to compile %s (vertex)", job->name);
            return false;
        }

        GLuint fragment_shader = ctwl_gl_compile_shader(
            GL_FRAGMENT_SHADER, job->fragment_src, job->fragment_len);
        if (fragment_shader == 0) {
            wlr_log(WLR_ERROR, "Failed to compile %s (fragment)", job->name);
            return false;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        glAttachShader(program, fragment_shader);
        glLinkProgram(program);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            char log[512];
            glGetProgramInfoLog(program, sizeof(log), NULL, log);
            wlr_log(WLR_ERROR, "Failed to link %s: %s", job->name, log);
            return false;
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        *job->program_ptr = program;
    }
}

void ctwl_gl_finish(struct ctwl_gl *gl) { glDeleteProgram(gl->hello_world); }