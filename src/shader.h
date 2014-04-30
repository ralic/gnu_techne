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
#include "texture.h"
#include "graphic.h"

typedef enum {
    SHADER_BASIC_UNIFORM,
    SHADER_SAMPLER_UNIFORM,
    SHADER_COUNTER_UNIFORM
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

    unsigned int index, unit;
    int location, size;
    GLenum target;
} shader_Sampler;

typedef struct {
    shader_UniformKind kind;
    shader_UniformMode mode;

    unsigned int offset, size, buffer;
} shader_Counter;

typedef union {
    shader_Any any;
    shader_Basic basic;
    shader_Sampler sampler;
    shader_Counter counter;
} shader_Uniform;

typedef struct {
    unsigned int name, *names;
    int reference;
} shader_Texture;

@interface ShaderMold: Node {
@public
    ShaderMold **handle;

    unsigned int name;
    shader_Uniform *uniforms;

    int blocks_n;               /* Number of uniform blocks. */
    int uniforms_n;             /* Number of uniform variables. */
    int buffers_n;              /* Number of atomic counter buffers. */
    int samplers_n;             /* Number of sampler uniforms. */

    const char **private;
    int private_n;
}

-(void) initWithHandle: (ShaderMold **)handle_in;
-(void) declare: (int) n privateUniforms: (const char **) names;
-(void) add: (const int) n sourceStrings: (const char **) strings
        for: (t_ProcessingStage)stage;
-(void) addSourceString: (const char *) source for: (t_ProcessingStage)stage;
-(void) addSourceFragement: (const char *) fragment
                       for: (t_ProcessingStage)stage;
-(void) finishAssemblingSourceFor: (t_ProcessingStage)stage;
-(void) link;

@end

@interface Shader: Graphic {
@public
    unsigned int *blocks, *buffers, name;
    shader_Uniform *uniforms;
    shader_Texture *textures;
    int public_buffer, blocks_n, uniforms_n, buffers_n, samplers_n;
    int reference;
}

-(void) load;
-(void) unload;
-(void) bind;

@end

int t_add_global_block (const char *name, const char *declaration);

unsigned int t_sampler_unit(Shader *shader, const char *uniform_name);
void t_set_sampler(Shader *shader, const char *uniform_name,
                   unsigned int texture_name);
void t_set_indexed_sampler(Shader *shader, const char *uniform_name,
                           unsigned int texture_name, int j);
void t_reset_counter(Shader *shader, const char *uniform_name);
void t_get_counter(Shader *shader, const char *uniform_name,
                   unsigned int *n);
void t_get_and_reset_counter(Shader *shader, const char *uniform_name,
                             unsigned int *n);

#endif
