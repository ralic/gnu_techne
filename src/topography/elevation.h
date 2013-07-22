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
#include "seeding.h"
#include "shape.h"
#include "body.h"
#include "shader.h"

typedef struct {
    Texture *detail;
    int reference;

    float values[3], weights[3], resolution[2];
} elevation_SwatchDetail;

@interface Elevation: Node {
@public
    roam_Context context;
    elevation_SwatchDetail *swatches;
    int swatches_n, *references;
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

    unsigned int buffer;
    int reference, optimize, *ranges;
}

-(int) _get_target;
-(void) _set_target;

-(int) _get_optimize;
-(void) _set_optimize;

-(int) _get_triangles;
-(void) _set_triangles;

-(int) _get_diamonds;
-(void) _set_diamonds;

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
    seeding_Context seeding;
    
    struct {
        unsigned int scale, offset, clustering;
    } locations;

    struct {
        unsigned int base;
    } units;

    unsigned int buffer;
    int reference;
}

-(int) _get_density;
-(void) _set_density;

-(int) _get_ceiling;
-(void) _set_ceiling;

-(int) _get_clustering;
-(void) _set_clustering;

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
