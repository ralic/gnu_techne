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
#include "techne.h"
#include "graphic.h"

typedef enum {
    SHADER_BASIC_UNIFORM,
    SHADER_SAMPLER_UNIFORM
} shader_UniformKind;

typedef enum {
    SHADER_PUBLIC_UNIFORM,
    SHADER_PROTECTED_UNIFORM,
    SHADER_PRIVATE_UNIFORM
} shader_UniformMode;

typedef struct {
    shader_UniformKind kind;
    shader_UniformMode mode;
} shader_Any;

typedef struct {
    shader_UniformKind kind;
    shader_UniformMode mode;
    
    unsigned int block;
    int type, size, offset, arraystride, matrixstride;
} shader_Basic;

typedef struct {
    shader_UniformKind kind;
    shader_UniformMode mode;
    
    unsigned int texture, unit;
    int location, reference;
    GLenum target;
} shader_Sampler; 

typedef union {
    shader_Any any;
    shader_Basic basic;
    shader_Sampler sampler;
} shader_Uniform;

@interface ShaderMold: Node {
@public
    ShaderMold **handle;

    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    int blocks_n, uniforms_n;

    const char **private;
    int private_n;
    int unit_0;                   /* The base texture unit for public
                                   * samplers. */
}

-(void) initWithHandle: (ShaderMold **)handle;
-(void) declare: (int) n privateUniforms: (const char **) names;
-(void) add: (const int) n sourceStrings: (const char **) strings
        for: (t_Enumerated)stage;
-(void) addSourceString: (const char *) source for: (t_Enumerated)stage;
-(void) addSourceFragement: (const char *) fragment for: (t_Enumerated)stage;
-(void) finishAssemblingSourceFor: (t_Enumerated)stage;
-(void) link;

@end

@interface Shader: Graphic {
@public
    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    int blocks_n, uniforms_n, reference;
}

-(void)load;
-(void)unload;

@end

int t_add_global_block (const char *name, const char *declaration);

#endif
