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

#include <lua.h>
#include <lauxlib.h>
#include <string.h>

#include "array/array.h"
#include "transform.h"
#include "techne.h"

static double zero[3] = {0, 0, 0}, *origin = zero;

static void recurse (Node *root)
{
    Node *child, *next;

    t_begin_interval (root);

    if ([root isKindOf: [Transform class]]) {
	[(Transform *)root transform];
    } else {
	for (child = root->down ; child ; child = next) {
	    next = child->right;	    
	    recurse (child);
	}
    }
    
    t_end_interval (root);
}

@implementation Transform

-(void) init
{
    int i, j;
    
    [super init];

    self->transform = LUA_REFNIL;
    
    for (i = 0 ; i < 3 ; i += 1) {
	for (j = 0 ; j < 3 ; j += 1) {
	    self->orientation[i * 3 + j] = i == j ? 1 : 0;
	    self->rotation[i * 3 + j] = i == j ? 1 : 0;
	}

	self->position[i] = 0;
	self->translation[i] = 0;
    }
}

-(void) free
{
     luaL_unref (_L, LUA_REGISTRYINDEX, self->transform);

     [super free];
}

-(id) parentTransform
{
    Node *parent;

    for (parent = self->up;
	 parent && ![parent isKindOf: [Transform class]];
	 parent = parent->up);

    return parent;
}

-(void) transformCustom
{
    Node *child, *sister;
    
    /* Calculate the homogenous transform matrix. */
    
    self->matrix[0] = self->rotation[0];
    self->matrix[4] = self->rotation[1];
    self->matrix[8] = self->rotation[2];
    self->matrix[12] = self->translation[0];
    
    self->matrix[1] = self->rotation[3];
    self->matrix[5] = self->rotation[4];
    self->matrix[9] = self->rotation[5];
    self->matrix[13] = self->translation[1];
    
    self->matrix[2] = self->rotation[6];
    self->matrix[6] = self->rotation[7];
    self->matrix[10] = self->rotation[8];
    self->matrix[14] = self->translation[2];

    self->matrix[3] = 0;
    self->matrix[7] = 0;
    self->matrix[11] = 0;
    self->matrix[15] = 1;	    

    t_pushuserdata (_L, 1, self);
    t_callhook (_L, self->transform, 1, 0);

    /* Here we save the sister node beforehand because the child
       node might be unlinked by its hook function which will
       result in a prematurely ended traversal. */
    
    for (child = self->down ; child ; child = sister) {
	sister = child->right;
	recurse (child);
    }
}

-(void) transformToTranslation: (double *) r
                   andRotation: (double *) R
{
    int i;

    /* Just return the position and orientation. */
	
    for (i = 0 ; i < 3 ; i += 1) {
	self->translation[i] = r[i];
    }

    for (i = 0 ; i < 9 ; i += 1) {
	self->rotation[i] = R[i];
    }

    [self transformCustom];
}

-(void) transformAsRoot
{
    int i;

    /* Just return the position and orientation. */
	
    for (i = 0 ; i < 3 ; i += 1) {
	self->translation[i] = self->position[i] - origin[i];
    }

    for (i = 0 ; i < 9 ; i += 1) {
	self->rotation[i] = self->orientation[i];
    }

    [self transformCustom];
}

-(void) transformRelativeTo: (double *) p
{
    double *oldorigin;

    oldorigin = origin;
    origin = p;

    [self transformAsRoot];

    origin = oldorigin;
}

-(void) transform
{
    Transform *parent;
    double *r, *R, *p, *W;
    int i, j;

    parent = [self parentTransform];

    if (parent) {
	p = self->position;
	W = self->orientation;
    
	r = parent->translation;
	R = parent->rotation;

	for (i = 0 ; i < 3 ; i += 1) {
	    for (j = 0 ; j < 3 ; j += 1) {
		self->rotation[i * 3 + j] = R[i * 3] * W[j] +
		                            R[i * 3 + 1] * W[j + 3] +
		                            R[i * 3 + 2] * W[j + 6];
	    }

	    self->translation[i] = R[i * 3] * p[0] +
		                   R[i * 3 + 1] * p[1] +
		                   R[i * 3 + 2] * p[2] +
		                   r[i]; 
	}
    
	[self transformCustom];
    } else {
	[self transformAsRoot];
    }
}

-(int) _get_position
{
    array_createarray (_L, ARRAY_TDOUBLE, self->position, 1, 3);

    return 1;
}

-(int) _get_orientation
{
    array_createarray (_L, ARRAY_TDOUBLE, self->orientation, 2, 3, 3);

    return 1;
}

-(int) _get_translation
{
    array_createarray (_L, ARRAY_TDOUBLE, self->translation, 1, 3);

    return 1;
}

-(int) _get_rotation
{
    array_createarray (_L, ARRAY_TDOUBLE, self->rotation, 2, 3, 3);

    return 1;
}

-(int) _get_transform
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->transform);

    return 1;
}
 
-(void) _set_position
{
    array_Array *array;
    
    array = array_testcompatible (_L, 3, ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                  ARRAY_TDOUBLE, 1, 3);

    if (array) {
	memcpy (self->position, array->values.any, 3 * sizeof(double));
    }
}

-(void) _set_orientation
{
    array_Array *array;
    
    array = array_testcompatible (_L, 3, ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                  ARRAY_TDOUBLE, 2, 3, 3);

    if (array) {
	memcpy (self->orientation, array->values.any, 9 * sizeof(double));
    }
}
 
-(void) _set_translation
{
    T_WARN_READONLY;
}

-(void) _set_rotation
{
    T_WARN_READONLY;
}

-(void) _set_transform
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->transform);
    self->transform = luaL_ref (_L, LUA_REGISTRYINDEX);
}
 
@end
