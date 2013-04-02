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

#include "elevation.h"
#include "shader.h"

@interface Splat: Shader {
@public
    Elevation *elevation;
    int reference_1;

    struct {
        unsigned int power, references, weights, resolutions;
        unsigned int turbidity, factor, beta_p;
        unsigned int direction, intensity, beta_r;
    } locations;
}
@end

#endif
