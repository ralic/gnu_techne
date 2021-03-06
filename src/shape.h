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

#ifndef _SHAPE_H_
#define _SHAPE_H_

#include <lua.h>

#include "gl.h"

#include "array/array.h"
#include "graphic.h"

typedef struct shape_Buffer {
    struct shape_Buffer *next;

    char *key;
    unsigned int name, index;

    array_Type type;
    int size;                   /* The number of elements per vertex. */
    int length;                 /* The number of vertices. */
    int flag;
} shape_Buffer;

@interface Shape: Graphic {
@public
    shape_Buffer *buffers, *indices;
    unsigned int name;
    GLenum mode;
}

-(void)initWithMode: (GLenum) mode;

@end

#endif
