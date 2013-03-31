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

#ifndef _SPLAT_H_
#define _SPLAT_H_

#include "shader.h"

typedef struct {
    unsigned int texture;
    float values[8];
    int reference;
} splat_Pigment;

@interface Splat: Shader {
@public
    double albedo, separation;

    splat_Pigment *pigments;
    int pigments_n;

    struct {
        unsigned int power, references, weights, resolutions;
        unsigned int turbidity, factor, beta_p;
        unsigned int direction, intensity, beta_r;
    } locations;

    struct {
        unsigned int base;
    } units;
}

-(int) _get_albedo;
-(int) _get_separation;
-(void) _set_albedo;
-(void) _set_separation;
-(int) _get_palette;
-(void) _set_palette;

@end

#endif
