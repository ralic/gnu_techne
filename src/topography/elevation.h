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
    roam_Tileset tileset;

    int *references;
}

-(int) _get_shape;
-(void) _set_shape;

@end

@interface ElevationShape: Shape {
@public
    struct {
        unsigned int scale, offset;
    } locations;

    roam_Context context;
    shape_Buffer *buffer;
    float *vertices;
    int reference, *ranges;
}

-(int) _get_target;
-(void) _set_target;
-(int) _get_state;
-(void) _set_state;

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

#endif
