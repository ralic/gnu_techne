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

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "array/array.h"
#include "techne.h"
#include "texture.h"

Texture *t_testtexture (lua_State *L, int index, GLenum target)
{
    Texture *object;

    object = t_testnode(_L, index, [Texture class]);

    if (!object) {
	return NULL;
    }

    if (object->target != target) {
	return NULL;
    }

    return object;
}

@implementation Texture

-(void)initWithTarget: (GLenum)target_in andName: (unsigned int)name_in
{
    self->target = target_in;
    self->name = name_in;
    
    [super init];
}

-(void)initWithTarget: (GLenum)target_in
{
    self->target = target_in;
    glGenTextures(1, &self->name);
    
    [super init];
}

-(void)free
{
    glDeleteTextures(1, &self->name);
    [super free];
}

-(int) _get_texels
{
    T_WARN_WRITEONLY;

    lua_pushnil(_L);
    
    return 1;
}

-(void) _set_texels
{
    array_Array *texels;
    
    if (!lua_isnil (_L, -1)) {
        GLenum type, format;
        
        texels = array_testarray (_L, -1);

        if (!texels) {
            t_print_error("Invalid value specified for texel data.\n");
            abort();
        }

        switch(target) {
        case GL_TEXTURE_2D:
            if (texels->rank != 3) {
                t_print_error("Texel data is of unsuitable rank.\n");
                abort();
            }
            break;
        }

        switch(texels->type) {
        case ARRAY_TUCHAR:case ARRAY_TNUCHAR:
            type = GL_UNSIGNED_BYTE;
            break;
        case ARRAY_TCHAR:case ARRAY_TNCHAR:
            type = GL_BYTE;
            break;
        default:
            t_print_error("Texel data specified for texture is of unsuitable type.\n");
            abort();
        }

        switch(texels->size[texels->rank - 1]) {
        case 1:
            format = GL_LUMINANCE;
            break;
        case 2:
            format = GL_LUMINANCE_ALPHA;
            break;
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            t_print_error("Texel data specified for texture is of unsuitable size.\n");
            abort();
        }

        /* Create the texture object. */
	
        glGetError();
        glBindTexture(self->target, self->name);
                    
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        switch(self->target) {
        case GL_TEXTURE_2D:
            glTexImage2D (self->target, 0, format,
                          texels->size[0], texels->size[1], 0,
                          format, type, texels->values.any);
            break;
        }

        glGenerateMipmap (self->target);

        glTexParameteri(self->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(self->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(self->target, 0);
    } else {
        glBindTexture(self->target, self->name);
        glTexImage2D (self->target, 0, GL_RGB,
                      0, 0, 0, GL_RGB,
                      GL_UNSIGNED_BYTE,
                      NULL);
    }
}

@end
