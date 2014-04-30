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

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include "gl.h"

#include "array/array.h"
#include "node.h"

@interface Texture: Node {
@public
    unsigned int name;
    GLenum target;

    struct {
        int rank;
        GLenum format;
        array_Type type;
    } template;
}

-(void)initWithTarget: (GLenum)target_in andName: (unsigned int)name_in;
-(void)initWithTarget: (GLenum)target_in;

-(int) _get_texels;
-(void) _set_texels;
-(int) _get_wrap;
-(void) _set_wrap;
-(int) _get_filter;
-(void) _set_filter;
-(int) _get_lod;
-(void) _set_lod;

@end

Texture *t_testtexture (lua_State *L, int index, GLenum target_in);
Texture *t_totexture (lua_State *L, int index, GLenum target);

#endif
