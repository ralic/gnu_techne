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
#include <ode/ode.h>

#include "roam.h"
#include "shape.h"
#include "body.h"
#include "shader.h"

#define BINS_N 32

#define SEED_BUFFER_SIZE (10000)
#define SEED_SIZE (3 * 3 * 4)
#define VEGETATION_LEVEL_BIAS 0

typedef struct {
    char *buffer;
    int total, fill, capacity;
    double mean, sum;
} elevation_Bin;

@interface Elevation: Node {
@public
    roam_Context context;
    int swatches, *references;
    double separation, albedo;
}

-(int) _get_shape;
-(void) _set_shape;
-(int) _get_seeds;
-(void) _set_seeds;
-(int) _get_body;
-(void) _set_body;
-(int) _get_separation;
-(void) _set_separation;
-(int) _get_albedo;
-(void) _set_albedo;

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

    unsigned int buffer;
    int reference, optimize, *ranges;
}

-(int) _get_target;
-(void) _set_target;

-(int) _get_optimize;
-(void) _set_optimize;

@end

@interface ElevationBody: Body {
@public
    roam_Tileset *tileset;
    dHeightfieldDataID data;
    int reference;
}

@end

@interface ElevationShader: Shader {
@public
}

@end

@interface ElevationSeeds: Shape {
@public
    roam_Context *context;
    elevation_Bin bins[BINS_N];

    struct {
        unsigned int scale, offset;
    } locations;

    struct {
        unsigned int base;
    } units;

    unsigned int buffer;
    int reference;

    double density, bias, horizon, error;
    int triangles_n[2];
}

-(int) _get_density;
-(void) _set_density;

-(int) _get_bias;
-(void) _set_bias;

-(int) _get_bins;
-(void) _set_bins;

-(int) _get_triangles;
-(void) _set_triangles;

-(int) _get_horizon;
-(void) _set_horizon;

-(int) _get_error;
-(void) _set_error;

@end

#endif
