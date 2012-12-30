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

#ifndef _SHADER_H_
#define _SHADER_H_

#include <lua.h>
#include "gl.h"
#include "graphic.h"

typedef struct {
    unsigned int block;
    int type, size, offset, arraystride, matrixstride;
} shader_Uniform;

typedef struct {
    unsigned int texture, unit;
    int location;
    GLenum target;
} shader_Sampler;

typedef enum {
    VERTEX_STAGE,
    GEOMETRY_STAGE,
    FRAGMENT_STAGE
}  shader_Stage;

@interface Shader: Graphic {
@public
    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    shader_Sampler *samplers;
    int blocks_n, samplers_n, ismold;
}


+(int) addUniformBlockNamed: (const char *)name
                   forStage: (shader_Stage)stage
                 withSource: (const char *)declaration;
-(void) addSource: (const char *) source for: (shader_Stage)stage;
-(void) link;
-(void)initFrom: (Shader *) mold;

@end

#endif
