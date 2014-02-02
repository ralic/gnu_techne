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

    /* Initialize seeding. */
    
    self->context = &self->elevation->context;
    self->seeding.horizon = -100;
    self->seeding.density = 10000;
    self->seeding.ceiling = 1000;
    self->seeding.clustering = 1.0;

    initialize_seeding(&self->seeding);

    /* Create the VBOS and feedback and vertex array objects. */

    glGenBuffers(2, self->vertexbuffers);
    glGenTransformFeedbacks(1, &self->feedback);

#define TRANSFORMED_SEED_SIZE ((4 + 4 * 3 + 1) * sizeof(float) + 3 * sizeof(unsigned int))

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, 10000000 * TRANSFORMED_SEED_SIZE, NULL,
                 GL_STREAM_COPY);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, self->vertexbuffers[1]);

    glGenVertexArrays (2, self->arrays);
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

    /* Bind the VBOs into the VAO. */
    
    glBindVertexArray(self->arrays[0]);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[0]);

    i = glGetAttribLocation(self->name, "apex");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)0);
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(self->name, "left");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(self->name, "right");
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, SEED_SIZE, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(i);

    /* Initialize uniforms. */

    glUseProgram(self->name);

   
    self->locations.scale = glGetUniformLocation(self->name, "scale");
    self->locations.offset = glGetUniformLocation(self->name, "offset");
    self->locations.clustering = glGetUniformLocation(self->name, "clustering");
    self->locations.instances = glGetUniformLocation(self->name, "instances");

    self->units.base = [self getUnitForSamplerUniform: "base"];

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

    /* Free the VBOs and transform feedback object and associated
     * buffer. */
    
    glDeleteBuffers(2, self->vertexbuffers);
    glDeleteTransformFeedbacks(1, &self->feedback);

    /* Free the vertex array. */
    
    glDeleteVertexArrays (2, self->arrays);
    
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
    
    glBindVertexArray(self->arrays[1]);
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[1]);

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
    double q;
    roam_Tileset *tiles;
    seeding_Bin *b;
    int i, j;

    tiles = &self->context->tileset;

    glUseProgram(self->name);
    [self bind];

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);
    glEnable (GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);

    t_push_modelview (self->matrix, T_MULTIPLY);

    /* Seed the vegetation. */
    
    begin_seeding (&self->seeding, self->context);
    
    glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[0]);
    glBindVertexArray(self->arrays[0]);
    glActiveTexture(GL_TEXTURE0 + self->units.base);

    glPatchParameteri(GL_PATCH_VERTICES, 1);
    
    q = ldexpf(1, -tiles->depth);
    glUniform2f(self->locations.scale,
                q / tiles->resolution[0], q / tiles->resolution[1]);
    
#ifdef RASTERTIZER_DISCARD
    glEnable (GL_RASTERIZER_DISCARD);
#endif

    for (i = 0, b = &self->seeding.bins[0];
         i < BINS_N;
         i += 1, b += 1) {
        b->patches = 0;
    }
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            int k, l, n;

            l = i * tiles->size[1] + j;
            n = seed_tile(l);
            
            for (k = 0, b = &self->seeding.bins[0];
                 k < BINS_N;
                 k += 1, b += 1) {

                if(b->fill > 0) {
                    double r, C;
                    
                    glUniform2f(self->locations.offset,
                                -i + 0.5 * tiles->size[0],
                                -j + 0.5 * tiles->size[1]);
                    
                    glBindTexture(GL_TEXTURE_2D, tiles->imagery[l]);
                    glPointSize(1);

                    glBufferData (GL_ARRAY_BUFFER, n * SEED_SIZE, NULL,
                                  GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0,
                                     b->fill * SEED_SIZE, b->buffer);

                    C = self->seeding.clustering;
                    r = round(b->center / C);

                    if (r >= 1.0) {
                        glUniform1f(self->locations.clustering, C);
                        glUniform1f(self->locations.instances, k == BINS_N - 1 ? 1.0 / 0.0 : r);
                        glDrawArraysInstanced(GL_POINTS, 0, b->fill, r);

                        b->patches += b->fill * r;
                    } else {
                        glUniform1f(self->locations.clustering,
                                    round(b->center));
                        glUniform1f(self->locations.instances, 1);
                        glDrawArraysInstanced(GL_POINTS, 0, b->fill, 1);
                        
                        b->patches += b->fill;
                    }
                }
            }
	}
    }
    
    /* _TRACE ("error: %f\n", seeding->error); */

#ifdef RASTERTIZER_DISCARD
    glDisable (GL_RASTERIZER_DISCARD);
#endif

    finish_seeding();
    t_pop_modelview ();

    glEndTransformFeedback();
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glDisable (GL_RASTERIZER_DISCARD);

    /* Draw the seeds. */

    glEnable (GL_DEPTH_TEST);
    glEnable (GL_SAMPLE_ALPHA_TO_COVERAGE);
    glEnable (GL_SAMPLE_ALPHA_TO_ONE);
    
    glEnable (GL_MULTISAMPLE);
    
    glUseProgram(((Shader *)self->up)->name);
    [((Shader *)self->up) bind];
    
    glBindVertexArray(self->arrays[1]);
    glDrawTransformFeedback(GL_PATCHES, self->feedback);

    /* { */
    /*     unsigned int n; */
    /*     glGetQueryObjectuiv(query, GL_QUERY_RESULT, &n); */
    /*     _TRACE ("%d\n", n); */
    /* } */

    glDisable (GL_DEPTH_TEST);
    glDisable (GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable (GL_SAMPLE_ALPHA_TO_ONE);

    glDisable (GL_MULTISAMPLE);

    [super draw: frame];
}

-(int) _get_density
{
    lua_pushnumber(_L, self->seeding.density);

    return 1;
}

-(void) _set_density
{
    self->seeding.density = lua_tonumber(_L, 3);
}

-(int) _get_ceiling
{
    lua_pushnumber(_L, self->seeding.ceiling);

    return 1;
}

-(void) _set_ceiling
{
    self->seeding.ceiling = lua_tonumber(_L, 3);
}

-(int) _get_clustering
{
    lua_pushnumber (_L, self->seeding.clustering);
    
    return 1;
}

-(void) _set_clustering
{
    self->seeding.clustering = lua_tonumber (_L, 3);
}

-(int) _get_bins
{
    int i;
    
    lua_createtable (_L, BINS_N, 0);

    for (i = 0 ; i < BINS_N ; i += 1) {
        seeding_Bin *b;

        b = &self->seeding.bins[i];
        
        lua_createtable (_L, 4, 0);
        lua_pushnumber (_L, b->center);
        lua_rawseti (_L, -2, 1);
        lua_pushinteger (_L, b->triangles);
        lua_rawseti (_L, -2, 2);
        lua_pushinteger (_L, b->patches);
        lua_rawseti (_L, -2, 3);
        lua_pushinteger (_L, b->capacity);
        lua_rawseti (_L, -2, 4);

        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_bins
{
    T_WARN_READONLY;
}

-(int) _get_triangles
{
    int i;
    
    lua_createtable (_L, 2, 0);

    for (i = 0 ; i < 2 ; i += 1) {
        lua_pushnumber (_L, self->seeding.triangles_n[i]);
        lua_rawseti (_L, -2, i + 1);
    }
    
    return 1;
}

-(void) _set_triangles
{
    T_WARN_READONLY;
}

-(int) _get_horizon
{
    lua_pushnumber (_L, -self->seeding.horizon);
    
    return 1;
}

-(void) _set_horizon
{
    self->seeding.horizon = -lua_tonumber (_L, 3);
}

-(int) _get_error
{
    lua_pushnumber (_L, self->seeding.error);
    
    return 1;
}

-(void) _set_error
{
    T_WARN_READONLY;
}

@end
