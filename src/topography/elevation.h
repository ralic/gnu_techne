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

@interface Elevation: Node {
@public
    roam_Context context;
    int *references;
}

-(int) _get_shape;
-(void) _set_shape;
-(int) _get_vegetation;
-(void) _set_vegetation;
-(int) _get_body;
-(void) _set_body;

@end

@interface ElevationShape: Shape {
@public
    roam_Context *context;

    struct {
        unsigned int scale, offset;
    } locations;

    unsigned int buffer;
    float *vertices;
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

@interface Vegetation: Shape {
@public
    roam_Context *context;

    struct {
        unsigned int scale, offset;
    } locations;

    int reference;

    double density, bias;
}

-(int) _get_density;
-(void) _set_density;

-(int) _get_bias;
-(void) _set_bias;

@end

#endif
