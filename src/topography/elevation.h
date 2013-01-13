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

typedef struct {
    unsigned short **samples;
    unsigned short **bounds;
    double *scales, *offsets;
    GLuint *imagery;
    int *orders;

    int size[2], depth;
    double resolution[2];
} elevation_Tileset;

@interface ElevationMold: Node {
@public
    elevation_Tileset tileset;

    int *references;
}

-(int) _get_shape;
-(void) _set_shape;

@end

@interface ElevationShape: Shape {
@public
    elevation_Tileset *tileset;

    int reference;

    struct block *pools[2];
    
    struct diamond *queues[2][QUEUE_SIZE];
    struct triangle *(*roots)[2];    
    
    int blocks[2], chunks[2], queued[2];
    int triangles, diamonds, culled, visible, drawn;
    int minimum, maximum;

    int target;
}

-(int) _get_target;
-(void) _set_target;
-(int) _get_state;
-(void) _set_state;

@end

@interface ElevationBody: Body {
@public
    elevation_Tileset *tileset;
    dHeightfieldDataID data;
    int reference;
}

@end

#endif
