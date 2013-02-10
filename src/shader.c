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

static char *declarations;
static struct {
    const char *name;
    unsigned int index;
} *globals;

enum {
    PRIVATE_UNIFORM,
    BASIC_UNIFORM,
    SAMPLER_UNIFORM
} shader_UniformKind;

static int globals_n;

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
	    UPDATE_MATRIX_ARRAY (size_0, stride_0, size_1, stride_1, 	\
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
	    READ_MATRIX_ARRAY (size_0, stride_0, size_1, stride_1, 	\
                               array->values.any,			\
                               elements * sizeof(ctype));		\
	}								\
    }	    

#define CASE_BLOCK_S_V(ctype, gltype, arraytype)           \
    case gltype:                                           \
        DO_VALUE(u->size, u->arraystride, 1, 0, 1,     \
                 ctype, gltype, arraytype, 0); break;              \
    case gltype##_VEC2:                                    \
	DO_VALUE(u->size, u->arraystride, 1, 0, 2,	   \
                     ctype, gltype, arraytype, 0); break;          \
    case gltype##_VEC3:					   \
	DO_VALUE(u->size, u->arraystride, 1, 0, 3,	   \
                     ctype, gltype, arraytype, 0); break;          \
    case gltype##_VEC4:					   \
	DO_VALUE(u->size, u->arraystride, 1, 0, 4,	   \
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

@implementation ShaderMold

+(void)initialize
{
    if(self == [ShaderMold class]) {
	/* Export the uniforms iterator. */
	
	lua_pushcfunction (_L, uniforms_iterator);
	lua_setglobal (_L, "uniforms");	
    }
}

-(void) add: (const int) n sourceStrings: (const char **) strings
        for: (t_Enumerated)stage
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
    default:
	assert(0);
    }
    
    if (declarations) {
	const char *source[n + 2];

        source[0] = glsl_preamble;
        source[1] = declarations;

        for (i = 0 ; i < n ; i += 1) {
            source[i + 2] = strings[i];
        }

	glShaderSource(shader, n + 2, source, NULL);
    } else {
	const char *source[n + 1];

        source[0] = glsl_preamble;

        for (i = 0 ; i < n ; i += 1) {
            source[i + 1] = strings[i];
        }

	glShaderSource(shader, n + 1, source, NULL);
    }
    
    glCompileShader(shader);
    glAttachShader(self->name, shader);
    glDeleteShader(shader);
}

-(void) addSource: (const char *) source for: (t_Enumerated)stage
{
    [self add: 1 sourceStrings: &source for: stage];
}

-(void) initWithHandle: (ShaderMold **)handle
{
    [super init];

    self->name = glCreateProgram();
    self->private = NULL;
    self->private_n = 0;
    self->handle = handle;

    if (self->handle) {
        *(self->handle) = self;
    }
}

-(void) free
{
    /* Free the uniform buffer object table. */

    if (self->blocks_n > 0) {
	free(self->blocks);
    }
    
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
    self->private = names;
    self->private_n = n;
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
	t_print_message("Program for %s nodes linked successfully (%d shaders, %d uniforms and %d attributes).\n",
			[self name],
			n, k, j);
    } else {
	t_print_warning("Program for %s nodes did not link successfully.\n",
			[self name]);

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
		default:assert(0);
		}
	    
		glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &j);

		if (j == GL_TRUE) {
		    continue;
		}
	    
		t_print_warning("%s shader %d did not compile "
				"successfully.\n",
				type, i + 1);

		glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &j);

		if (j > 1) {
		    char buffer[j];

		    glGetShaderInfoLog (shaders[i], j, NULL, buffer);
		    t_print_warning("Compiler output follows:\n%s\n",
				    buffer);
		}
	    }
	}
    }
    
    /* Create a map of active uniforms. */
    
    glGetProgramiv (self->name, GL_ACTIVE_UNIFORMS, &u);
    
    if (u > 0) {
	unsigned int list[u], skiplist[u];
	int types[u], sizes[u], offsets[u];
	int arraystrides[u], matrixstrides[u], indices[u];
	int i, l;

        self->uniforms_n = u;
	self->uniforms = malloc (u * sizeof (shader_Uniform));

        glGetProgramiv(self->name, GL_ACTIVE_UNIFORM_MAX_LENGTH, &l);
        glGetUniformIndices (self->name, self->private_n, self->private,
                             skiplist);

	for (i = 0 ; i < u ; i += 1) {
	    list[i] = i;
	}	

	/* Read interesting parameters for all uniforms. */
	
	glGetActiveUniformsiv(self->name, u, list,
			      GL_UNIFORM_BLOCK_INDEX, indices);
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
        
	for (i = 0, n = 0 ; i < u ; i += 1) {
            shader_Uniform *uniform;
            int j, skip;

            for (j = 0, skip = 0 ; j < self->private_n ; j += 1) {
                if (i == skiplist[j]) {
                    skip = 1;
                    break;
                }
            }

            uniform = &self->uniforms[i];
            
            if (skip) {
                uniform->kind = PRIVATE_UNIFORM;
            } else if (types[i] >= GL_SAMPLER_1D &&
                       types[i] <= GL_SAMPLER_3D) {
                char buffer[l];

                uniform->sampler.kind = SAMPLER_UNIFORM;
                
                /* Cache the sampler location. */
            
                glGetActiveUniformName (self->name, list[i], l, NULL, buffer);
                uniform->sampler.location = glGetUniformLocation (self->name, buffer);

                /* Map sampler type to texture target. */
            
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
                default:
                    t_print_error("Unknown sampler type.\n");
                    abort();
                }
            } else {
                uniform->basic.kind = BASIC_UNIFORM;
                
                uniform->basic.block = indices[i];
                uniform->basic.type = types[i];
                uniform->basic.size = sizes[i];
                uniform->basic.offset = offsets[i];
                uniform->basic.arraystride = arraystrides[i];
                uniform->basic.matrixstride = matrixstrides[i];
            }	
        }
    } else {
	self->uniforms = NULL;
    }

    glGetProgramiv (self->name, GL_ACTIVE_UNIFORM_BLOCKS, &self->blocks_n);

    if (self->blocks_n > 0) {
	self->blocks = malloc (self->blocks_n * sizeof (unsigned int));

	for (i = 0 ; i < self->blocks_n ; i += 1) {
	    int l;

	    glUniformBlockBinding (self->name, i, i);
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
                    self->blocks[i] = GL_INVALID_INDEX;
		}
	    }
	}
    } else {
	self->blocks = NULL;
    }
}

@end

@implementation Shader

-(void)init
{
    [super init];

    self->reference = LUA_REFNIL;

    self->name = 0;    
    self->uniforms_n = 0;
    self->blocks_n = 0;

    self->uniforms = NULL;
    self->blocks = NULL;
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
    
    /* Allocate and initialize the uniforms table. */

    if (self->uniforms_n > 0) {
        self->uniforms = malloc (self->uniforms_n * sizeof (shader_Uniform));
        memcpy (self->uniforms, mold->uniforms,
                self->uniforms_n * sizeof (shader_Uniform));

        for (i = 0 ; i < self->uniforms_n ; i += 1) {
            if (self->uniforms[i].kind == SAMPLER_UNIFORM) {
                self->uniforms[i].sampler.unit = i;
                self->uniforms[i].sampler.texture = 0;
                self->uniforms[i].sampler.reference = LUA_REFNIL;
            }
        }
    } else {
	self->uniforms = NULL;
    }

    /* Allocate the uniform buffer objects. */
    
    if (self->blocks_n > 0) {
	self->blocks = malloc (self->blocks_n * sizeof (unsigned int));
        memcpy (self->blocks, mold->blocks,
                self->blocks_n * sizeof (unsigned int));

	for (i = 0 ; i < self->blocks_n ; i += 1) {
	    int s;

            if (self->blocks[i] == GL_INVALID_INDEX) {
                glGetActiveUniformBlockiv(self->name, i,
                                          GL_UNIFORM_BLOCK_DATA_SIZE,
                                          &s);

                glGenBuffers(1, &self->blocks[i]);
                glBindBuffer(GL_UNIFORM_BUFFER, self->blocks[i]);

                {
                    char zero[s];

                    memset (zero, 0, s * sizeof (char));
                    glBufferData(GL_UNIFORM_BUFFER, s, zero, GL_DYNAMIC_DRAW);
                }
            }
	}
    } else {
	self->blocks = NULL;
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

    /* Free the uniforms table. */

    if (self->uniforms_n > 0) {
        for (i = 0 ; i < self->uniforms_n ; i += 1) {
            if (self->uniforms[i].kind == SAMPLER_UNIFORM) {
                luaL_unref(_L, LUA_REGISTRYINDEX,
                           self->uniforms[i].sampler.reference);
            }
        }
        
        free(self->uniforms);
    }

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
        if (self->uniforms[i].kind == BASIC_UNIFORM) {
#define DO_VALUE READ_VALUE
            DO_UNIFORM (&self->uniforms[i].basic);
#undef DO_VALUE

            return 1;
        } else if (self->uniforms[i].kind == SAMPLER_UNIFORM) {
            lua_rawgeti (_L, LUA_REGISTRYINDEX, self->uniforms[i].sampler.reference);            

            return 1;
        }
    }

    return [super _get_];
}

-(int) _set_
{
    const char *k;
    unsigned int i;

    /* Skip this if the shader has no uniforms. */

    if (self->blocks_n == 0) {
        return [super _set_];
    }

    k = lua_tostring (_L, 2);

    /* Check if the key refers to a uniform and if so update the
     * uniform buffer object it's stored in. */
    
    glGetUniformIndices(self->name, 1, &k, &i);

    if (i != GL_INVALID_INDEX) {
        if (self->uniforms[i].kind == BASIC_UNIFORM) {
#define DO_VALUE UPDATE_VALUE
            DO_UNIFORM (&self->uniforms[i].basic);
#undef DO_VALUE
            
            return 1;
        } else if (self->uniforms[i].kind == SAMPLER_UNIFORM) {
            Texture *texture;
            shader_Sampler *sampler;

            sampler = &self->uniforms[i].sampler;
            texture = t_testtexture(_L, 3, sampler->target);
            /* _TRACE ("%p\n", texture); */
    
            if (!texture) {
                sampler->texture = 0;
            } else {
                /* _TRACE ("%d\n", texture->name); */
                sampler->texture = texture->name;
            }

            sampler->reference = luaL_ref (_L, LUA_REGISTRYINDEX);            

            return 1;
        }
    }

    return [super _set_];
}

-(void) draw: (int)frame
{
    int i;

    /* _TRACE ("%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f,\n" */
    /* 	    "%f, %f, %f, %f\n", */
    /* 	    self->matrix[0], self->matrix[1], self->matrix[2], self->matrix[3], self->matrix[4], self->matrix[5], self->matrix[6], self->matrix[7], self->matrix[8], self->matrix[9], self->matrix[10], self->matrix[11], self->matrix[12], self->matrix[13], self->matrix[14], self->matrix[15]); */

    /* Bind the program and all uniform buffers and proceed to draw
     * the meshes. */
    
    for (i = 0 ; i < self->blocks_n ; i += 1) {
	if (self->blocks[i] != GL_INVALID_INDEX) {
	    glBindBufferBase(GL_UNIFORM_BUFFER, i, self->blocks[i]);
	}
    }

    for (i = 0 ; i < self->uniforms_n ; i += 1) {
        if (self->uniforms[i].kind == SAMPLER_UNIFORM) {
            shader_Sampler *sampler;

            sampler = &self->uniforms[i].sampler;
        
            glActiveTexture(GL_TEXTURE0 + sampler->unit);
            glBindTexture (sampler->target, sampler->texture);
            glUniform1i (sampler->location, sampler->unit);
        }
    }
    
    [super draw: frame];
}

@end
