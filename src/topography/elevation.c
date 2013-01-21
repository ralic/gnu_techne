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
#include <limits.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "array/array.h"
#include "structures.h"
#include "techne.h"
#include "algebra.h"
#include "shader.h"
#include "elevation.h"
#include "roam.h"

static int construct(lua_State *L)
{
    Class class;

    lua_pushvalue(_L, lua_upvalueindex(1));
    class = (Class)lua_touserdata(L, lua_upvalueindex (2));

    [[class alloc] init];
    t_configurenode (_L, 1);

    return 1;
}

static dReal heightfield_data_callback (void *data, int x, int z)
{
    roam_Tileset *tiles;
    ElevationBody *self = data;
    double h;

    /* The sign of the x coordinate needs to be flipped.
       See comment about heightfield frames in ODE and
       Techne below. */

    tiles = self->tileset;
    look_up_sample (tiles, (1 << tiles->depth) * tiles->size[0] - x, z, &h, NULL);
    
    /* printf ("%d, %d => %f\n", x, z, h); */
    
    return h;
}

@implementation Elevation

-(void) init
{
    roam_Tileset *tiles;
    int i;

    [super init];

    tiles = &self->tileset;
    
    /* Read in resolution data. */

    lua_pushstring(_L, "depth");
    lua_gettable(_L, 1);

    tiles->depth = lua_tonumber(_L, -1);
    
    lua_pop(_L, 1);

    lua_pushstring(_L, "resolution");
    lua_gettable(_L, 1);

    if(lua_istable(_L, 1)) {
        for(i = 0 ; i < 2 ; i += 1) {
            lua_rawgeti(_L, -1, i + 1);
            tiles->resolution[i] = lua_tonumber(_L, -1);
                
            lua_pop(_L, 1);
        }
    }
    
    lua_pop(_L, 1);

    /* Read in the tiles. */
    
    lua_pushstring(_L, "tiles");
    lua_gettable(_L, 1);

    if (!lua_isnil (_L, -1)) {
        /* Figure out the tileset size. */
        
        lua_len(_L, -1);
        tiles->size[0] = lua_tointeger(_L, -1);
        lua_pop (_L, 1);
        
        lua_rawgeti(_L, -1, 1);
        lua_len(_L, -1);
        tiles->size[1] = lua_tointeger(_L, -1);
        lua_pop (_L, 2);
        
        /* Allocate the tile tables. */
    
        self->references = (int *)calloc (tiles->size[0] *
                                          tiles->size[1] * 2,
                                          sizeof (int));
    
        tiles->samples = (unsigned short **)calloc (tiles->size[0] *
                                                    tiles->size[1],
                                                    sizeof (unsigned short *));
        tiles->bounds = (unsigned short **)calloc (tiles->size[0] *
                                                   tiles->size[1],
                                                   sizeof (unsigned short *));
        tiles->orders = (int *)calloc (tiles->size[0] * tiles->size[1],
                                       sizeof (int));
        tiles->imagery = (unsigned int *)calloc (tiles->size[0] * tiles->size[1],
                                                 sizeof (unsigned int));
        tiles->scales = (double *)calloc (tiles->size[0] * tiles->size[1],
                                          sizeof (double));
        tiles->offsets = (double *)calloc (tiles->size[0] * tiles->size[1],
                                           sizeof (double));

        glGenTextures(tiles->size[0] * tiles->size[1], tiles->imagery);
    
        for (i = 0 ; i < 2 * tiles->size[0] * tiles->size[1] ; i += 1) {
            self->references[i] = LUA_REFNIL;
        }

        for (i = 0 ; i < tiles->size[0] * tiles->size[1] ; i += 1) {
            lua_rawgeti(_L, -1, i + 1);
        
            if (lua_istable (_L, -1)) {
                array_Array *heights, *errors, *pixels;
                double c, delta;

                /* The height samples. */
	    
                lua_rawgeti (_L, -1, 1);

                heights = array_testcompatible (_L, -1,
                                                ARRAY_TYPE | ARRAY_RANK,
                                                ARRAY_TUSHORT, 2);

                if (!heights) {
                    t_print_error("Array specified for elevation data is incompatible.\n");
                    abort();
                }

                if (heights->size[0] != heights->size[1]) {
                    t_print_error("Elevation tiles must be rectangular.\n");
                    abort();
                }

                self->references[2 * i] = luaL_ref(_L, LUA_REGISTRYINDEX);

                /* The error bounds. */
	    
                lua_rawgeti (_L, -1, 2);

                errors = array_testcompatible (_L, -1,
                                               ARRAY_TYPE | ARRAY_RANK,
                                               ARRAY_TUSHORT, 2);

                if (!errors) {
                    t_print_error("Array specified for elevation error bounds is incompatible.\n");
                    abort();
                }

                if (errors->size[0] != errors->size[1]) {
                    t_print_error("Elevation tiles must be rectangular.\n");
                    abort();
                }

                self->references[2 * i + 1] = luaL_ref(_L, LUA_REGISTRYINDEX);

                if (heights->size[0] != errors->size[0]) {
                    t_print_error("Elevation and error bound tiles don't match.\n");
                    abort();
                }

                /* The vertical scale. */

                lua_rawgeti (_L, -1, 4);

                if (lua_istable (_L, -1)) {
                    lua_rawgeti (_L, -1, 1);

                    if (lua_isnumber (_L, -1)) {
                        c = lua_tonumber (_L, -1);
                    } else {
                        c = 1;
                    }
    
                    lua_pop (_L, 1);

                    /* The vertical offset. */
	    
                    lua_rawgeti (_L, -1, 2);

                    if (lua_isnumber (_L, -1)) {
                        delta = lua_tonumber (_L, -1);
                    } else {
                        delta = 0;
                    }
    
                    lua_pop (_L, 1);
                } else {
                    c = 1;
                    delta = 0;
                }

                lua_pop (_L, 1);

                /* Load the elevation tile. */
            
                tiles->samples[i] = heights->values.ushorts;
                tiles->bounds[i] = errors->values.ushorts;
                tiles->orders[i] = (int)(log(heights->size[0] - 1) / log(2));
                tiles->scales[i] = c / USHRT_MAX;
                tiles->offsets[i] = delta;

                /* The imagery. */

                lua_rawgeti (_L, -1, 3);

                if (!lua_isnil (_L, -1)) {
                    pixels = array_testcompatible (_L, -1,
                                                   ARRAY_TYPE | ARRAY_RANK,
                                                   ARRAY_TNUCHAR, 3);

                    if (!pixels) {
                        t_print_error("Array specified for elevation imagery is incompatible.\n");
                        abort();
                    }

                    if (pixels->size[2] != 3) {
                        t_print_error("Elevation imagery data must be specified in RGB format.\n");
                        abort();
                    }

                    /* Create the texture object. */
	
                    glGetError();
                    glBindTexture(GL_TEXTURE_2D, tiles->imagery[i]);
                    
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    
                    glTexImage2D (GL_TEXTURE_2D, 0,
                                  GL_RGB,
                                  pixels->size[0], pixels->size[1], 0,
                                  GL_RGB,
                                  GL_UNSIGNED_BYTE,
                                  pixels->values.uchars);

                    glGenerateMipmap (GL_TEXTURE_2D);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR_MIPMAP_LINEAR);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_MIRRORED_REPEAT);
                }

                lua_pop (_L, 1);
            }

            lua_pop (_L, 1);
        }
    }

    lua_pop (_L, 1);
}

-(int) _get_shape
{
    lua_pop (_L, 1);
    lua_pushlightuserdata(_L, [ElevationShape class]);
    lua_pushcclosure(_L, construct, 2);
    
    return 1;
}

-(void) _set_shape
{
}

-(int) _get_body
{
    lua_pop (_L, 1);
    lua_pushlightuserdata(_L, [ElevationBody class]);
    lua_pushcclosure(_L, construct, 2);
    
    return 1;
}

-(void) _set_body
{
}

-(void) free
{
    roam_Tileset *tiles;
    int i;

    tiles = &self->tileset;

    glDeleteTextures (tiles->size[0] * tiles->size[1], tiles->imagery);
    
    for (i = 0 ; i < 2 * tiles->size[0] * tiles->size[1] ; i += 1) {
        luaL_unref (_L, LUA_REGISTRYINDEX, self->references[i]);
    }

    free (self->references);
    free (tiles->samples);
    free (tiles->bounds);
    free (tiles->imagery);
    free (tiles->orders);
    free (tiles->scales);
    free (tiles->offsets);
    
    [super free];
}

@end

@implementation ElevationShape

-(void) init
{
    roam_Tileset *tiles;
    shape_Buffer *b;
    Elevation *mold;

    /* Make a reference to the mold to make sure it's not
     * collected. */

    mold = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    [super initWithMode: GL_TRIANGLES];

    tiles = &mold->tileset;
    self->context.tileset = tiles;
    self->context.target = 5000;

    self->ranges = (int *)calloc (tiles->size[0] * tiles->size[1],
                                  sizeof (int));

    /* Create the vertex buffer. */

    b = malloc (sizeof (shape_Buffer));
    b->key = strdup("positions");

    glGenBuffers(1, &b->name);
    
    b->type = ARRAY_TFLOAT;
    b->size = 3;
    b->length = 9 * self->context.target * sizeof(float);

    glBindBuffer (GL_ARRAY_BUFFER, b->name);
    glBufferData (GL_ARRAY_BUFFER, b->length, NULL,
                  GL_STREAM_DRAW);

    self->vertices = malloc(b->length);
    self->buffer = b;

    /* Create the base mesh.  */
    
    allocate_mesh (&self->context);
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);

    free_mesh(&self->context);
    free (self->ranges);
        
    [super free];
}

-(void) meetParent: (Shader *)parent
{
    shape_Buffer *b;
    int i;

    if (![parent isKindOf: [Shader class]]) {
	t_print_warning("%s node has no shader parent.\n",
			[self name]);
	
	return;
    }

    b = self->buffer;
    i = glGetAttribLocation(parent->name, b->key);

    /* Bind the VBO into the VAO. */
    
    glBindVertexArray(self->name);
    glVertexAttribPointer(i, b->size, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);

    self->locations.scale = glGetUniformLocation(parent->name, "scale");
    self->locations.offset = glGetUniformLocation(parent->name, "offset");
}

-(int) _get_target
{
    lua_pushnumber (_L, self->context.target);

    return 1;
}

-(int) _get_state
{
    lua_newtable (_L);

    lua_pushnumber (_L, self->context.triangles);
    lua_rawseti (_L, -2, 1);

    lua_pushnumber (_L, self->context.visible);
    lua_rawseti (_L, -2, 2);

    lua_pushnumber (_L, self->context.diamonds);
    lua_rawseti (_L, -2, 3);

    lua_pushnumber (_L, self->context.queued[0]);
    lua_rawseti (_L, -2, 4);

    lua_pushnumber (_L, self->context.queued[1]);
    lua_rawseti (_L, -2, 5);

    lua_pushnumber (_L, self->context.minimum);
    lua_rawseti (_L, -2, 6);

    lua_pushnumber (_L, self->context.maximum);
    lua_rawseti (_L, -2, 7);

    return 1;
}

-(void) _set_state
{
    T_WARN_READONLY;
}

-(void) _set_target
{
    shape_Buffer *b;

    self->context.target = lua_tonumber (_L, 3);

    /* Update the vertex buffer size. */

    b = self->buffer;
    b->length = 9 * self->context.target * sizeof(float);

    glBindBuffer (GL_ARRAY_BUFFER, b->name);
    glBufferData (GL_ARRAY_BUFFER, b->length, NULL,
                  GL_STREAM_DRAW);

    self->vertices = realloc(self->vertices, b->length);
}

-(void) draw
{
    shape_Buffer *b;
    roam_Tileset *tiles;
    int i, j;
    
    tiles = self->context.tileset;

    {
        float M[16] = {tiles->resolution[0], 0, 0, 0,
                       0, tiles->resolution[1], 0, 0,
                       0, 0, 1, 0,
                       -(1 << (tiles->depth - 1)) * tiles->size[0] * tiles->resolution[0],
                       -(1 << (tiles->depth - 1)) * tiles->size[1] * tiles->resolution[1],
                       0, 1};
    
        t_push_modelview (self->matrix, T_MULTIPLY);
        t_load_modelview (M, T_MULTIPLY);
    }
    
    /* glPolygonMode (GL_FRONT_AND_BACK, GL_LINE); */

    optimize_geometry(&self->context, self->vertices, self->ranges);

    /* Update the vertex buffer object. */
    
    b = self->buffer;
    
    glBindBuffer (GL_ARRAY_BUFFER, b->name);
    glBufferData (GL_ARRAY_BUFFER, b->length, NULL, GL_STREAM_DRAW);
    glBufferData (GL_ARRAY_BUFFER, b->length, self->vertices, GL_STREAM_DRAW);

    /* Prepare to draw. */
    
    glBindVertexArray(self->name);
    glUniform1f(self->locations.scale, ldexpf(1, -tiles->depth));
    glActiveTexture(GL_TEXTURE0);
    
    for (i = 0 ; i < tiles->size[0] ; i += 1) {    
	for (j = 0 ; j < tiles->size[1] ; j += 1) {
	    int l, k = i * tiles->size[1] + j;

            glUniform2f(self->locations.offset, j, i);
            glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);

            l = k > 0 ? self->ranges[k - 1] : 0;
            
            glDrawArrays (self->mode, 3 * l, 3 * (self->ranges[k] - l));
        }
    }
    
    t_pop_modelview ();
    
    [super draw];
}

@end

@implementation ElevationBody

-(void) init
{
    roam_Tileset *tiles;
    Elevation *mold;

    /* Make a reference to the mold to make sure it's not
     * collected. */

    mold = t_tonode (_L, -1);
    self->reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    [super init];
    
    tiles = self->tileset = &mold->tileset;
    self->data = dGeomHeightfieldDataCreate();
    
    dGeomHeightfieldDataBuildCallback (self->data,
				       self,
				       heightfield_data_callback,
				       (1 << tiles->depth) *
				       tiles->size[0] * tiles->resolution[0],
				       (1 << tiles->depth) *
				       tiles->size[1] * tiles->resolution[1],
				       (1 << tiles->depth) *
				       tiles->size[0] + 1,
				       (1 << tiles->depth) *
				       tiles->size[1] + 1,
				       1, 0, 100, 0);
    
    dGeomHeightfieldDataSetBounds (self->data, 0, dInfinity);    
    
    /* Create the geom itself. */

    self->geom = dCreateHeightfield(NULL, self->data, 1);

    dGeomSetData (self->geom, self);

    {
	/* ODE assumes that heighfield are oriented with the
	   y-axis pointing upwards, while Techne thinks the
	   z-axis does.  In order to align the two frames we
	   need to apply a transform to the geom but in the
	   process the x-axis has to be negated to keep the
	   handedness of both frames the same. */
	
	dMatrix3 R = {-self->orientation[0],
		      self->orientation[2],
		      self->orientation[1],
		      0,
	
		      -self->orientation[3],
		      self->orientation[5],
		      self->orientation[4],
		      0,

		      -self->orientation[6],
		      self->orientation[8],
		      self->orientation[7],
		      0};
	
	dGeomSetRotation (self->geom, R);
	dGeomSetPosition (self->geom,
			  self->position[0],
			  self->position[1],
			  self->position[2]);
    }
}

-(void) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->reference);
    dGeomHeightfieldDataDestroy(self->data);
        
    [super free];
}

-(void) _set_orientation
{    
    dMatrix3 R;
	
    [super _set_orientation];

    /* See comment about heightfield frames
       in ODE and Techne above. */
	    
    R[0] = -self->orientation[0];
    R[1] = self->orientation[2];
    R[2] = self->orientation[1];
    R[3] = 0;
	
    R[4] = -self->orientation[3];
    R[5] = self->orientation[5];
    R[6] = self->orientation[4];
    R[7] = 0;

    R[8] = -self->orientation[6];
    R[9] = self->orientation[8];
    R[10] = self->orientation[7];
    R[11] = 0;

    dGeomSetRotation (self->geom, R);
}

@end
