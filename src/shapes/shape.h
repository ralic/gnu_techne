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

#ifndef _SHAPE_H_
#define _SHAPE_H_

#include "transform.h"
#include "array/array.h"

@interface Shape: Transform {
    double width, color[4];
    array_Array *vertices;
    int reference;
}

-(int) _get_vertices;
-(int) _get_width;
-(int) _get_opacity;
-(int) _get_color;

-(void) _set_vertices;
-(void) _set_width;
-(void) _set_opacity;
-(void) _set_color;

@end

#endif
