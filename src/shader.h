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
    int kind;
    
    unsigned int block;
    int type, size, offset, arraystride, matrixstride;
} shader_Basic;

typedef struct {
    int kind;
    
    unsigned int texture, unit;
    int location, reference;
    GLenum target;
} shader_Sampler; 

typedef union {
    int kind;
    
    shader_Basic basic;
    shader_Sampler sampler;
} shader_Uniform;

typedef enum {
    VERTEX_STAGE,
    GEOMETRY_STAGE,
    FRAGMENT_STAGE
} shader_Stage;

typedef enum {
    PRIVATE_UNIFORM,
    BASIC_UNIFORM,
    SAMPLER_UNIFORM
} shader_UniformKind;

@interface ShaderMold: Node {
@public
    ShaderMold **handle;

    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    int blocks_n, uniforms_n;

    const char **private;
    int private_n;
}

-(void) initWithHandle: (ShaderMold **)handle;
-(void) declare: (int) n privateUniforms: (const char **)names;
-(void) addSource: (const char *) source for: (shader_Stage)stage;
-(void) link;

@end

@interface Shader: Graphic {
@public
    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    int blocks_n, uniforms_n, reference;
}

@end

int t_add_global_block (const char *name, const char *declaration,
                        shader_Stage stage);

#endif
