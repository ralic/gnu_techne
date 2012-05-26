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

struct trackdata {
    /* Tarmac. */

    double *segments, tolerance;
    int size, last;

    /* Terrain. */

    dGeomID field;
    const int *tiles;
    int depth;
    const double *resolution;
    void (*sampler)(int, int, double *, double *);
};

int dTrackClass;

@interface Racetrack: Body {
    double *vertices, *uv, *normals;    
    double tessellation[3], scale[2];
    int size, dirty;
}

-(void) update;

-(int) _get_element;	
-(int) _get_sampler;
-(int) _get_vertices;
-(int) _get_scale;
-(int) _get_tessellation;

-(void) _set_element;
-(void) _set_vertices;
-(void) _set_sampler;
-(void) _set_scale;
-(void) _set_tessellation;

@end

#endif
