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

#ifndef _VEGETATION_H_
#define _VEGETATION_H_

#include <lua.h>
#include "shader.h"
#include "elevation.h"

@interface Vegetation: Shader {
@public
    roam_Context *context;
    seeding_Context seeding;
    Elevation *elevation;
    Shader **species;
    
    int reference_1;
    unsigned int feedback, *arrays, *vertexbuffers;
    
    struct {
        unsigned int intensity, scale, offset, clustering, instances;
        unsigned int thresholds;
    } locations;

    struct {
        unsigned int base;
    } units;
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

@interface VegetationSpecies: Shader {
@public
    int offset;
    double threshold;
}

-(int) _get_threshold;
-(void) _set_threshold;

@end

#endif
