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
#include "atmosphere.h"
#include "vegetation.h"
#include "shader.h"

static unsigned int query;

@implementation Vegetation

-(void)init
{
    const char *private[] = {"base", "detail", "offset", "scale",
                             "factor", "references", "weights",
                             "resolutions", "clustering"};
    char *header;
    ShaderMold *shader;
    int i, collect;
        
#include "glsl/color.h"
#include "glsl/splatting.h"
#include "glsl/rand.h"
#include "glsl/vegetation_vertex.h"
#include "glsl/vegetation_geometry.h"

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);

    self->species = malloc (self->elevation->swatches_n * sizeof(int));

    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        self->species[i] = LUA_REFNIL;
    }
    
    [super init];

    /* Are we profiling? */
    
    lua_getglobal (_L, "options");

    lua_getfield (_L, -1, "profile");
    collect = lua_toboolean (_L, -1);
    lua_pop (_L, 2);

    /* Create the feedback and vertex array objects. */

    glGenTransformFeedbacks(1, &self->feedback);
    glGenBuffers(1, &self->points);

#define TRANSFORMED_SEED_SIZE ((4 + 4 * 3 + 1) * sizeof(float) + 3 * sizeof(unsigned int))

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);
    glBindBuffer(GL_ARRAY_BUFFER, self->points);
    glBufferData(GL_ARRAY_BUFFER, 10000000 * TRANSFORMED_SEED_SIZE, NULL,
                 GL_STREAM_COPY);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, self->points);

    glGenVertexArrays (1, &self->vao);
    glGenQueries(1, &query);

    /* Create the program. */

    asprintf (&header, "const int N = %d;\n%s",
              self->elevation->swatches_n,
              collect ? "#define COLLECT_STATISTICS\n" : "");

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 9 privateUniforms: private];
    [shader add: 5
            sourceStrings: (const char *[5]){header, glsl_rand, glsl_color,
                                             glsl_splatting,
                                             glsl_vegetation_vertex}
            for: T_VERTEX_STAGE];

    [shader addSourceString: glsl_vegetation_geometry
                        for: T_GEOMETRY_STAGE];

    {
        const char *varyings[] = {"apex_g", "left_g", "right_g", "stratum_g",
                                  "color_g", "distance_g", "clustering_g",
                                  "chance_g"};
        
        glTransformFeedbackVaryings(shader->name, 8, varyings,
                                    GL_INTERLEAVED_ATTRIBS);
    }

    [shader link];
    [self load];

    /* Initialize uniforms. */

    glUseProgram(self->name);

    {
        unsigned int factor_l, references_l, weights_l, resolutions_l, separation_l;
        
        separation_l = glGetUniformLocation (self->name, "separation");
        factor_l = glGetUniformLocation (self->name, "factor");
        references_l = glGetUniformLocation (self->name, "references");
        weights_l = glGetUniformLocation (self->name, "weights");
        resolutions_l = glGetUniformLocation (self->name, "resolutions");

        glUniform1f (factor_l, self->elevation->albedo);
        glUniform1f (separation_l, self->elevation->separation);

        /* Initialize reference color uniforms. */
        
        for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
            elevation_SwatchDetail *swatch;
            int j;

            swatch = &self->elevation->swatches[i];


            for (j = 0 ; j < 3 ; j += 1) {
                [self setSamplerUniform: "detail"
                                     to: swatch->detail[j]->name
                                atIndex: 3 * i + j];

                glUniform2fv (resolutions_l + 3 * i + j, 1,
                              swatch->resolutions[j]);
            }
            
            glUniform3fv (references_l + i, 1, swatch->values);
            glUniform3fv (weights_l + i, 1, swatch->weights);
        }
    }
}

-(void)free
{
    int i;

    /* Free the transform feedback object and associated buffer. */
    
    glDeleteTransformFeedbacks(1, &self->feedback);
    glDeleteBuffers(1, &self->points);

    /* Free the vertex array. */
    
    glDeleteVertexArrays (1, &self->vao);
    
    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        luaL_unref(_L, LUA_REGISTRYINDEX, self->species[i]);
    }

    free(self->species);

    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(void) meetParent: (Shader *)parent
{
    int i;
    
    [super meetParent: parent];

    /* Initialize the vertex array object. */
    
    glBindVertexArray(self->vao);
    glBindBuffer(GL_ARRAY_BUFFER, self->points);

    i = glGetAttribLocation(parent->name, "apex");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)0);
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "left");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "right");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "stratum");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)(9 * sizeof(float)));
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "color");
    glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)(12 * sizeof(float)));
    glEnableVertexAttribArray(i);    

    i = glGetAttribLocation(parent->name, "distance");
    glVertexAttribPointer(i, 1, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                          (void *)(16 * sizeof(float)));
    glEnableVertexAttribArray(i);    

    i = glGetAttribLocation(parent->name, "clustering");
    glVertexAttribIPointer(i, 1, GL_UNSIGNED_INT, TRANSFORMED_SEED_SIZE,
                          (void *)(17 * sizeof(float)));
    glEnableVertexAttribArray(i);    

    i = glGetAttribLocation(parent->name, "chance");
    glVertexAttribIPointer(i, 2, GL_UNSIGNED_INT, TRANSFORMED_SEED_SIZE,
                           (void *)(17 * sizeof(float) + sizeof(unsigned int)));
    glEnableVertexAttribArray(i);    
}

-(int) _get_element
{
    return 0;
}

-(void) _set_element
{
    /* int n; */
    
    /* n = lua_tointeger(_L, 2); */
}

-(void) draw: (int)frame
{
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);
    glUseProgram(self->name);

    glEnable (GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);

    [super draw: frame];

    glEndTransformFeedback();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glDisable (GL_RASTERIZER_DISCARD);

    /* Draw the seeds. */
    
    glUseProgram(((Shader *)self->up)->name);
    [((Shader *)self->up) bind];
    /* glPointSize(3); */
    
    glBindVertexArray(self->vao);
    glDrawTransformFeedback(GL_PATCHES, self->feedback);

    /* { */
    /*     unsigned int n; */
    /*     glGetQueryObjectuiv(query, GL_QUERY_RESULT, &n); */
    /*     _TRACE ("%d\n", n); */
    /* } */
 }

@end
