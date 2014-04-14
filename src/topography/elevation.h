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

#ifndef _ELEVATION_H_
#define _ELEVATION_H_

#include <lua.h>
#include <GL/gl.h>

#include "roam.h"
#include "seeding.h"
#include "shape.h"
#include "shader.h"

typedef struct {
    Texture *detail[3];
    int references[3];
    float resolutions[3][2];

    float values[3], weights[3];
} elevation_SwatchDetail;

@interface Elevation: Node {
@public
    roam_Context context;
    elevation_SwatchDetail *swatches;
    int swatches_n, *bands_n, *references;
    float separation, albedo;
}

-(int) _get_shape;
-(void) _set_shape;
-(int) _get_separation;
-(void) _set_separation;
-(int) _get_albedo;
-(void) _set_albedo;
-(int) _get_swatches;
-(void) _set_swatches;

@end

@interface ElevationShape: Shape {
@public
    roam_Context *context;

    struct {
        unsigned int scale, offset;
    } locations;

    struct {
        unsigned int base;
    } units;

    t_ProfilingCount triangles, culled, visible;
    t_ProfilingCount diamonds, splittable, mergeable;

    unsigned int buffer;
    int reference, optimize, *ranges;
}

-(int) _get_target;
-(void) _set_target;

-(int) _get_optimize;
-(void) _set_optimize;

-(int) _get_triangles;
-(void) _set_triangles;

-(int) _get_culled;
-(void) _set_culled;

-(int) _get_visible;
-(void) _set_visible;

-(int) _get_diamonds;
-(void) _set_diamonds;

-(int) _get_splittable;
-(void) _set_splittable;

-(int) _get_mergeable;
-(void) _set_mergeable;

-(int) _get_reculling;
-(void) _set_reculling;

-(int) _get_reordering;
-(void) _set_reordering;

-(int) _get_tessellation;
-(void) _set_tessellation;

@end

@interface ElevationShader: Shader {
@public
}

@end

#endif
