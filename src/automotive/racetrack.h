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

#ifndef _RACETRACK_H_
#define _RACETRACK_H_

#include "body.h"
#include "shape.h"
#include "topography/roam.h"

struct trackdata {
    /* Tarmac. */

    double *segments, tolerance;
    int segments_n, last;

    /* Terrain. */

    dGeomID field;
    roam_Tileset *tileset;
};

int dTrackClass;

@interface Racetrack: Node {
@public
    double *segments, tolerance;
    int segments_n;
}

-(int) _get_segments;	
-(void) _set_segments;

-(int) _get_tolerance;
-(void) _set_tolerance;

-(int) _get_body;
-(void) _set_body;

-(int) _get_shape;
-(void) _set_shape;

@end

@interface RacetrackShape: Shape {
    double *segments, tolerance;
    int segments_n;
    
    double tessellation[2], scale[2];
    int dirty, reference;
}

-(int) _get_scale;
-(int) _get_tessellation;

-(void) _set_scale;
-(void) _set_tessellation;

@end

@interface RacetrackBody: Body {
    int reference;
}

-(int) _get_sampler;
-(void) _set_sampler;

@end

#endif
