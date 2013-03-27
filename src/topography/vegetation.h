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

@interface Vegetation: Shader {
@public
    double separation;
    int swatches;

    struct {
        unsigned int colors, power;
    } locations;
}

-(int) _get_separation;
-(void) _set_separation;
-(void) update;

@end

#endif
