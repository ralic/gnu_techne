/* Copyright (C) 2009 Papavasileiou Dimitris
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "texture.h"
#include "shader.h"

#include "glsl/preamble.h"

#define is_sampler(type)                                                \
    ((type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_SHADOW) ||         \
     (type == GL_SAMPLER_1D_ARRAY || type == GL_SAMPLER_2D_ARRAY) ||    \
     (type >= GL_INT_SAMPLER_1D &&                                      \
      type <= GL_UNSIGNED_INT_SAMPLER_2D_ARRAY))

static char *declarations;
static struct {
    const char *name;
    unsigned int index;
} *globals;

static int globals_n;

struct {
    int length;
    const char **fragments;
} pipelines[T_STAGES_N];

#define TYPE_ERROR()							\
    {									\
        t_print_error("Value assigned to uniform variable '%s' of "	\
                      "shader '%s' is of unsuitable type.\n",		\
                      k, [self name]);					\
        abort();							\
    }

#define READ_SINGLE(value, length)					\
    {									\
        glGetBufferSubData (GL_UNIFORM_BUFFER, u->offset, length,       \
                            value);                                     \
    }

#define UPDATE_SINGLE(value, length)					\
    {									\
        glBufferSubData (GL_UNIFORM_BUFFER, u->offset, length, value);	\
    }

#define READ_ARRAY(size_0, stride_0, value, length)			\
    {									\
        void *b;							\
        int i;								\
                                                                        \
        b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
                                                                        \
        for(i = 0 ; i < size_0 ; i += 1) {				\
            memcpy((void *)value + i * length,                          \
                   b + u->offset + i * stride_0,                        \
                   length);						\
        }								\
                                                                        \
        glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define UPDATE_ARRAY(size_0, stride_0, value, length)			\
    {									\
        void *b;							\
        int i;								\
                                                                        \
        b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
                                                                        \
        for(i = 0 ; i < size_0 ; i += 1) {				\
            memcpy(b + u->offset + i * stride_0,			\
                   (void *)value + i * length,				\
                   length);						\
        }								\
                                                                        \
        glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define READ_MATRIX_ARRAY(size_0, stride_0, size_1, stride_1,		\
                            value, length)				\
    {									\
        void *b;							\
        int i, j;							\
                                                                        \
        b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
                                                                        \
        for(j = 0 ; j < size_1 ; j += 1) {				\
            for(i = 0 ; i < size_0 ; i += 1) {				\
                memcpy((void *)value + (j * size_0 + i) * length,	\
                       b + u->offset + i * stride_0 + j * stride_1,	\
                       length);						\
            }								\
        }								\
                                                                        \
        glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define UPDATE_MATRIX_ARRAY(size_0, stride_0, size_1, stride_1,		\
                            value, length)				\
    {									\
        void *b;							\
        int i, j;							\
                                                                        \
        b = glMapBuffer (GL_UNIFORM_BUFFER, GL_WRITE_ONLY);		\
                                                                        \
        for(j = 0 ; j < size_1 ; j += 1) {				\
            for(i = 0 ; i < size_0 ; i += 1) {				\
                memcpy(b + u->offset + i * stride_0 + j * stride_1,	\
                       (void *)value + (j * size_0 + i) * length,	\
                       length);						\
            }								\
        }								\
                                                                        \
        glUnmapBuffer (GL_UNIFORM_BUFFER);				\
    }

#define UPDATE_VALUE(size_0, stride_0, size_1, stride_1, elements,	\
                     ctype, gltype, arraytype, ismatrix)                \
    {									\
        array_Array *array = NULL;                                      \
        ctype n;							\
                                                                        \
        if (size_0 == 1 && size_1 == 1) {				\
            /* This is a non-array value. */                            \
                                                                        \
            if (elements == 1) {					\
                /* A scalar. */                                         \
                                                                        \
                if (gltype == GL_BOOL) {                                \
                    if (lua_type (_L, 3) != LUA_TBOOLEAN) {             \
                        TYPE_ERROR();					\
                    }                                                   \
                                                                        \
                    n = (ctype)lua_toboolean (_L, 3);			\
                } else {                                                \
                    if (lua_type (_L, 3) != LUA_TNUMBER) {              \
                        TYPE_ERROR();                                   \
                    }                                                   \
                                                                        \
                    n = (ctype)lua_tonumber (_L, 3);			\
                }                                                       \
                                                                        \
                UPDATE_SINGLE (&n, elements * sizeof(ctype));		\
            } else {							\
                /* A vector. */                                         \
                                                                        \
                array = array_testcompatible (_L, 3,                    \
                                              ARRAY_TYPE | ARRAY_RANK,	\
                                              arraytype, 1);		\
                                                                        \
                if(!array) {						\
                    TYPE_ERROR();					\
                }							\
                                                                        \
                /* Adjust the array to the uniform's size if            \
                 * needed. */                                           \
                                                                        \
                if (array->size[0] != elements) {                       \
                    if (elements == 4) {                                \
                        array = array_adjust(_L, 3,                     \
                                             ((ctype[4]){0, 0, 0, 1}),  \
                                             1, elements);              \
                    } else {                                            \
                        array = array_adjust(_L, 3, NULL, 1, elements); \
                    }                                                   \
                }                                                       \
                                                                        \
                UPDATE_SINGLE (array->values.any,			\
                               elements * sizeof(ctype));		\
            }								\
        } else if (size_1 == 1) {                                       \
            /* This is either an array or a matrix. */                  \
                                                                        \
            if (elements == 1) {					\
                /* A scalar array, adjust to size if needed. */         \
                                                                        \
                array_testcompatible (_L, 3,                            \
                                      ARRAY_TYPE | ARRAY_RANK,          \
                                      arraytype, 1);                    \
                                                                        \
                array = array_adjust(_L, 3, NULL, 1, size_0);           \
            } else {							\
                /* Either a vector array or a matrix. */                \
                                                                        \
                array = array_testcompatible (_L, 3,                    \
                                              ARRAY_TYPE | ARRAY_RANK,  \
                                              arraytype, 2);            \
                                                                        \
                /* Adjust to size if needed. */                         \
                                                                        \
                if (array &&                                            \
                    (array->size[0] != size_0 ||                        \
                     array->size[1] != elements)) {                     \
                    ctype defaults[size_0][elements];                   \
                    int i;                                              \
                                                                        \
                    memset (defaults, 0, sizeof (defaults));            \
                                                                        \
                    if(ismatrix) {                                      \
                        if (size_0 == elements) {			\
                            /* If this is a matrix initialize the	\
                             *  defaults to the unit matrix. */		\
                                                                        \
                            for(i = 0 ; i < size_0 ; i += 1) {		\
                                defaults[i][i] = 1;			\
                            }						\
                        }						\
                    } else {                                            \
                        /* If this is a vector array adjust each        \
                         * element to {0, 0, 0, 1}. */                  \
                                                                        \
                        if (elements == 4) {                            \
                            for(i = 0 ; i < size_0 ; i += 1) {          \
                                defaults[i][3] = 1;                     \
                            }                                           \
                        }                                               \
                    }                                                   \
                                                                        \
                    array = array_adjust(_L, 3, defaults,               \
                                         2, size_0, elements);          \
                }                                                       \
            }								\
                                                                        \
            if(!array) {						\
                TYPE_ERROR();						\
            }								\
                                                                        \
            UPDATE_ARRAY (size_0, stride_0, array->values.any,		\
                          elements * sizeof(ctype));			\
        } else {							\
            /* A matrix array. */                                       \
                                                                        \
            array = array_testcompatible (_L, 3,                        \
                                          ARRAY_TYPE | ARRAY_RANK,      \
                                          arraytype, 3);                \
                                                                        \
            if(!array) {						\
                TYPE_ERROR();						\
            }								\
                                                                        \
            /* This inversion is intentional: size_1 is the array       \
             * size, hence the number of matrices in the array and      \
             * size_0 is the row size of the matrix. */                 \
                                                                        \
            if (array->size[0] != size_1 ||                             \
                array->size[1] != size_0 ||                             \
                array->size[2] != elements) {                           \
                array = array_adjust(_L, 3, NULL,                       \
                                     3, size_1, size_0, elements);      \
            }                                                           \
                                                                        \
            UPDATE_MATRIX_ARRAY (size_0, stride_0, size_1, stride_1,    \
                                 array->values.any,			\
                                 elements * sizeof(ctype));		\
        }								\
    }

#define READ_VALUE(size_0, stride_0, size_1, stride_1, elements,	\
                   ctype, gltype, arraytype, ismatrix)                  \
    {									\
        array_Array *array;                                             \
        ctype n;                                                        \
                                                                        \
        if (size_0 == 1 && size_1 == 1) {				\
            /* This is a non-array value. */                            \
                                                                        \
            if (elements == 1) {					\
                /* A scalar. */                                         \
                                                                        \
                READ_SINGLE (&n, sizeof(ctype));                        \
                                                                        \
                if (gltype == GL_BOOL) {                                \
                    lua_pushboolean (_L, n);                            \
                } else {                                                \
                    lua_pushnumber (_L, n);                             \
                }                                                       \
            } else {							\
                /* A vector. */                                         \
                                                                        \
                array = array_createarray (_L, arraytype, NULL, 1, elements); \
                                                                        \
                READ_SINGLE (array->values.any,                         \
                             elements * sizeof(ctype));                 \
            }								\
        } else if (size_1 == 1) {                                       \
            /* This is either an array or a matrix. */                  \
                                                                        \
            if (elements == 1) {					\
                /* A scalar array. */                                   \
                                                                        \
                array = array_createarray (_L, arraytype, NULL, 1, size_0); \
            } else {							\
                /* Either a vector array or a matrix. */                \
                                                                        \
                array = array_createarray (_L, arraytype, NULL, 2,      \
                                   size_0, elements);                   \
            }								\
                                                                        \
            READ_ARRAY (size_0, stride_0, array->values.any,            \
                        elements * sizeof(ctype));			\
        } else {							\
            /* A matrix array. */                                       \
                                                                        \
            array = array_createarray(_L, arraytype, NULL, 3, size_1, size_0, \
                                      elements);                        \
                                                                        \
            READ_MATRIX_ARRAY (size_0, stride_0, size_1, stride_1,      \
                               array->values.any,			\
                               elements * sizeof(ctype));		\
        }								\
    }

#define CASE_BLOCK_S_V(ctype, gltype, arraytype)           \
    case gltype:                                           \
        DO_VALUE(u->size, u->arraystride, 1, 0, 1,     \
                 ctype, gltype, arraytype, 0); break;              \
    case gltype##_VEC2:                                    \
        DO_VALUE(u->size, u->arraystride, 1, 0, 2,         \
                     ctype, gltype, arraytype, 0); break;          \
    case gltype##_VEC3:                                    \
        DO_VALUE(u->size, u->arraystride, 1, 0, 3,         \
                     ctype, gltype, arraytype, 0); break;          \
    case gltype##_VEC4:                                    \
        DO_VALUE(u->size, u->arraystride, 1, 0, 4,         \
                     ctype, gltype, arraytype, 0); break;          \

#define CASE_BLOCK_M(ctype, gltype, arraytype)				\
    case gltype##_MAT2:							\
    DO_VALUE(2, u->matrixstride, u->size, u->arraystride, 2,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT3:							\
    DO_VALUE(3, u->matrixstride, u->size, u->arraystride, 3,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT4:							\
    DO_VALUE(4, u->matrixstride, u->size, u->arraystride, 4,	\
                 ctype, gltype, arraytype, 1); break;				\
                                                                        \
    case gltype##_MAT2x3:						\
    DO_VALUE(2, u->matrixstride, u->size, u->arraystride, 3,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT2x4:						\
    DO_VALUE(2, u->matrixstride, u->size, u->arraystride, 4,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT3x2:						\
    DO_VALUE(3, u->matrixstride, u->size, u->arraystride, 2,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT3x4:						\
    DO_VALUE(3, u->matrixstride, u->size, u->arraystride, 4,	\
                 ctype, gltype, arraytype, 1); break;				\
                                                                        \
    case gltype##_MAT4x2:						\
    DO_VALUE(4, u->matrixstride, u->size, u->arraystride, 2,	\
                 ctype, gltype, arraytype, 1); break;				\
    case gltype##_MAT4x3:						\
    DO_VALUE(4, u->matrixstride, u->size, u->arraystride, 3,	\
                 ctype, gltype, arraytype, 1); break;				\

#define DO_UNIFORM(uniform)						\
    {									\
        shader_Basic *u;						\
                                                                        \
        u = uniform;							\
                                                                        \
        glBindBuffer(GL_UNIFORM_BUFFER, self->blocks[u->block]);	\
                                                                        \
        switch (u->type) {						\
            CASE_BLOCK_S_V(double, GL_DOUBLE, ARRAY_TDOUBLE);		\
            CASE_BLOCK_M(double, GL_DOUBLE, ARRAY_TDOUBLE);		\
                                                                        \
            CASE_BLOCK_S_V(float, GL_FLOAT, ARRAY_TFLOAT);		\
            CASE_BLOCK_M(float, GL_FLOAT, ARRAY_TFLOAT);		\
                                                                        \
            CASE_BLOCK_S_V(int, GL_INT, ARRAY_TINT);			\
            CASE_BLOCK_S_V(unsigned int, GL_UNSIGNED_INT, ARRAY_TUINT);	\
            CASE_BLOCK_S_V(unsigned char, GL_BOOL, ARRAY_TUCHAR);	\
        default: _TRACE ("%x\n", u->type);abort();                      \
        }								\
    }

static int next_uniform(lua_State *L)
{
    Shader *shader;
    unsigned int i;

    shader = t_checknode (L, 1, [Shader class]);

    if (lua_isnil (L, 2)) {
        /* We need the first uniform so return 0. */

        if (shader->blocks_n > 0) {
            i = 0;
        } else {
            i = GL_INVALID_INDEX;
        }
    } else {
        const char *k;

        /* Look up the index of the current uniform and return the
         * next. */

        k = lua_tostring (L, 2);

        glGetUniformIndices(shader->name, 1, &k, &i);

        if (i != shader->blocks_n - 1) {
            i += 1;
        } else {
            i = GL_INVALID_INDEX;
        }
    }

    lua_pop (L, 1);

    if (i != GL_INVALID_INDEX) {
        GLenum type;
        int l, size;

        /* Get the uniforms name and return the key - value pair. */

        glGetActiveUniformsiv(shader->name, 1, &i,
                              GL_UNIFORM_NAME_LENGTH, &l);

        {
            char name[l];

            glGetActiveUniform(shader->name, i, l, NULL,
                               &size, &type, name);

            lua_pushstring (L, name);
        }

        lua_pushvalue (L, 2);
        lua_gettable (L, 1);

        return 2;
    } else {
        lua_pushnil(L);

        return 1;
    }
}

static int uniforms_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_uniform);
    lua_pushvalue(L, 1);
    lua_pushnil (L);

    return 3;
}

static void bind_shader(Shader *self)
{
    shader_Uniform *uniform;
    int i, j;

    /* Bind the program and all uniform buffers and proceed to draw
     * the meshes. */

    for (i = 0 ; i < self->blocks_n ; i += 1) {
        if (self->blocks[i] != GL_INVALID_INDEX) {
            glBindBufferBase(GL_UNIFORM_BUFFER, i, self->blocks[i]);
        }
    }

    if (self->public_buffer >= 0) {
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0,
                         self->buffers[self->public_buffer]);
    }

    for (i = 0 ; i < self->uniforms_n ; i += 1) {
        uniform = &self->uniforms[i];

        if (uniform->any.kind == SHADER_SAMPLER_UNIFORM) {
            shader_Sampler *sampler;
            shader_Texture *texture;
            unsigned int *names;

            sampler = &uniform->sampler;
            texture = &self->textures[sampler->index];

            names = sampler->size > 1 ? texture->names : &texture->name;

            /* _TRACE ("%s, %d, %d, %d\n", [self name], sampler->index, sampler->location, texture->name); */
            /* Bind all textures associated with the uniform to
             * consecutive units starting with the assigned base
             * unit.  */

            for (j = 0 ; j < sampler->size ; j += 1) {
                glUniform1i (sampler->location + j, sampler->unit + j);

                if (names[j] > 0) {
                    glActiveTexture(GL_TEXTURE0 + sampler->unit + j);
                    glBindTexture (sampler->target, names[j]);

                    /* { */
                    /*     char _name[64]; */
                    /*     glGetActiveUniformName (self->name, i, 64, NULL, _name); */
                    /* _TRACE ("%s: %d, (%d, %d)\n", _name, names[j], sampler->unit, sampler->target); */
                    /* } */
                }
            }
        }
    }
}

int t_add_global_block (const char *name, const char *declaration)
{
    unsigned int i;
    int l;

    /* Allocate a buffer object and add it to the globals list. */

    glGenBuffers(1, &i);

    globals_n += 1;
    globals = realloc (globals, globals_n * sizeof (globals[0]));

    globals[globals_n - 1].name = name;
    globals[globals_n - 1].index = i;

    /* Add the declaration for the global block to the declaractions
       string for the stage. */

    if (declarations) {
        l = strlen(declarations) + strlen(declaration) + 1;
        declarations = realloc(declarations, l * sizeof (char));
        strcat(declarations, declaration);
    } else {
        l = strlen(declaration) + 1;
        declarations = malloc(l * sizeof (char));
        strcpy(declarations, declaration);
    }

    return i;
}

static unsigned int get_uniform_index(Shader *shader, const char *uniform_name)
{
    unsigned int i;

    glGetUniformIndices(shader->name, 1, &uniform_name, &i);

    if (i == GL_INVALID_INDEX) {
        t_print_warning("Uniform '%s' is inactive.\n", uniform_name);
        return 0;
    }

    return i;
}

unsigned int t_sampler_unit(Shader *shader, const char *uniform_name)
{
    unsigned int i;

    i = get_uniform_index(shader, uniform_name);

    return shader->uniforms[i].sampler.unit;
}

void t_set_sampler(Shader *shader, const char *uniform_name,
                   unsigned int texture_name)
{
    shader_Sampler *sampler;
    shader_Texture *texture;
    unsigned int i;

    i = get_uniform_index(shader, uniform_name);

    sampler = &shader->uniforms[i].sampler;
    texture = &shader->textures[sampler->index];

    assert (sampler->kind == SHADER_SAMPLER_UNIFORM);
    assert (sampler->size == 1);

    luaL_unref(_L, LUA_REGISTRYINDEX, texture->reference);

    texture->name = texture_name;
    texture->reference = LUA_REFNIL;
}

void t_set_indexed_sampler(Shader *shader, const char *uniform_name,
                           unsigned int texture_name, int j)
{
    shader_Sampler *sampler;
    shader_Texture *texture;
    unsigned int i, *names;

    i = get_uniform_index(shader, uniform_name);

    sampler = &shader->uniforms[i].sampler;
    texture = &shader->textures[sampler->index];

    assert (sampler->kind == SHADER_SAMPLER_UNIFORM);
    assert (sampler->size > j);

    luaL_unref(_L, LUA_REGISTRYINDEX, texture->reference);
    texture->reference = LUA_REFNIL;

    names = sampler->size > 1 ? texture->names : &texture->name;
    names[j] = texture_name;
}

void t_reset_counter(Shader *shader, const char *uniform_name)
{
    unsigned int i;
    shader_Counter *counter;

    i = get_uniform_index(shader, uniform_name);
    counter = &shader->uniforms[i].counter;

    glBindBuffer (GL_ATOMIC_COUNTER_BUFFER,
                  shader->buffers[counter->buffer]);

    {
        unsigned int n[counter->size];

        memset (n, 0, sizeof(n));
        glBufferSubData (GL_ATOMIC_COUNTER_BUFFER, counter->offset,
                         sizeof (n), n);
    }
}

void t_get_counter(Shader *shader, const char *uniform_name, unsigned int *n)
{
    unsigned int i;
    shader_Counter *counter;

    i = get_uniform_index(shader, uniform_name);
    counter = &shader->uniforms[i].counter;

    glBindBuffer (GL_ATOMIC_COUNTER_BUFFER,
                  shader->buffers[counter->buffer]);

    glGetBufferSubData (GL_ATOMIC_COUNTER_BUFFER, counter->offset,
                        counter->size * sizeof(unsigned int), n);
}

void t_get_and_reset_counter(Shader *shader, const char *uniform_name,
                             unsigned int *n)
{
    unsigned int i;
    shader_Counter *counter;

    i = get_uniform_index(shader, uniform_name);
    counter = &shader->uniforms[i].counter;

    glBindBuffer (GL_ATOMIC_COUNTER_BUFFER,
                  shader->buffers[counter->buffer]);

    glGetBufferSubData (GL_ATOMIC_COUNTER_BUFFER, counter->offset,
                        counter->size * sizeof(unsigned int), n);

    {
        unsigned int m[counter->size];

        memset (m, 0, sizeof(m));
        glBufferSubData (GL_ATOMIC_COUNTER_BUFFER, counter->offset,
                         sizeof (m), m);
    }
}

@implementation ShaderMold

+(void)initialize
{
    if(self == [ShaderMold class]) {
        /* Export the uniforms iterator. */

        lua_pushcfunction (_L, uniforms_iterator);
        lua_setglobal (_L, "uniforms");
    }
}

-(void) initWithHandle: (ShaderMold **)handle_in
{
    [super init];

    self->name = glCreateProgram();
    self->private = NULL;
    self->private_n = 0;
    self->uniforms = NULL;
    self->uniforms_n = 0;
    self->blocks_n = 0;
    self->buffers_n = 0;
    self->handle = handle_in;

    if (self->handle) {
        *(self->handle) = self;
    }
}

-(void) add: (const int) n sourceStrings: (const char **) strings
        for: (t_ProcessingStage)stage
{
    unsigned int shader;
    int i;

    switch(stage) {
    case T_VERTEX_STAGE:
        shader = glCreateShader(GL_VERTEX_SHADER);break;
    case T_GEOMETRY_STAGE:
        shader = glCreateShader(GL_GEOMETRY_SHADER);break;
    case T_FRAGMENT_STAGE:
        shader = glCreateShader(GL_FRAGMENT_SHADER);break;
    case T_TESSELATION_CONTROL_STAGE:
        shader = glCreateShader(GL_TESS_CONTROL_SHADER);break;
    case T_TESSELATION_EVALUATION_STAGE:
        shader = glCreateShader(GL_TESS_EVALUATION_SHADER);break;
    default:
        assert(0);
    }

    if (declarations) {
        const char *sources[n + 2];

        sources[0] = glsl_preamble;
        sources[1] = declarations;

        for (i = 0 ; i < n ; i += 1) {
            char *source;

            asprintf(&source, "#line 1 %d\n%s\n", i + 1, strings[i]);
            sources[i + 2] = source;
        }

        glShaderSource(shader, n + 2, sources, NULL);
    } else {
        const char *sources[n + 1];

        sources[0] = glsl_preamble;

        for (i = 0 ; i < n ; i += 1) {
            char *source;

            asprintf(&source, "#line 1 %d\n%s\n", i + 1, strings[i]);
            sources[i + 1] = source;
        }

        glShaderSource(shader, n + 1, sources, NULL);
    }

    glCompileShader(shader);
    glAttachShader(self->name, shader);
    glDeleteShader(shader);
}

-(void) addSourceString: (const char *) source for: (t_ProcessingStage)stage
{
    [self add: 1 sourceStrings: &source for: stage];
}

-(void) addSourceFragement: (const char *) fragment
                       for: (t_ProcessingStage)stage
{
    pipelines[stage].fragments = realloc (pipelines[stage].fragments,
                                          (pipelines[stage].length + 1) * sizeof(char *));

    pipelines[stage].fragments[pipelines[stage].length] = fragment;
    pipelines[stage].length += 1;
}

-(void) finishAssemblingSourceFor: (t_ProcessingStage)stage
{
    assert (pipelines[stage].fragments != NULL);
    assert (pipelines[stage].length > 0);

    [self add: pipelines[stage].length sourceStrings: pipelines[stage].fragments for: stage];

    free(pipelines[stage].fragments);
    pipelines[stage].fragments = NULL;
    pipelines[stage].length = 0;
}

-(void) free
{
    /* Free the uniform information and delete the program. */

    if (self->uniforms_n > 0) {
        free(self->uniforms);
    }

    glDeleteProgram (self->name);

    if (self->handle) {
        *(self->handle) = NULL;
    }

    /* _TRACE ("Deleting %s program and associated shaders.\n", [self name]); */

    [super free];
}

-(void) declare: (int) n privateUniforms: (const char **)names
{
    self->private = (const char **)realloc (self->private,
                                            (self->private_n + n) *
                                            sizeof (char *));

    memcpy(self->private + self->private_n, names, n * sizeof (char *));
    self->private_n += n;
}

-(void) link
{
    int i, j, k, n;
    int u;

    glGetProgramiv(self->name, GL_LINK_STATUS, &i);
    if (i == GL_TRUE) {
        t_print_error("Attempting to link already linked program.\n");
        abort();
    }

    glLinkProgram(self->name);

    /* Validate the shader and print the info logs if necessary. */

    glGetProgramiv(self->name, GL_LINK_STATUS, &i);
    glGetProgramiv(self->name, GL_ATTACHED_SHADERS, &n);
    glGetProgramiv(self->name, GL_ACTIVE_ATTRIBUTES, &j);
    glGetProgramiv(self->name, GL_ACTIVE_UNIFORMS, &k);

    if (i == GL_TRUE) {
        t_print_message("Program linked with %d shaders, %d uniforms and %d attributes).\n", n, k, j);
    } else {
        t_print_warning("Program did not link.\n",
                        [self name]);
    }

    glGetProgramiv(self->name, GL_INFO_LOG_LENGTH, &j);

    if (j > 1) {
        char buffer[j];

        glGetProgramInfoLog (self->name, j, NULL, buffer);
        t_print_warning("Linker output follows:\n%s\n", buffer);
    }

    if (n > 0) {
        unsigned int shaders[n];

        glGetAttachedShaders (self->name, n, NULL, shaders);

        for (i = 0 ; i < n ; i += 1) {
            char *type;

            glGetShaderiv(shaders[i], GL_SHADER_TYPE, &j);

            switch (j) {
            case GL_VERTEX_SHADER: type = "Vertex"; break;
            case GL_FRAGMENT_SHADER: type = "Fragment"; break;
            case GL_GEOMETRY_SHADER: type = "Geometry"; break;
            case GL_TESS_CONTROL_SHADER: type = "Tesselation control"; break;
            case GL_TESS_EVALUATION_SHADER: type = "Tesselation evaluation"; break;
            default:assert(0);
            }

            glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &j);

            if (j != GL_TRUE) {
                t_print_warning("%s shader %d did not compile "
                                "successfully.\n",
                                type, i + 1);
            }

            glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &j);

            if (j > 1) {
                char buffer[j];

                glGetShaderInfoLog (shaders[i], j, NULL, buffer);
                t_print_warning("Compiler output follows:\n%s\n",
                                buffer);
            }
        }
    }

    /* We need to query the program about its active uniforms.  This
     * information will be used by all instances to allocate and
     * manage the memory that will hold the actual values for each
     * instance of the program. */

    glGetProgramiv (self->name, GL_ACTIVE_UNIFORMS, &u);

    if (u > 0) {
        unsigned int list[u], privates[u];
        int types[u], sizes[u], offsets[u], buffer_indices[u];
        int arraystrides[u], matrixstrides[u], block_indices[u];
        int i, l;

        self->uniforms_n = u;
        self->uniforms = malloc (u * sizeof (shader_Uniform));

        glGetProgramiv(self->name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &l);
        glGetUniformIndices (self->name, self->private_n, self->private,
                             privates);

        for (i = 0 ; i < u ; i += 1) {
            list[i] = i;
        }

        /* Read interesting parameters for all uniforms. */

        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_BLOCK_INDEX, block_indices);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_TYPE, types);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_SIZE, sizes);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_OFFSET, offsets);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_ARRAY_STRIDE, arraystrides);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_MATRIX_STRIDE, matrixstrides);
        glGetActiveUniformsiv(self->name, u, list,
                              GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX,
                              buffer_indices);

        for (i = 0, j = 0, n = 0 ; i < u ; i += 1) {
            shader_Uniform *uniform;

            uniform = &self->uniforms[i];
            uniform->any.mode = SHADER_PUBLIC_UNIFORM;

            if (is_sampler(types[i])) {
                char buffer[l];

                uniform->sampler.kind = SHADER_SAMPLER_UNIFORM;
                uniform->sampler.size = sizes[i];
                uniform->sampler.unit = n;
                uniform->sampler.index = j;

                n += uniform->sampler.size;
                j += 1;

                /* Cache the sampler location. */

                glGetActiveUniformName (self->name, list[i], l, NULL, buffer);
                uniform->sampler.location = glGetUniformLocation (self->name, buffer);

                /* Map the sampler type to texture target. */

                switch (types[i]) {
                case GL_SAMPLER_1D:
                    uniform->sampler.target = GL_TEXTURE_1D;
                    break;
                case GL_SAMPLER_2D:
                    uniform->sampler.target = GL_TEXTURE_2D;
                    break;
                case GL_SAMPLER_3D:
                    uniform->sampler.target = GL_TEXTURE_3D;
                    break;
                case GL_SAMPLER_2D_RECT:
                    uniform->sampler.target = GL_TEXTURE_RECTANGLE;
                    break;
                default:
                    t_print_error("Unknown sampler type.\n");
                    abort();
                }
            } else if (types[i] == GL_UNSIGNED_INT_ATOMIC_COUNTER) {
                uniform->counter.kind = SHADER_COUNTER_UNIFORM;

                uniform->counter.buffer = buffer_indices[i];
                uniform->counter.offset = offsets[i];
                uniform->counter.size = sizes[i];

                /* _TRACE ("%d: b = %d, offset = %d\n", i, buffer_indices[i], offsets[i]); */
            } else {
                uniform->basic.kind = SHADER_BASIC_UNIFORM;

                uniform->basic.block = block_indices[i];
                uniform->basic.type = types[i];
                uniform->basic.size = sizes[i];
                uniform->basic.offset = offsets[i];
                uniform->basic.arraystride = arraystrides[i];
                uniform->basic.matrixstride = matrixstrides[i];
            }
        }

        self->samplers_n = j;

        /* Take care of private uniforms. */

        for (i = 0 ; i < self->private_n ; i += 1) {
            j = privates[i];

            if (j >= 0) {
                assert(j < self->uniforms_n);
                self->uniforms[j].any.mode = SHADER_PRIVATE_UNIFORM;
            } else {
                _TRACE ("Private uniform '%s' (%d) is inactive.\n", self->private[i], i);
            }
        }
    } else {
        self->uniforms = NULL;
    }

    /* Here we just set up bindings between uniform blocks and buffer
     * objects.  Uniform block i is bound to binding point i and the
     * convention is that prior to drawing the shader binds all its
     * buffer objects in the same way.  That is the buffer object with
     * name blocks[i] is bound to binding point i.  The actual buffer
     * objects are allocated per-shader on load.  */

    glGetProgramiv (self->name, GL_ACTIVE_UNIFORM_BLOCKS, &self->blocks_n);

    for (i = 0 ; i < self->blocks_n ; i += 1) {
        glUniformBlockBinding (self->name, i, i);
    }

    glGetProgramiv (self->name, GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, &self->buffers_n);
}

@end

@implementation Shader

-(void)init
{
    [super init];

    self->reference = LUA_REFNIL;
    self->public_buffer = -1;
}

-(void) load
{
    ShaderMold *mold;
    int i;

    /* Make a reference to the mold to make sure it's not
     * collected. */

    mold = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);

    self->name = mold->name;
    self->uniforms_n = mold->uniforms_n;
    self->blocks_n = mold->blocks_n;
    self->buffers_n = mold->buffers_n;
    self->samplers_n = mold->samplers_n;

    /* Allocate and initialize the textures table. */

    self->uniforms = mold->uniforms;

    if (mold->samplers_n > 0) {
        shader_Uniform *uniform;

        self->textures = malloc (mold->samplers_n * sizeof (shader_Texture));

        for (i = 0 ; i < self->uniforms_n ; i += 1) {
            uniform = &self->uniforms[i];

            if (uniform->any.kind == SHADER_SAMPLER_UNIFORM) {
                shader_Sampler *sampler;
                shader_Texture *texture;

                sampler = &uniform->sampler;
                texture = &self->textures[sampler->index];

                texture->name = 0;
                texture->reference = LUA_REFNIL;

                if (sampler->size > 1) {
                    texture->names = calloc (sampler->size,
                                             sizeof(unsigned int));
                } else {
                    texture->names = NULL;
                }
            }
        }
    } else {
        self->textures = NULL;
    }

    /* Allocate the uniform buffer objects. */

    if (self->blocks_n > 0) {
        self->blocks = malloc (self->blocks_n * sizeof (unsigned int));

        for (i = 0 ; i < self->blocks_n ; i += 1) {
            int l;

            glGetActiveUniformBlockiv(self->name, i,
                                      GL_UNIFORM_BLOCK_NAME_LENGTH,
                                      &l);

            {
                char blockname[l];

                glGetActiveUniformBlockName(self->name, i, l, NULL,
                                            blockname);

                /* If the block name begins with two underscores it's a
                 * global block so it'll have already been created and
                 * bound. */

                if (!strncmp(blockname, "__", 2)) {
                    int j;

                    for (j = 0;
                         j < globals_n && strcmp (globals[j].name, blockname);
                         j += 1);

                    assert (j != globals_n);
                    self->blocks[i] = globals[j].index;
                } else {
                    int n;

                    glGetActiveUniformBlockiv(self->name, i,
                                              GL_UNIFORM_BLOCK_DATA_SIZE,
                                              &n);

                    glGenBuffers(1, &self->blocks[i]);
                    glBindBuffer(GL_UNIFORM_BUFFER, self->blocks[i]);

                    {
                        char zero[n];

                        memset (zero, 0, n * sizeof (char));
                        glBufferData(GL_UNIFORM_BUFFER, n, zero, GL_DYNAMIC_DRAW);
                    }
                }
            }
        }
    } else {
        self->blocks = NULL;
    }

    /* Allocate the atomic counter buffer objects. */

    if (self->buffers_n > 0) {
        self->buffers = malloc (self->buffers_n * sizeof (unsigned int));

        for (i = 0 ; i < self->buffers_n ; i += 1) {
            int j, n;

            /* Keep a note of the index if this buffer is bound to
             * binding point 0, which is reserved for public atomic
             * counters. */

            glGetActiveAtomicCounterBufferiv(self->name, i, GL_ATOMIC_COUNTER_BUFFER_BINDING, &j);

            if (j == 0) {
                self->public_buffer = i;
            }

            /* Allocate a buffer object of the required size. */

            glGetActiveAtomicCounterBufferiv(self->name, i, GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE, &n);

            glGenBuffers(1, &self->buffers[i]);
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, self->buffers[i]);

            {
                char zero[n];

                memset (zero, 0, n * sizeof (char));
                glBufferData(GL_ATOMIC_COUNTER_BUFFER, n, zero,
                             GL_DYNAMIC_COPY);
            }
        }
    } else {
        self->buffers = NULL;
    }
}

-(void) unload
{
    int i, j;

    /* Free the uniform buffer objects. */

    if (self->blocks_n > 0) {
        /* Remove any global blocks from the list.  We don't want to
           free those. */

        for (i = 0 ; i < self->blocks_n ; i += 1) {
            for (j = 0 ; j < globals_n ; j += 1) {
                if (self->blocks[i] == globals[j].index) {
                    self->blocks[i] = GL_INVALID_INDEX;
                }
            }
        }

        glDeleteBuffers (self->blocks_n, self->blocks);
        free(self->blocks);
    }

    /* Free the atomic counter buffer objects. */

    if (self->buffers_n > 0) {
        glDeleteBuffers (self->buffers_n, self->buffers);
        free(self->buffers);
    }

    /* Free the textures table if there is one. */

    if (self->samplers_n > 0) {
        for (i = 0 ; i < self->uniforms_n ; i += 1) {
            shader_Uniform *uniform;

            uniform = &self->uniforms[i];

            if (uniform->any.kind == SHADER_SAMPLER_UNIFORM) {
                shader_Sampler *sampler;
                shader_Texture *texture;

                sampler = &uniform->sampler;
                texture = &self->textures[sampler->index];

                luaL_unref(_L, LUA_REGISTRYINDEX, texture->reference);

                if (sampler->size > 1) {
                    free(texture->names);
                }
            }
        }

        free(self->textures);
    }

    /* Release the reference to the mold. */

    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);
}

-(void) free
{
    [self unload];
    [super free];
}

-(int) _get_
{
    const char *k;
    unsigned int i;

    /* Skip this if the shader has no uniforms. */

    if (self->blocks_n == 0) {
        return [super _get_];
    }

    k = lua_tostring (_L, 2);

    /* Check if the key refers to a uniform and return the stored
     * reference. */

    glGetUniformIndices(self->name, 1, &k, &i);

    if (i != GL_INVALID_INDEX) {
        shader_Uniform *uniform;

        uniform = &self->uniforms[i];

        if (uniform->any.mode != SHADER_PRIVATE_UNIFORM) {
            if (uniform->any.kind == SHADER_BASIC_UNIFORM) {
#define DO_VALUE READ_VALUE
                DO_UNIFORM (&self->uniforms[i].basic);
#undef DO_VALUE

                return 1;
            } else if (uniform->any.kind == SHADER_SAMPLER_UNIFORM) {
                shader_Sampler *sampler;
                shader_Texture *texture;

                sampler = &self->uniforms[i].sampler;
                texture = &self->textures[sampler->index];

                lua_rawgeti (_L, LUA_REGISTRYINDEX, texture->reference);

                return 1;
            } else if (uniform->any.kind == SHADER_COUNTER_UNIFORM) {
                int n[uniform->counter.size];

                glBindBuffer (GL_ATOMIC_COUNTER_BUFFER,
                              self->buffers[uniform->counter.buffer]);

                glGetBufferSubData (GL_ATOMIC_COUNTER_BUFFER,
                                    uniform->counter.offset,
                                    sizeof (n), n);

                if (uniform->counter.size == 1) {
                    lua_pushunsigned (_L, n[0]);
                } else {
                    int i;

                    lua_createtable (_L, uniform->counter.size, 0);

                    for (i = 0 ; i < uniform->counter.size ; i += 1) {
                        lua_pushinteger (_L, n[i]);
                        lua_rawseti (_L, -2, i + 1);
                    }
                }

                return 1;
            }
        }
    }

    return [super _get_];
}

-(int) _set_
{
    const char *k;
    unsigned int i;

    /* Skip this if the shader has no uniforms. */

    if (self->blocks_n == 0 || lua_type(_L, 2) != LUA_TSTRING) {
        return [super _set_];
    }

    k = lua_tostring (_L, 2);

    /* Check if the key refers to a uniform and if so update the
     * uniform buffer object it's stored in. */

    glGetUniformIndices(self->name, 1, &k, &i);

    if (i != GL_INVALID_INDEX) {
        shader_Uniform *uniform;

        uniform = &self->uniforms[i];

        if (uniform->any.mode != SHADER_PRIVATE_UNIFORM) {
            if (uniform->any.kind == SHADER_BASIC_UNIFORM) {
#define DO_VALUE UPDATE_VALUE
                DO_UNIFORM (&uniform->basic);
#undef DO_VALUE

                return 1;
            } else if (uniform->any.kind == SHADER_SAMPLER_UNIFORM) {
                shader_Sampler *sampler;
                shader_Texture *texture;
                unsigned int *names;
                int j, n;

                sampler = &self->uniforms[i].sampler;
                texture = &self->textures[sampler->index];

                if (sampler->size > 1) {
                    if (!lua_istable (_L, 3)) {
                        t_print_error("Sampler array must be set with an array of textures.\n");
                        abort();
                    }
                } else {
                    /* Wrap the texture in a table to unify handling
                     * of single and array samplers. */

                    lua_createtable (_L, 1, 0);
                    lua_insert(_L, -2);
                    lua_rawseti (_L, -2, 1);
                }

                luaL_unref (_L, LUA_REGISTRYINDEX, texture->reference);

                n = lua_rawlen(_L, 3);
                names = sampler->size > 1 ? texture->names : &texture->name;

                for (j = 0 ; j < n ; j += 1) {
                    Texture *texture;

                    lua_rawgeti (_L, 3, j + 1);
                    texture = t_testtexture(_L, -1, sampler->target);

                /* _TRACE ("%p\n", texture); */
                    if (!texture) {
                        names[j] = 0;
                    } else {
                    /* _TRACE ("%d\n", texture->name); */
                        names[j] = texture->name;
                    }

                    lua_pop(_L, 1);
                }

                texture->reference = luaL_ref (_L, LUA_REGISTRYINDEX);

                return 1;
            } else if (uniform->any.kind == SHADER_COUNTER_UNIFORM) {
                unsigned int n[uniform->counter.size];

                glBindBuffer (GL_ATOMIC_COUNTER_BUFFER,
                              self->buffers[uniform->counter.buffer]);

                if (uniform->counter.size == 1) {
                    if (!lua_isnumber (_L, 3)) {
                        t_print_error("Atomic counter must be set with an integer.\n");
                        abort();
                    }

                    n[0] = lua_tounsigned (_L, 3);
                } else {
                    if (!lua_istable (_L, 3)) {
                        t_print_error("Arrays of atomic counters must be set with a table.\n");
                        abort();
                    }

                    for (i = 0 ; i < uniform->counter.size ; i += 1) {
                        lua_rawgeti (_L, -1, i + 1);
                        n[i] = lua_tounsigned (_L, -1);
                        lua_pop(_L, 1);
                    }
                }

                glBufferSubData (GL_ATOMIC_COUNTER_BUFFER,
                                 uniform->counter.offset,
                                 sizeof (n), n);

                return 1;
            }
        }
    }

    return [super _set_];
}

-(void) bind
{
    bind_shader(self);
}

@end
