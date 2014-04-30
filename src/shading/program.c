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

#include "techne.h"
#include "program.h"

@implementation Program

-(int) _get_sources
{
    int i, n;
    unsigned int shaders[3] = {GL_INVALID_INDEX,
                               GL_INVALID_INDEX,
                               GL_INVALID_INDEX};

    glGetAttachedShaders(self->name, 3, &n, shaders);

    lua_newtable(_L);

    for (i = 0 ; i < n ; i += 1) {
        char *source;
        int l, type;

        glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &l);
        glGetShaderiv(shaders[i], GL_SHADER_TYPE, &type);
        source = malloc(l * sizeof (char));
        glGetShaderSource(shaders[i], l, NULL, source);

        lua_pushstring (_L, source);

        switch(type) {
        case GL_VERTEX_SHADER: lua_rawseti (_L, -2, 1);break;
        case GL_GEOMETRY_SHADER: lua_rawseti (_L, -2, 2);break;
        case GL_FRAGMENT_SHADER: lua_rawseti (_L, -2, 3);break;
        }

        free(source);
    }

    return 1;
}

-(void) _set_sources
{
    const char *source;

    lua_rawgeti (_L, 3, 1);
    if (lua_type (_L, -1) == LUA_TSTRING) {
        source = lua_tostring(_L, -1);
        [self addSourceString: source for: T_VERTEX_STAGE];
    }
    lua_pop(_L, 1);

    lua_rawgeti (_L, 3, 2);
    if (lua_type (_L, -1) == LUA_TSTRING) {
        source = lua_tostring(_L, -1);
        [self addSourceString: source for: T_GEOMETRY_STAGE];
    }
    lua_pop(_L, 1);

    lua_rawgeti (_L, 3, 3);
    if (lua_type (_L, -1) == LUA_TSTRING) {
        source = lua_tostring(_L, -1);
        [self addSourceString: source for: T_FRAGMENT_STAGE];
    }
    lua_pop(_L, 1);

    [self link];
}

-(int) call
{
    lua_pushvalue (_L, 1);
    [[ProgramShader alloc] init];
    t_configurenode(_L, 2);

    return 1;
}

@end

@implementation ProgramShader

-(void) draw: (int)frame
{
    glEnable (GL_CULL_FACE);
    glEnable (GL_DEPTH_TEST);

    glUseProgram(self->name);

    [self bind];
    [super draw: frame];

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_CULL_FACE);
}

@end
