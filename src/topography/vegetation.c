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
#include "splatting.h"
#include "shader.h"

static unsigned int queries[2];

@implementation Vegetation

-(void)init
{
    roam_Tileset *tiles;
    const double A = 0.0625;
    
    const char *private[] = {"base", "detail", "offset", "scale",
                             "factor", "references", "weights",
                             "resolutions", "clustering", "thresholds"};

    const char *header;
    ShaderMold *shader;
    int i, n, collect;
        
#include "glsl/color.h"
#include "glsl/rand.h"
#include "glsl/vegetation_vertex.h"
#include "glsl/vegetation_geometry.h"

    /* Make a reference to the elevation to make sure it's not
     * collected. */

    self->elevation = t_tonode (_L, -1);
    self->reference_1 = luaL_ref (_L, LUA_REGISTRYINDEX);
    
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

    /*****************************************************************/
    /* Calculate the seeding level: Assume we want to seed triangles */
    /* of area A.  The root triangles are of area                    */
    /*                                                               */
    /* A_0 = ((2^o + 1) r_x r_y) / 2                                 */
    /*                                                               */
    /* where o is the order and r_x,y the resolution.                */
    /*                                                               */
    /* Each level halves the area so a triangle of level i has area  */
    /*                                                               */
    /* A_i = A_0 / 2^i = ((2^o + 1) r_x r_y) / 2^(i + 1)             */
    /*                                                               */
    /* Setting A_i = A and simplifying 2^o + 1 to 2^o we get:        */
    /*                                                               */
    /* i = 2 o + log_2(r_x r_y / A) - 1                              */
    /*****************************************************************/

    tiles = &self->context->tileset;

    self->seeding.level = 2 * tiles->depth + log(tiles->resolution[0] * tiles->resolution[1] / A) / log(2) - 1;
    
    initialize_seeding(&self->seeding);

    n = self->elevation->swatches_n;
    self->species = calloc (n, sizeof(Shader *));

    /* Create the VBOS and feedback and vertex array objects. */

    self->vertexbuffers = malloc((n + 1) * sizeof(unsigned int));
    glGenBuffers(n + 1, self->vertexbuffers);
    glGenTransformFeedbacks(1, &self->feedback);
    
    self->arrays = malloc((n + 1) * sizeof(unsigned int));
    glGenVertexArrays (n + 1, self->arrays);
    glGenQueries(2, queries);

    /* Create the program. */

    [self unload];
    
    shader = [ShaderMold alloc];
        
    [shader initWithHandle: NULL];
    [shader declare: 10 privateUniforms: private];

    add_splatting_sources(self->elevation, shader, T_VERTEX_STAGE);

    asprintf((char **)&header,
             "%s\n"
             "const int SWATCHES = %d;\n",
             collect ? "#define COLLECT_STATISTICS\n" : "", n);
    
    [shader add: 4
            sourceStrings: (const char *[4]){
                header,
                glsl_color,
                glsl_rand,
                glsl_vegetation_vertex
            }
            for: T_VERTEX_STAGE];

    /* Render the geometry shader. */
    
    lua_createtable(_L, 0, 1);
    lua_pushinteger(_L, n);
    lua_setfield (_L, -2, "species");

    if(t_rendertemplate(_L, glsl_vegetation_geometry) != LUA_OK) {
        puts(lua_tostring(_L, -1));
        abort();
    }

    [shader addSourceString: lua_tostring(_L, -1) for: T_GEOMETRY_STAGE];

    lua_pop(_L, 1);

    {
        const char *varyings[8 * n];

        for (i = 0 ; i < n ; i += 1) {
            asprintf ((char **)&varyings[8 * i + 0], "stream_%d_apex_g", i);
            asprintf ((char **)&varyings[8 * i + 1], "stream_%d_left_g", i);
            asprintf ((char **)&varyings[8 * i + 2], "stream_%d_right_g", i);
            asprintf ((char **)&varyings[8 * i + 3], "stream_%d_color_g", i);
            asprintf ((char **)&varyings[8 * i + 4], "stream_%d_distance_g", i);
            asprintf ((char **)&varyings[8 * i + 5], "stream_%d_clustering_g", i);
            asprintf ((char **)&varyings[8 * i + 6], "stream_%d_instance_g", i);
            varyings[8 * i + 7] = "gl_NextBuffer";
        }

        /* Submit all varyings but the last gl_NextBuffer (which means
         * the -1 is intenional). */
        
        glTransformFeedbackVaryings(shader->name, 8 * n - 1, varyings,
                                    GL_INTERLEAVED_ATTRIBS);
    }

    [shader link];
    [self load];

    /* Bind the vertex buffer to the transform feedback object. */

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);

    for (i = 0 ; i < n ; i += 1) {
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i,
                         self->vertexbuffers[i + 1]);
    }

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
    
    self->locations.thresholds = glGetUniformLocation(self->name, "thresholds");
    self->locations.offset = glGetUniformLocation(self->name, "offset");
    self->locations.clustering = glGetUniformLocation(self->name, "clustering");

    self->units.base = [self getUnitForSamplerUniform: "base"];

    set_splatting_uniforms(self->elevation, self);

    glUseProgram(self->name);
    
    for (i = 0 ; i < self->elevation->swatches_n ; i += 1) {
        glUniform1f (self->locations.thresholds + i, 1.0 / 0.0);
    }
}

-(void)free
{
    int n;
    
    n = self->elevation->swatches_n;

    /* Free the VBOs, vertex arrays and transform feedback object and
     * associated buffers. */
    
    glDeleteBuffers(n + 1, self->vertexbuffers);
    glDeleteVertexArrays (n + 1, self->arrays);
    glDeleteTransformFeedbacks(1, &self->feedback);

    free(self->vertexbuffers);
    free(self->arrays);
    free(self->species);

    luaL_unref(_L, LUA_REGISTRYINDEX, self->reference_1);

    [super free];
}

-(void) adopt: (VegetationSpecies *)child
{
    if ([child isKindOf: [VegetationSpecies class]]) {
        int i, j;

        j = (int)child->key.number;

        if ((double)j != child->key.number) {
            t_print_warning("%s node has non-integer key.\n",
                            [child name]);
	
            return;
        }

        self->species[j - 1] = child;

        /* { */
        /*     int k, l; */

        /*     glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, &k); */
        /*     glGetIntegerv(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, &l); */
        /*     _TRACE ("%d, %d\n", k, l); */
        /* } */

        /* Initialize the vertex array object. */
    
        glBindVertexArray(self->arrays[j]);
        glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[j]);

        i = glGetAttribLocation(child->name, "apex");
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                              (void *)0);
        glEnableVertexAttribArray(i);

        i = glGetAttribLocation(child->name, "left");
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                              (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(i);

        i = glGetAttribLocation(child->name, "right");
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                              (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(i);

        i = glGetAttribLocation(child->name, "color");
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                              (void *)(9 * sizeof(float)));
        glEnableVertexAttribArray(i);    

        i = glGetAttribLocation(child->name, "distance");
        glVertexAttribPointer(i, 1, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                              (void *)(12 * sizeof(float)));
        glEnableVertexAttribArray(i);    

        i = glGetAttribLocation(child->name, "clustering");
        glVertexAttribPointer(i, 1, GL_FLOAT, GL_FALSE, TRANSFORMED_SEED_SIZE,
                               (void *)(13 * sizeof(float)));
        glEnableVertexAttribArray(i);    

        i = glGetAttribLocation(child->name, "instance");
        glVertexAttribIPointer(i, 1, GL_UNSIGNED_INT, TRANSFORMED_SEED_SIZE,
                               (void *)(14 * sizeof(float)));
        glEnableVertexAttribArray(i);    
    }

    [super adopt: child];
}

-(void) abandon: (Shader *)child
{
    int i, j;

    if ([child isKindOf: [VegetationSpecies class]]) {
        j = (int)child->key.number;

        self->species[j - 1] = NULL;

        /* Reset the vertex array object. */

        glBindVertexArray(self->arrays[j]);

        for (i = 0 ; i < GL_MAX_VERTEX_ATTRIBS - 1 ; i += 1) {
            glDisableVertexAttribArray(i);
        }
    }

    [super abandon: child];
}

-(void) draw: (int)frame
{
    roam_Tileset *tiles;
    seeding_Context *seeds;
    seeding_Bin *b;
    int i, j;

    seeds = &self->seeding;
    tiles = &self->context->tileset;
    
    t_push_modelview (self->matrix, T_MULTIPLY);
    
    begin_seeding (seeds, self->context);

    for (i = 0, b = &seeds->bins[0] ; i < BINS_N ;i += 1, b += 1) {
        b->triangles = 0;
        b->clusters = 0;
    }

    glPatchParameteri(GL_PATCH_VERTICES, 1);

    /* Seed all tiles. */

    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
            int k, l, n;
            static int highwater[2] = {8, 1024};

            seed_tile(l = i * tiles->size[1] + j);

            /* Calculate buffer size requirements. */
    
            for (k = 0, n = 0, b = &seeds->bins[0];
                 k < BINS_N;
                 k += 1, b += 1) {
                int c;

                while (b->fill > highwater[0]) {
                    highwater[0] *= 2;
                }

                c = (int)round(b->center / seeds->clustering);

                if (c > 1) {
                    n += c * b->fill;
                    b->clusters += c * b->fill;
                } else {
                    n += b->fill;
                    b->clusters += b->fill;
                }

                b->triangles += b->fill;
            }

            if (n == 0) {
                continue;
            } else while (n > highwater[1]) {
                highwater[1] *= 2;
            }

            /* Request a new block for the transformed seeds of each
             * species. */

            for (k = 0 ; k < self->elevation->swatches_n ; k += 1) {
                if (!self->species[k]) {
                    continue;
                }

                glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[k + 1]);
                glBufferData (GL_ARRAY_BUFFER,
                              highwater[1] * TRANSFORMED_SEED_SIZE,
                              NULL, GL_STREAM_COPY);                
            }

            /* Bind the first stage shader. */
            
            glUseProgram(self->name);
            [self bind];
            
            glEnable (GL_RASTERIZER_DISCARD);
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, self->feedback);
            glBeginTransformFeedback(GL_POINTS);
            glBeginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 0,
                                queries[0]);
            glBeginQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1,
                                queries[1]);
    
            glBindBuffer(GL_ARRAY_BUFFER, self->vertexbuffers[0]);
            glBindVertexArray(self->arrays[0]);
            glActiveTexture(GL_TEXTURE0 + self->units.base);

            /* Go through all the bins and submit triangles for
             * seeding. */
            
            for (k = 0, b = &seeds->bins[0];
                 k < BINS_N;
                 k += 1, b += 1) {

                if(b->fill > 0) {
                    double r, C;
                    
                    glUniform2f(self->locations.offset,
                                -i + 0.5 * tiles->size[0],
                                -j + 0.5 * tiles->size[1]);
                    
                    glBindTexture(GL_TEXTURE_2D, tiles->imagery[l]);

                    /* Orphan the buffer and allocate a new piece of
                     * memory equal to the largest bin capacity. */
                    
                    glBufferData (GL_ARRAY_BUFFER, highwater[0] * SEED_SIZE,
                                  NULL, GL_STREAM_DRAW);
                    glBufferSubData (GL_ARRAY_BUFFER, 0,
                                     b->fill * SEED_SIZE, b->buffer);

                    C = seeds->clustering;
                    r = round(b->center / C);

                    if (r > 1) {
                        glUniform1f(self->locations.clustering, C);
                        glDrawArraysInstanced(GL_POINTS, 0, b->fill, r);
                    } else {
                        glUniform1f(self->locations.clustering,
                                    round(b->center));
                        glDrawArraysInstanced(GL_POINTS, 0, b->fill, 1);
                    }
                }
            }

            /* Clean up after the first stage. */
            
            glEndTransformFeedback();
            glEndQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 0);
            glEndQueryIndexed(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1);
            glDisable (GL_RASTERIZER_DISCARD);

            /* unsigned int N[2]; */
            /* glGetQueryObjectuiv(queries[0], GL_QUERY_RESULT, &N[0]); */
            /* glGetQueryObjectuiv(queries[1], GL_QUERY_RESULT, &N[1]); */
            /* /\* _TRACE ("%d, %d, %d, %d\n", N[0], N[1], n, highwater[1]); *\/ */
            /* _TRACE ("%d\n", N[0] + N[1]); */
            /* assert(n < highwater[1]); */

            /* Bind the second stage shader and draw the seeds. */

            glEnable (GL_DEPTH_TEST);
            glEnable (GL_SAMPLE_ALPHA_TO_COVERAGE);
            glEnable (GL_SAMPLE_ALPHA_TO_ONE);
            glEnable (GL_MULTISAMPLE);
    
            for (k = 0 ; k < self->elevation->swatches_n ; k += 1) {
                Shader *shader;

                shader = self->species[k];

                if (!shader) {
                    continue;
                }
                
                glUseProgram(shader->name);
                [shader bind];

                glBindVertexArray(self->arrays[k + 1]);
                glDrawTransformFeedbackStream(GL_PATCHES, self->feedback, k);
            }

            glDisable (GL_DEPTH_TEST);
            glDisable (GL_SAMPLE_ALPHA_TO_COVERAGE);
            glDisable (GL_SAMPLE_ALPHA_TO_ONE);
            glDisable (GL_MULTISAMPLE);
            
	}
    }
    
    /* _TRACE ("error: %f\n", seeds->error); */
    
    finish_seeding();
    t_pop_modelview ();

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
        lua_pushinteger (_L, b->clusters);
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

@implementation VegetationSpecies

-(void) meetParent: (Vegetation *)parent
{
    [super meetParent: parent];
    
    if (![parent isKindOf: [Vegetation class]]) {
	t_print_warning("%s node has no Vegetation parent.\n",
			[self name]);
	
	return;
    }

    self->offset = (int)self->key.number;

    if ((double)self->offset != self->key.number) {
	t_print_warning("%s node has non-integer key.\n",
			[self name]);
	
	return;
    }

    /* Update the threshold. */
    
    glUseProgram(parent->name);
    glUniform1f (parent->locations.thresholds + self->offset - 1,
                 self->threshold);

    /* Update the canopy. */
    
    if(parent->elevation->context.canopy < self->bound) {
        parent->elevation->context.canopy = self->bound;
    }
}

-(void) missParent: (Vegetation *)parent
{
    VegetationSpecies *child;
    double h = 0;
    
    [super missParent: parent];

    /* Mark the swatch as infertile. */
        
    glUseProgram(parent->name);
    glUniform1f (parent->locations.thresholds + self->offset - 1, 1.0 / 0.0);

    /* Update the canopy. */

    for (child = (VegetationSpecies *)parent->down, h = 0;
         child;
         h = child != self && child->bound > h ? child->bound : h,
             child = (VegetationSpecies *)child->right);
    
    parent->elevation->context.canopy = h;
}

-(int) _get_threshold
{
    Vegetation *parent;
    float f;

    parent = (Vegetation *)self->up;

    if (parent) {
        glGetUniformfv (parent->name,
                        parent->locations.thresholds + self->offset - 1, &f);
        lua_pushnumber (_L, f);
    } else {
        lua_pushnil(_L);
    }
    
    return 1;
}

-(void) _set_threshold
{
    Vegetation *parent;

    self->threshold = lua_tonumber(_L, 3);
    parent = (Vegetation *)self->up;

    if (parent) {
        glUseProgram(parent->name);
        glUniform1f (parent->locations.thresholds + self->offset - 1,
                     self->threshold);
    }
}

-(void) setCanopy: (double)h
{
    Vegetation *parent = (Vegetation *)self->up;

    self->bound = h;

    if(parent && parent->elevation->context.canopy < self->bound) {
        parent->elevation->context.canopy = self->bound;
    }
}

@end
