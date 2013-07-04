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

-(int) _get_wrap
{
    const GLenum parameters[] = {GL_TEXTURE_WRAP_S,
                                 GL_TEXTURE_WRAP_T,
                                 GL_TEXTURE_WRAP_R};
    int i, mode;
    
    lua_createtable(_L, 3, 0);

    for (i = 0 ; i < 3 ; i += 1) {
        glBindTexture(self->target, self->name);
        glGetTexParameteriv(self->target, parameters[i], &mode);

        switch (mode) {
        case GL_CLAMP_TO_EDGE: lua_pushliteral(_L, "clamp"); break;
        case GL_REPEAT: lua_pushliteral(_L, "repeat"); break;
        case GL_MIRRORED_REPEAT: lua_pushliteral(_L, "mirror"); break;
        default: assert(0);
        }

        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_wrap
{
    const GLenum parameters[] = {GL_TEXTURE_WRAP_S,
                                 GL_TEXTURE_WRAP_T,
                                 GL_TEXTURE_WRAP_R};
    const char *mode;
    size_t l;
    int i;

    if (lua_istable(_L, 3)) {
        glBindTexture(self->target, self->name);

        for (i = 0 ; i < 3 ; i += 1) {
            lua_rawgeti(_L, 3, i + 1);
            mode = lua_tolstring(_L, -1, &l);

            if (mode) {
                if (!xstrnlcmp("clamp", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_CLAMP_TO_EDGE);
                } else if (!xstrnlcmp("repeat", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_REPEAT);
                } else if (!xstrnlcmp("mirror", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_MIRRORED_REPEAT);
                } else {
                    t_print_warning("Invalid texture wrapping mode '%s'.\n",
                                    mode);
                }
            }

            lua_pop(_L, 1);
        }
    }
}

-(int) _get_lod
{
    float f;
    int i;
    
    lua_createtable(_L, 3, 0);

    glBindTexture(self->target, self->name);

    glGetTexParameteriv(self->target, GL_TEXTURE_MIN_LOD, &i);
    lua_pushinteger(_L, i);
    lua_rawseti(_L, -2, 1);

    glGetTexParameteriv(self->target, GL_TEXTURE_MAX_LOD, &i);
    lua_pushinteger(_L, i);
    lua_rawseti(_L, -2, 2);

    glGetTexParameterfv(self->target, GL_TEXTURE_LOD_BIAS, &f);
    lua_pushnumber(_L, f);
    lua_rawseti(_L, -2, 3);

    return 1;
}

-(void) _set_lod
{
    if (lua_istable(_L, 3)) {
        glBindTexture(self->target, self->name);
        
        lua_rawgeti(_L, 3, 1);
        if(lua_isnumber(_L, -1)) {
            glTexParameteri(self->target, GL_TEXTURE_MIN_LOD,
                            lua_tointeger (_L, -1));
        }
        lua_pop(_L, 1);

        lua_rawgeti(_L, 3, 2);
        if(lua_isnumber(_L, -1)) {
            glTexParameteri(self->target, GL_TEXTURE_MAX_LOD,
                            lua_tointeger (_L, -1));
        }
        lua_pop(_L, 1);

        lua_rawgeti(_L, 3, 3);
        if(lua_isnumber(_L, -1)) {
            glTexParameterf(self->target, GL_TEXTURE_LOD_BIAS,
                            lua_tonumber (_L, -1));
        }
        lua_pop(_L, 1);
    }
}

-(int) _get_filter
{
    const GLenum parameters[] = {GL_TEXTURE_MIN_FILTER,
                                 GL_TEXTURE_MAG_FILTER};
    int i, mode;
    
    lua_createtable(_L, 3, 0);

    for (i = 0 ; i < 2 ; i += 1) {
        glBindTexture(self->target, self->name);
        glGetTexParameteriv(self->target, parameters[i], &mode);

        switch (mode) {
        case GL_NEAREST: lua_pushliteral(_L, "nearest"); break;
        case GL_LINEAR: lua_pushliteral(_L, "linear"); break;
        case GL_NEAREST_MIPMAP_NEAREST:
            lua_pushliteral(_L, "nearest-mipmap-nearest"); break;
        case GL_LINEAR_MIPMAP_NEAREST:
            lua_pushliteral(_L, "linear-mipmap-nearest"); break;
        case GL_NEAREST_MIPMAP_LINEAR:
            lua_pushliteral(_L, "nearest-mipmap-linear"); break;
        case GL_LINEAR_MIPMAP_LINEAR:
            lua_pushliteral(_L, "linear-mipmap-linear"); break;
        default: assert(0);
        }

        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_filter
{
    const GLenum parameters[] = {GL_TEXTURE_MIN_FILTER,
                                 GL_TEXTURE_MAG_FILTER};
    const char *mode;
    size_t l;
    int i;

    if (lua_istable(_L, 3)) {
        glBindTexture(self->target, self->name);

        for (i = 0 ; i < 2 ; i += 1) {
            lua_rawgeti(_L, 3, i + 1);
            mode = lua_tolstring(_L, -1, &l);

            if (mode) {
                if (!xstrnlcmp("nearest", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_NEAREST);
                } else if (!xstrnlcmp("linear", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_LINEAR);
                } else if (!xstrnlcmp("nearest-mipmap-nearest", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_NEAREST_MIPMAP_NEAREST);
                } else if (!xstrnlcmp("linear-mipmap-nearest", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_LINEAR_MIPMAP_NEAREST);
                } else if (!xstrnlcmp("nearest-mipmap-linear", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_NEAREST_MIPMAP_LINEAR);
                } else if (!xstrnlcmp("linear-mipmap-linear", mode, l)) {
                    glTexParameteri(self->target, parameters[i],
                                    GL_LINEAR_MIPMAP_LINEAR);
                } else {
                    t_print_warning("Invalid texture filtering mode '%s'.\n",
                                    mode);
                }
            }

            lua_pop(_L, 1);
        }
    }
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

        switch(self->target) {
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
            format = GL_R;
            break;
        case 2:
            format = GL_RG;
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
        default:
            t_print_error("Unsupported texture target.\n");
            abort();
        }

        glGenerateMipmap (self->target);
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
