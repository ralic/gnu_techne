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
#include <math.h>

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

    if (target != 0 && object->target != target) {
        return NULL;
    }

    return object;
}

Texture *t_totexture (lua_State *L, int index, GLenum target)
{
    Texture *texture;

    texture = t_testtexture(L, index, target);

    if(!texture) {
        lua_pushfstring (L,
                         "Texture expected, got %s.",
                         lua_typename (L, lua_type (L, index)));
        lua_error (L);
    }

    return texture;
}

static void map_array_to_texture (array_Array *array,
                                  GLenum *gltype, GLenum *internal,
                                  GLenum *format)

{
    const GLenum formats[][3][5][2] = {
        /* R */

        {
            {
                {GL_R32F, GL_RED}, {GL_NONE, GL_NONE},
                {GL_R32I, GL_RED_INTEGER}, {GL_R32UI, GL_RED_INTEGER},
                {GL_NONE, GL_NONE}
            },

            {
                {GL_R16F, GL_RED}, {GL_R16, GL_RED},
                {GL_R16I, GL_RED_INTEGER}, {GL_R16UI, GL_RED_INTEGER},
                {GL_R16_SNORM, GL_RED}
            },

            {
                {GL_NONE, GL_NONE}, {GL_R8, GL_RED},
                {GL_R8I, GL_RED_INTEGER}, {GL_R8UI, GL_RED_INTEGER},
                {GL_R8_SNORM, GL_RED}
            },
        },

        {
            /* RG */

            {
                {GL_RG32F, GL_RG}, {GL_NONE, GL_NONE},
                {GL_RG32I, GL_RG_INTEGER}, {GL_RG32UI, GL_RG_INTEGER},
                {GL_NONE, GL_NONE}
            },

            {
                {GL_RG16F, GL_RG}, {GL_RG16, GL_RG},
                {GL_RG16I, GL_RG_INTEGER}, {GL_RG16UI, GL_RG_INTEGER},
                {GL_RG16_SNORM, GL_RG}
            },

            {
                {GL_NONE, GL_NONE}, {GL_RG8, GL_RG},
                {GL_RG8I, GL_RG_INTEGER}, {GL_RG8UI, GL_RG_INTEGER},
                {GL_RG8_SNORM, GL_RG}
            },
        },

        {
            /* RGB */

            {
                {GL_RGB32F, GL_RGB}, {GL_NONE, GL_NONE},
                {GL_RGB32I, GL_RGB_INTEGER}, {GL_RGB32UI, GL_RGB_INTEGER},
                {GL_NONE, GL_NONE}
            },

            {
                {GL_RGB16F, GL_RGB}, {GL_RGB16, GL_RGB},
                {GL_RGB16I, GL_RGB_INTEGER}, {GL_RGB16UI, GL_RGB_INTEGER},
                {GL_RGB16_SNORM, GL_RGB}
            },

            {
                {GL_NONE, GL_NONE}, {GL_RGB8, GL_RGB},
                {GL_RGB8I, GL_RGB_INTEGER}, {GL_RGB8UI, GL_RGB_INTEGER},
                {GL_RGB8_SNORM, GL_RGB}
            },
        },

        {
            /* RGBA */

            {
                {GL_RGBA32F, GL_RGBA}, {GL_NONE, GL_NONE},
                {GL_RGBA32I, GL_RGBA_INTEGER}, {GL_RGBA32UI, GL_RGBA_INTEGER},
                {GL_NONE, GL_NONE}
            },

            {
                {GL_RGBA16F, GL_RGBA}, {GL_RGBA16, GL_RGBA},
                {GL_RGBA16I, GL_RGBA_INTEGER}, {GL_RGBA16UI, GL_RGBA_INTEGER},
                {GL_RGBA16_SNORM, GL_RGBA}
            },

            {
                {GL_NONE, GL_NONE}, {GL_RGBA8, GL_RGBA},
                {GL_RGBA8I, GL_RGBA_INTEGER}, {GL_RGBA8UI, GL_RGBA_INTEGER},
                {GL_RGBA8_SNORM, GL_RGBA}
            },
        }
    };

    int l, m, n;
    const GLenum *pair;

    switch(array->type) {
    case ARRAY_TUCHAR:
        *gltype = GL_UNSIGNED_BYTE; m = 2; n = 3; break;
    case ARRAY_TNUCHAR:
        *gltype = GL_UNSIGNED_BYTE; m = 2; n = 1; break;
    case ARRAY_TCHAR:
        *gltype = GL_BYTE; m = 2; n = 2; break;
    case ARRAY_TNCHAR:
        *gltype = GL_BYTE; m = 2; n = 4; break;
    case ARRAY_TUSHORT:
        *gltype = GL_UNSIGNED_SHORT; m = 1; n = 3; break;
    case ARRAY_TNUSHORT:
        *gltype = GL_UNSIGNED_SHORT; m = 1; n = 1; break;
    case ARRAY_TSHORT:
        *gltype = GL_SHORT; m = 1; n = 2; break;
    case ARRAY_TNSHORT:
        *gltype = GL_SHORT; m = 1; n = 4; break;
    case ARRAY_TUINT:
        *gltype = GL_UNSIGNED_INT; m = 0; n = 3; break;
    case ARRAY_TNUINT:
        *gltype = GL_UNSIGNED_INT; m = 0; n = 1; break;
    case ARRAY_TINT:
        *gltype = GL_INT; m = 0; n = 2; break;
    case ARRAY_TNINT:
        *gltype = GL_INT; m = 0; n = 4; break;
    case ARRAY_TFLOAT:
        *gltype = GL_FLOAT; m = 0; n = 0; break;
    default:
        t_print_error("Texel data specified for texture is of "
                      "unsuitable type.\n");
        abort();
    }

    l = array->size[array->rank - 1] - 1;
    pair = formats[l][m][n];

    *internal = pair[0];
    *format = pair[1];
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

    lua_createtable(_L, 3, 0);

    glBindTexture(self->target, self->name);

    glGetTexParameterfv(self->target, GL_TEXTURE_MIN_LOD, &f);
    lua_pushnumber(_L, f);
    lua_rawseti(_L, -2, 1);

    glGetTexParameterfv(self->target, GL_TEXTURE_MAX_LOD, &f);
    lua_pushnumber(_L, f);
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
            glTexParameterf(self->target, GL_TEXTURE_MIN_LOD,
                            lua_tonumber (_L, -1));
        }
        lua_pop(_L, 1);

        lua_rawgeti(_L, 3, 2);
        if(lua_isnumber(_L, -1)) {
            glTexParameterf(self->target, GL_TEXTURE_MAX_LOD,
                            lua_tonumber (_L, -1));
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
    int i, base, max, rank;

    glBindTexture(self->target, self->name);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glGetTexParameteriv(self->target, GL_TEXTURE_BASE_LEVEL, &base);
    glGetTexParameteriv(self->target, GL_TEXTURE_MAX_LEVEL, &max);

    switch(self->target)  {
    case GL_TEXTURE_2D: rank = 3; break;
    case GL_TEXTURE_3D: case GL_TEXTURE_2D_ARRAY: rank = 4; break;
    default: assert(0);
    }

    lua_newtable(_L);

    for (i = base ; i <= max ; i += 1) {
        GLenum internal, format, gltype;
        array_Type type;
        array_Array *array;
        int channels[4][2], size[4], n;

        switch(self->target) {
        case GL_TEXTURE_2D:
            glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_WIDTH,
                                     &size[0]);
            glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_HEIGHT,
                                     &size[1]);
        break;
        case GL_TEXTURE_3D: case GL_TEXTURE_2D_ARRAY:
            glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_DEPTH,
                                     &size[0]);
            glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_WIDTH,
                                     &size[1]);
            glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_HEIGHT,
                                     &size[2]);
            break;
        }

        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_RED_TYPE,
                                 &channels[0][0]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_RED_SIZE,
                                 &channels[0][1]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_GREEN_TYPE,
                                 &channels[1][0]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_GREEN_SIZE,
                                 &channels[1][1]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_BLUE_TYPE,
                                 &channels[2][0]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_BLUE_SIZE,
                                 &channels[2][1]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_ALPHA_TYPE,
                                 &channels[3][0]);
        glGetTexLevelParameteriv(self->target, i, GL_TEXTURE_ALPHA_SIZE,
                                 &channels[3][1]);

        /* Figure out the array type. */

        switch (channels[0][0]) {
        case GL_FLOAT: type = ARRAY_TFLOAT; break;
        case GL_SIGNED_NORMALIZED: type = ARRAY_TNCHAR; break;
        case GL_UNSIGNED_NORMALIZED: type = ARRAY_TNUCHAR; break;
        case GL_INT: type = ARRAY_TCHAR; break;
        case GL_UNSIGNED_INT: type = ARRAY_TUCHAR; break;
        default: assert(0);
        }

        if (type != ARRAY_TFLOAT) {
            n = channels[0][1] / 8;

            if (type > 0) {
                type -= 2 * ((n > 3 ? 3 : n) - 1);
            }  else {
                type += 2 * ((n > 3 ? 3 : n) - 1);
            }
        }

        /* Count the number of channels. */

        for(n = 0 ; channels[n][0] != GL_NONE ; n += 1);
        size[rank - 1] = n;

        array = array_createarrayv(_L, type, NULL, rank, size);
        map_array_to_texture(array, &gltype, &internal, &format);

        glGetTexImage(self->target, i, format, gltype, array->values.any);
        lua_rawseti(_L, 3, i + 1);
    }

    return 1;
}

-(void) _set_texels
{
    int i, n, max;

    /* If a single texel array is specified wrap it in a table and
     * default minification filtering to linear. */

    if (array_isarray (_L, 3)) {
        lua_createtable(_L, 1, 0);
        lua_insert(_L, 3);
        lua_rawseti(_L, 3, 1);

        glBindTexture(self->target, self->name);
        glTexParameteri(self->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (!lua_istable (_L, 3)) {
        t_print_error("Invalid value specified for texel data.\n");
        abort();
    }

    glBindTexture(self->target, self->name);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    lua_pushnil(_L);

    n = lua_rawlen(_L, 3);

    for (i = 0, max = 0 ; i < n ; i += 1) {
        array_Array *texels;
        GLenum type, internal, format;
        int h, allocated;

        h = lua_gettop(_L);
        lua_rawgeti(_L, 3, i + 1);

        texels = array_testarray (_L, -1);

        if (!texels) {
            t_print_error("Invalid value specified for texel data "
                          "at level %d.\n", i);
            abort();
        }

        if ((self->target == GL_TEXTURE_2D && texels->rank != 3) ||
            ((self->target == GL_TEXTURE_3D ||
              self->target == GL_TEXTURE_2D_ARRAY) && texels->rank != 4)) {
            t_print_error("Texel data at level %d is of unsuitable "
                          "rank.\n", i);
            abort();
        }

        map_array_to_texture(texels, &type, &internal, &format);

        /* _TRACEV (3, "d", texels->size); */
        /* _TRACE ("%d, %x, %x, %x\n", texels->type, type, internal,
           format); */

        /* Create the texture object. */
        glGetTexParameteriv (self->target, GL_TEXTURE_IMMUTABLE_FORMAT,
                             &allocated);

        if (!allocated) {
            int maxsize;

            assert (i == 0);

            switch(self->target) {
            case GL_TEXTURE_2D:
                maxsize = texels->size[0] > texels->size[1] ?
                    texels->size[0] : texels->size[1];

                glTexStorage2D (self->target,
                                (int)log2(maxsize) + 1,
                                internal,
                                texels->size[0], texels->size[1]);
                break;
            case GL_TEXTURE_2D_ARRAY:
                maxsize = texels->size[1] > texels->size[2] ?
                    texels->size[1] : texels->size[2];

                glTexStorage3D (self->target,
                                (int)log2(maxsize) + 1,
                                internal,
                                texels->size[1], texels->size[2],
                                texels->size[0]);
                break;
            case GL_TEXTURE_3D:
                maxsize = texels->size[1] > texels->size[2] ?
                    texels->size[1] : texels->size[2];
                maxsize = maxsize > texels->size[0] ?
                    maxsize : texels->size[0];

                glTexStorage3D (self->target,
                                (int)log2(maxsize) + 1,
                                internal,
                                texels->size[1], texels->size[2],
                                texels->size[0]);
                break;
            default:
                t_print_error("Unsupported texture target.\n");
                abort();
            }
        }

        switch(self->target) {
        case GL_TEXTURE_2D:
            glTexSubImage2D (self->target, i,
                             0, 0, texels->size[0], texels->size[1],
                             format, type, texels->values.any);
            break;
        case GL_TEXTURE_3D: case GL_TEXTURE_2D_ARRAY:
            glTexSubImage3D (self->target, i,
                             0, 0, 0,
                             texels->size[1], texels->size[2], texels->size[0],
                             format, type, texels->values.any);
            break;
        default:
            t_print_error("Unsupported texture target.\n");
            abort();
        }

        lua_pop(_L, 1);

        if (max < i) {
            max = i;
        }

        lua_settop(_L, h - 1);
    }

    glTexParameteri(self->target, GL_TEXTURE_MAX_LEVEL, max);
    glBindTexture(self->target, 0);
}

@end
