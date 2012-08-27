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
#include "transform.h"

typedef struct shape_Buffer {
    struct shape_Buffer *next;
    
    char *key;
    unsigned int name, index;

    array_Type type;
    int size, length;
} shape_Buffer;

@interface Shape: Transform {
@public
    shape_Buffer *buffers, *indices;
    unsigned int name;
    GLenum mode;
}

-(Shape *)initWithMode: (GLenum) mode;

@end

#endif
