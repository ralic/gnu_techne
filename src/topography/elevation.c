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
#include "vegetation.h"
#include "seeding.h"
#include "splat.h"
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

@implementation Elevation

-(void) init
{
    roam_Tileset *tiles;
    int i;

    [super init];

    tiles = &self->context.tileset;

    self->swatches = NULL;
    self->swatches_n = 0;
    self->bands_n = NULL;
    self->separation = 1;

    /* Read in resolution data. */

    lua_pushstring(_L, "depth");
    lua_gettable(_L, 1);

    tiles->depth = lua_tonumber(_L, -1);

    lua_pop(_L, 1);

    lua_pushstring(_L, "resolution");
    lua_gettable(_L, 1);

    if(lua_istable(_L, -1)) {
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
        int j, k;

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

        for (i = 0 ; i < tiles->size[0] ; i += 1) {
            lua_rawgeti(_L, -1, i + 1);

            for (j = 0 ; j < tiles->size[1] ; j += 1) {
                lua_rawgeti(_L, -1, j + 1);

                if (!lua_isnil (_L, -1)) {
                    array_Array *heights, *errors, *rgb;
                    double c, delta;

                    k = i * tiles->size[1] + j;

                    /* The height samples. */

                    lua_rawgeti (_L, -1, 1);

                    heights = array_testarray (_L, -1);

                    if (!heights) {
                        t_print_error("No elevation data specified for tile %d.\n");
                        abort();
                    }

                    if (heights->rank != 2) {
                        t_print_error("Elevation array is of unsuitable rank.\n");
                        abort();
                    }

                    if (abs(heights->type) != ARRAY_TUSHORT) {
                        t_print_error("Elevation array is of unsuitable type.\n");
                        abort();
                    }

                    if (heights->size[0] != heights->size[1]) {
                        t_print_error("Elevation tiles must be rectangular.\n");
                        abort();
                    }

                    self->references[2 * k] = luaL_ref(_L, LUA_REGISTRYINDEX);

                    /* The error bounds. */

                    lua_rawgeti (_L, -1, 2);

                    if (lua_isnil(_L, -1)) {
                        /* If no precalculated bounds have been specified,
                           calculate them now. */

                        lua_pop(_L, 1);
                        errors = array_createarrayv(_L, ARRAY_TUSHORT, NULL, 2,
                                                    heights->size);

                        calculate_tile_bounds (heights->values.ushorts,
                                               errors->values.ushorts,
                                               heights->size[0]);
                    }

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

                    self->references[2 * k + 1] = luaL_ref(_L, LUA_REGISTRYINDEX);

                    if (heights->size[0] != errors->size[0]) {
                        t_print_error("Elevation and error bound tiles don't match.\n");
                        abort();
                    }

                    /* The vertical scale. */

                    lua_rawgeti (_L, -1, 4);

                    if (lua_istable (_L, -1)) {
                        /* Scale. */

                        lua_rawgeti (_L, -1, 1);

                        if (lua_isnumber (_L, -1)) {
                            c = lua_tonumber (_L, -1);
                        } else {
                            c = 1;
                        }

                        lua_pop (_L, 1);

                        /* Offset. */

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

                    tiles->samples[k] = heights->values.ushorts;
                    tiles->bounds[k] = errors->values.ushorts;
                    tiles->orders[k] = (int)(log(heights->size[0] - 1) / log(2));
                    tiles->scales[k] = c / USHRT_MAX;
                    tiles->offsets[k] = delta;

                    /* The imagery. */

                    lua_rawgeti (_L, -1, 3);

                    if (!lua_isnil (_L, -1)) {
                        rgb = array_testcompatible (_L, -1,
                                                    ARRAY_TYPE | ARRAY_RANK,
                                                    ARRAY_TNUCHAR, 3);

                        if (!rgb) {
                            t_print_error("Array specified for elevation imagery is incompatible.\n");
                            abort();
                        }

                        if (rgb->size[2] != 3) {
                            t_print_error("Elevation imagery data must be specified in RGB format.\n");
                            abort();
                        }

                        /* Create the texture object. */

                        glGetError();
                        glBindTexture(GL_TEXTURE_2D, tiles->imagery[k]);

                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                        glTexImage2D (GL_TEXTURE_2D, 0,
                                      GL_RGB,
                                      rgb->size[0], rgb->size[1], 0,
                                      GL_RGB,
                                      GL_UNSIGNED_BYTE,
                                      rgb->values.uchars);

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

            lua_pop (_L, 1);
        }

        t_print_message ("Loaded a %d by %d tileset.\n",
                         tiles->size[0], tiles->size[1]);

        /* Calculate some derivative quantities. */

        tiles->offset[0] = -(1 << (tiles->depth - 1)) * tiles->size[0] * tiles->resolution[0];
        tiles->offset[1] = -(1 << (tiles->depth - 1)) * tiles->size[1] * tiles->resolution[1];

        /* Create the base mesh.  */

        allocate_mesh (&self->context);
    }

    lua_pop (_L, 1);

    /* Remove the set values from the initialization table. */

    lua_pushstring(_L, "tiles");
    lua_pushnil(_L);
    lua_settable (_L, 1);

    lua_pushstring(_L, "resolution");
    lua_pushnil(_L);
    lua_settable (_L, 1);

    lua_pushstring(_L, "depth");
    lua_pushnil(_L);
    lua_settable (_L, 1);
}

-(int) _get_swatches
{
    int i, j;

    lua_createtable (_L, self->swatches_n, 0);

    for (i = 0 ; i < self->swatches_n ; i += 1) {
        elevation_SwatchDetail *swatch;
        int k;

        swatch = &self->swatches[i];

        lua_createtable (_L, 2, 0);

        /* The reference color. */

        lua_createtable (_L, 3, 0);

        for (j = 0; j < 3 ; j += 1) {
            if (swatch->weights[j] > 0) {
                lua_pushnumber(_L, swatch->values[j]);
            } else {
                lua_pushnil(_L);
            }

            lua_rawseti(_L, -2, j + 1);
        }

        lua_rawseti(_L, -2, 1);

        /* The detail textures. */

        lua_createtable (_L, self->bands_n[i], 0);

        for (k = 0 ; k < self->bands_n[i] ; k += 1) {
            lua_createtable(_L, 2, 0);

            lua_rawgeti(_L, LUA_REGISTRYINDEX, swatch->references[k]);
            lua_rawseti(_L, -2, 1);

            /* The resolution. */

            lua_createtable (_L, 2, 0);

            for (j = 0; j < 2 ; j += 1) {
                lua_pushnumber(_L, swatch->resolutions[k][j]);
                lua_rawseti(_L, -2, j + 1);
            }

            lua_rawseti(_L, -2, 2);
            lua_rawseti(_L, -2, k + 1);
        }

        lua_rawseti(_L, -2, 2);
        lua_rawseti(_L, -2, i + 1);
    }

    return 1;
}

-(void) _set_swatches
{
    int i, j, n;

    n = lua_rawlen (_L, 3);

    self->swatches_n = n;
    self->swatches = realloc(self->swatches,
                             n * sizeof(elevation_SwatchDetail));
    self->bands_n = realloc(self->bands_n,
                            n * sizeof(int));

    for (i = 0 ; i < n ; i += 1) {
        elevation_SwatchDetail *swatch;
        int k;

        swatch = &self->swatches[i];

        lua_rawgeti (_L, 3, i + 1);

        /* The reference color. */

        lua_rawgeti (_L, -1, 1);

        for (j = 0 ; j < 3 ; j += 1) {
            lua_pushinteger (_L, j + 1);
            lua_gettable (_L, -2);

            if (lua_isnumber(_L, -1)) {
                swatch->values[j] = lua_tonumber (_L, -1);
                swatch->weights[j] = 1;
            } else {
                swatch->values[j] = 0;
                swatch->weights[j] = 0;
            }

            lua_pop(_L, 1);
        }

        lua_pop (_L, 1);

        /* The bands. */

        lua_rawgeti (_L, -1, 2);
        self->bands_n[i] = lua_rawlen (_L, -1);

        for (k = 0 ; k < self->bands_n[i] ; k += 1) {
            lua_rawgeti (_L, -1, k + 1);

            /* The detail texture. */

            lua_rawgeti (_L, -1, 1);
            swatch->detail[k] = t_totexture (_L, -1, GL_TEXTURE_2D);
            swatch->references[k] = luaL_ref(_L, LUA_REGISTRYINDEX);

            /* The resolution. */

            lua_rawgeti (_L, -1, 2);

            for (j = 0 ; j < 2 ; j += 1) {
                lua_pushinteger (_L, j + 1);
                lua_gettable (_L, -2);
                swatch->resolutions[k][j] = lua_tonumber(_L, -1);
                lua_pop (_L, 1);
            }

            lua_pop (_L, 2);
        }

        lua_pop (_L, 2);
    }
}

-(int) _get_separation
{
    lua_pushnumber (_L, self->separation);

    return 1;
}

-(void) _set_separation
{
    self->separation = lua_tonumber (_L, -1);
}

-(int) _get_albedo
{
    lua_pushnumber (_L, self->albedo);

    return 1;
}

-(void) _set_albedo
{
    self->albedo = lua_tonumber (_L, -1);
}

-(int) _get_shape
{
    lua_pushvalue (_L, 1);
    lua_pushlightuserdata(_L, [ElevationShape class]);
    lua_pushcclosure(_L, construct, 2);

    return 1;
}

-(void) _set_shape
{
}

-(int) _get_vegetation
{
    lua_pushvalue (_L, 1);
    lua_pushlightuserdata(_L, [/* Elevation */Vegetation class]);
    lua_pushcclosure(_L, construct, 2);

    return 1;
}

-(void) _set_vegetation
{
}

-(int) _get_splat
{
    lua_pushvalue (_L, 1);
    lua_pushlightuserdata(_L, [/* Elevation */Splat class]);
    lua_pushcclosure(_L, construct, 2);

    return 1;
}

-(void) _set_splat
{
}

-(void) free
{
    roam_Tileset *tiles;
    int i;

    tiles = &self->context.tileset;

    glDeleteTextures (tiles->size[0] * tiles->size[1], tiles->imagery);

    for (i = 0 ; i < 2 * tiles->size[0] * tiles->size[1] ; i += 1) {
        luaL_unref (_L, LUA_REGISTRYINDEX, self->references[i]);
    }

    for (i = 0 ; i < self->swatches_n ; i += 1) {
        glDeleteTextures (1, &self->swatches[i].detail[0]->name);
        glDeleteTextures (1, &self->swatches[i].detail[1]->name);
        luaL_unref (_L, LUA_REGISTRYINDEX, self->swatches[i].references[0]);
        luaL_unref (_L, LUA_REGISTRYINDEX, self->swatches[i].references[1]);
    }

    free_mesh(&self->context);

    free (self->swatches);
    free (self->bands_n);
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
