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

#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "structures.h"
#include "graphics.h"
#include "shape.h"
#include "shader.h"

static int next_attribute(lua_State *L)
{
    Shape *shape;
    shape_Buffer *b;

    shape = t_checknode (L, 1, [Shape class]);

    if (lua_isnil (L, 2)) {
	b = shape->buffers;
    } else {
	const char *k;

	/* Look for the current buffer and return the next. */
	
	k = lua_tostring (L, 2);

	for (b = shape->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);

	b = b->next;
    }

    lua_pop (L, 1);

    /* If we're not done return the key - value pair, otherwise
     * nil. */
	
    if (b) {
	lua_pushstring (L, b->key);
	lua_pushvalue (L, 2);
	lua_gettable (L, 1);

	return 2;
    } else {
	lua_pushnil(L);

	return 1;
    }
}

static int attributes_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_attribute);
    lua_pushvalue(L, 1);
    lua_pushnil (L);

    return 3;
}

static void match_attribute_to_buffer (unsigned int program,
				       unsigned int attribute,
				       shape_Buffer *buffer)
{
    char name;
    GLenum type;
    GLboolean normalized;	
    int i, size, integral, precision;

    glGetActiveAttrib (program, attribute, 0, NULL, &size, &type,
		       &name);
	
    switch (type) {
    case GL_DOUBLE_MAT2: case GL_DOUBLE_MAT3: case GL_DOUBLE_MAT4:
    case GL_DOUBLE_MAT2x3: case GL_DOUBLE_MAT2x4:
    case GL_DOUBLE_MAT3x2: case GL_DOUBLE_MAT3x4:
    case GL_DOUBLE_MAT4x2: case GL_DOUBLE_MAT4x3:
	t_print_error("Matrix type vertex attributes not "
		      "supported.\n");
	    
	abort();
	break;

    case GL_DOUBLE_VEC2: case GL_DOUBLE_VEC3: case GL_DOUBLE_VEC4:
    case GL_DOUBLE:
	if (buffer->type != ARRAY_TDOUBLE) {
	    t_print_error("Double precision floating point data "
			  "expected for vertex attribute '%s'.\n",
			  attribute);

	    abort();
	}

	precision = 1;
	break;

    case GL_FLOAT_MAT2: case GL_FLOAT_MAT3: case GL_FLOAT_MAT4:
    case GL_FLOAT_MAT2x3: case GL_FLOAT_MAT2x4:
    case GL_FLOAT_MAT3x2: case GL_FLOAT_MAT3x4:
    case GL_FLOAT_MAT4x2: case GL_FLOAT_MAT4x3:
	t_print_error("Matrix type vertex attributes not "
		      "supported.\n");
	    
	abort();
	break;

    case GL_FLOAT_VEC2: case GL_FLOAT_VEC3: case GL_FLOAT_VEC4:
    case GL_FLOAT:
	precision = 2;
	break;

    case GL_INT:
    case GL_INT_VEC2: case GL_INT_VEC3: case GL_INT_VEC4:

    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_VEC2: case GL_UNSIGNED_INT_VEC3:
    case GL_UNSIGNED_INT_VEC4:
	if (buffer->type == ARRAY_TDOUBLE ||
	    buffer->type == ARRAY_TFLOAT) {
	    t_print_error("Integral data expected for vertex attribute "
			  "'%s' node.\n", attribute);

	    abort();
	}

	precision = 3;
	break;
    default:
	assert(0);
    }

    /* Try to map the supplied array type to a GL type. */
	
    switch(abs(buffer->type)) {
    case ARRAY_TDOUBLE:
	type = GL_DOUBLE;
	normalized = GL_FALSE;
	integral = 0;
	break;
    case ARRAY_TFLOAT:
	type = GL_FLOAT;
	normalized = GL_FALSE;
	integral = 0;
	break;
    case ARRAY_TULONG:
    case ARRAY_TLONG:
	t_print_error("Array used for vertex attribute data is of "
		      "unsuitable type.\n");
	abort();	    
	break;
    case ARRAY_TUINT:
	type = GL_UNSIGNED_INT;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    case ARRAY_TINT:
	type = GL_INT;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    case ARRAY_TUSHORT:
	type = GL_UNSIGNED_SHORT;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    case ARRAY_TSHORT:
	type = GL_SHORT;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    case ARRAY_TUCHAR:
	type = GL_UNSIGNED_BYTE;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    case ARRAY_TCHAR:
	type = GL_BYTE;
	integral = buffer->type > 0;
	normalized = !integral ? GL_TRUE : GL_FALSE;
	break;
    default:
	assert(0);
    }

    /* Bind the data into the vertex array object's state. */
	
    glBindBuffer(GL_ARRAY_BUFFER, buffer->name);
    i = glGetAttribLocation(program, buffer->key);	

    switch (precision) {
    case 1:
	glVertexAttribIPointer(i, buffer->size, type, 0, (void *)0);
	break;
    case 2:
	glVertexAttribPointer(i, buffer->size, type, normalized,
			      0, (void *)0);
	break;
    case 3:
	glVertexAttribIPointer(i, buffer->size, type, 0, (void *)0);
	break;
    }

    glEnableVertexAttribArray(i);    
}

@implementation Shape

+(void)initialize
{
    if(self == [Shape class]) {
	lua_pushcfunction (_L, attributes_iterator);
	lua_setglobal (_L, "attributes");
    }
}

-(void)initWithMode: (GLenum) m
{
    [super init];

    self->mode = m;
    self->buffers = NULL;
    self->indices = NULL;
    self->wireframe = 0;

    /* Create the vertex array object. */
    
    glGenVertexArrays (1, &self->name);
}

-(void) free
{
    shape_Buffer *b, *next;

    /* Free the attribute buffers. */
    
    for (b = self->buffers ; b ; b = next) {
	next = b->next;

	glDeleteBuffers(1, &b->name);
	free(b->key);
	free(b);
    }

    /* Free the index buffer. */
    
    if (self->indices) {
	b = self->indices;
	
	glDeleteBuffers(1, &b->name);
	free(b->key);
	free(b);
    }

    /* And finally free the vertex array. */
    
    glDeleteVertexArrays (1, &self->name);

    [super free];
}

-(int) _get_wireframe
{
    lua_pushboolean (_L, self->wireframe);
    
    return 1;
}
 
-(void) _set_wireframe
{
    self->wireframe = lua_toboolean (_L, 3);
}

-(int) _get_
{
    array_Array array;
    shape_Buffer *b;
    const char *k;
    int size;

    k = lua_tostring (_L, 2);

    /* Check if the key corresponds to a buffer. */
    
    if (!strcmp (k, "indices")) {
	b = self->indices;
    } else {
	for (b = self->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);
    }
    
    if (!b) {
	return [super _get_];
    }

    /* If so recreate the array. */
    
    glBindBuffer(GL_ARRAY_BUFFER, b->name);
    glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

    array.type = b->type;
    array.free = FREE_BOTH;
    array.length = size;
    array.rank = b->size == 1 ? 1 : 2;
    array.size = malloc (array.rank * sizeof(int));

    array.size[0] = b->length;
    
    if (array.rank > 1) {
	array.size[1] = b->size;
    }

    /* Read back the data from the buffer object. */
    
    array.values.any = malloc (size);
    glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, array.values.any);

    array_pusharray(_L, &array);

    return 1;
}

-(int) _set_
{
    array_Array *array;
    shape_Buffer *b;
    const char *k;
    int isindices;

    k = lua_tostring (_L, 2);
    isindices = !strcmp(k, "indices");

    /* Check whether the given key already points to a buffer. */

    if (isindices) {
	b = self->indices;
    } else {
	for (b = self->buffers;
	     b && strcmp (b->key, k);
	     b = b->next);
    }

    /* If index data is provided as a normal Lua table attempt to
     * promote it to and unsigned integer array.  For vertex attribute
     * buffers promote to the default (double) type. */
    
    if (isindices && lua_type(_L, 3) == LUA_TTABLE) {
	array = array_testcompatible (_L, 3,
                                      ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                      ARRAY_TYPE, ARRAY_TUINT);
    } else {
	array = array_testarray (_L, 3);
    }

    /* Now if an array has been supplied assume it defines a new
     * buffer for the shape, otherwise treat it like a normal key. */

    if (array) {
	GLenum target;
        int streaming;
	
	/* Check if supplied data is sane. */
	
	if (isindices) {
	    /* Expect only unsigned integral types for index data. */

	    switch (array->type) {
	    case ARRAY_TUINT:
	    case ARRAY_TUSHORT:
	    case ARRAY_TUCHAR:
		break;
	    default:
		t_print_error("Array used for index data must be of "
			      "unsigned integral type.\n");
		abort();	    
		break;
	    }
	}

	if (array->rank > 2) {
	    t_print_error("Array used for vertex attribute data is of "
			  "unsuitable rank.\n");
	    abort();
	} else if (array->rank == 2 && array->size[1] < 2) {
	    t_print_error("Scalar vertex attributes should be specified "
			  "as arrays of rank 1.\n");
	    abort();
	}	    

        /* If no buffer exists create a new one, otherwise check
         * streaming requirements. */
        
        if (!b) {
            b = malloc (sizeof (shape_Buffer));
            b->key = strdup(k);

            glGenBuffers(1, &b->name);

            b->type = array->type;
            b->size = array->rank == 1 ? 1 : array->size[1];
            b->length = array->size[0];

            /* Link in the buffer. */
	
            if (isindices) {
                self->indices = b;
            } else {
                t_single_link_at_head(b, &self->buffers);
            }

            streaming = 0;
        } else {
            if (array->type != b->type) {
                t_print_error("Streamed vertex attributes data has different "
                              "type.\n");
                abort();
            }

            if ((array->rank == 1 && b->size != 1) ||
                (array->rank == 2 && b->size != array->size[1])) {
                t_print_error("Streamed vertex attributes data has different "
                              "size.\n");
                abort();
            }

            b->length = array->size[0];

            streaming = 1;
        }
        
	/* Create the buffer object and initialize it. */

	target = isindices ? GL_ELEMENT_ARRAY_BUFFER : GL_ARRAY_BUFFER;

	glBindBuffer(target, b->name);
	glBufferData(target, array->length, array->values.any,
		     GL_STATIC_DRAW);

	/* If the shape is already linked to a shader update the
	 * attribute's binding unless we're just streaming in new
	 * data.. */
	
	if (self->up && !streaming) {
	    Shader *shader;
	    int n, l, j, program;

	    shader = (Shader *)self->up;
	    program = shader->name;
	    
	    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);
	    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &l);
    
	    glBindVertexArray(self->name);

	    /* Look for the vertex attribute's index and match it to
	     * the specified buffer. */
	    
	    for (j = 0 ; j < n; j += 1) {
		char attribute[l];
		GLenum type;
		int size;
		
		glGetActiveAttrib (program, j, l, NULL, &size, &type,
				   attribute);

		if (!strcmp(attribute, b->key)) {
		    break;
		}
	    }				  
	
	    if (!b) {
		t_print_warning("%s node does not define vertex attribute "
				"'%s'.\n",
				[shader name], b->key);
	    }

	    match_attribute_to_buffer(program, j, b);
	}
	
	return 1;
    } else {
        /* If there was a buffer with this key but the currently
         * supplied value is not an array, free the buffer. */
    
        if (b) {
            if (isindices) {
                self->indices = NULL;
            } else {
                t_single_unlink_from(b, &self->buffers);
            }
    
            glDeleteBuffers(1, &b->name);
            free(b->key);
            free(b);	
        }
        
	return [super _set_];
    }
}

-(void) meetParent: (Shader *)parent
{
    shape_Buffer *b;
    int j, l, n;

    if (![parent isKindOf: [Shader class]]) {
	t_print_warning("%s node has no shader parent.\n",
			[self name]);
	
	return;
    }

    /* Once we're linked to a shader parent we can interrogate it to
     * find out what kind of data we're supposed to have in our
     * buffers. */

    glGetProgramiv(parent->name, GL_LINK_STATUS, &j);

    if (j != GL_TRUE) {
	return;
    }
    
    glGetProgramiv(parent->name, GL_ACTIVE_ATTRIBUTES, &n);
    glGetProgramiv(parent->name, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &l);

    /* Lower all the flags. */
    
    for (b = self->buffers;
	 b;
	 b->flag = 0, b = b->next);
    
    glBindVertexArray(self->name);
    
    for (j = 0 ; j < n ; j += 1) {
	char attribute[l];
	GLenum type;
	int size;

	glGetActiveAttrib (parent->name, j, l, NULL, &size, &type,
			   attribute);

	for (b = self->buffers;
	     b && strcmp(b->key, attribute);
	     b = b->next);
	
	if (!b) {
	    t_print_warning("%s node does not specify vertex attribute "
			    "'%s'.\n",
			    [self name], attribute);
	}

	if (b) {
	    match_attribute_to_buffer (parent->name, j, b);
	    b->flag = 1;
	}
    }

    for (b = self->buffers;
	 b;
	 b->flag = 0, b = b->next) {
	if (!b->flag) {
	    t_print_warning("%s node does not define vertex attribute "
			    "'%s'.\n",
			    [parent name], b->key);
	}
    }

    glBindVertexArray(0);
}

-(void) draw
{
    [super draw];
    
    /* Set the transform. */

    /* if (self->mode == GL_POINTS) */
    /* 	_TRACEM(4, 4, "f", self->matrix); */

    if (self->buffers) {
	t_push_modelview (self->matrix, T_MULTIPLY);

	/* Bind the vertex array and draw the supplied indices or the
	 * arrays if no indices we're supplied. */

	if (self->wireframe) {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (self->mode == GL_POINTS) {
	    glPointSize(3);
	}
    
	glBindVertexArray(self->name);

	if (self->indices) {
	    shape_Buffer *i;
	    GLenum type;

	    i = self->indices;
	
	    switch(i->type) {
	    case ARRAY_TUINT: type = GL_UNSIGNED_INT; break;
	    case ARRAY_TUSHORT: type = GL_UNSIGNED_SHORT; break;
	    case ARRAY_TUCHAR: type = GL_UNSIGNED_BYTE; break;
	    default: assert(0);
	    }
	
	    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, i->name);
	    glDrawRangeElements (self->mode, 0, self->buffers->length - 1,
				 i->size * i->length,
				 type, (void *)0);
	} else {
	    glDrawArrays (self->mode, 0, self->buffers->length);
	}

	if (self->wireframe) {
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	t_pop_modelview ();
    }
}

@end
